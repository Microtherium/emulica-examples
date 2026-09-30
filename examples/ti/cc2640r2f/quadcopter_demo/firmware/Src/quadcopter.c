/* Quadcopter flight-control task for the CC2640R2F FreeRTOS demo.
 *
 * Every 10 ms (a real, scheduled FreeRTOS task - osDelay-driven, not a
 * busy loop) it runs one PID attitude-control step on roll, pitch and yaw,
 * mixes the three PID outputs plus a fixed hover throttle into four motor
 * duty cycles (X configuration), and writes them to four real GPT PWM
 * channels (GPT0..GPT3 Timer A, one per motor).
 *
 * **The airframe is simulated in software, not a real IMU.** The emulator
 * models no accelerometer/gyro and no GPT peripheral, so there is nothing
 * real to read or drive. Instead the plant below integrates the torques
 * produced by the *actually written* motor duties, so the loop is
 * genuinely closed: the PID sees the aircraft start tilted and disturbed,
 * and its motor commands converge as the simulated attitude settles. The
 * PWM register writes are what sim.resc's watchpoint hooks observe.
 *
 * Only single-precision float is used (no double): this is a Cortex-M3
 * with no FPU, so every operation is a soft-float library call. */
#include "cc2640r2f_regs.h"
#include "cmsis_os.h"
#include "quadcopter.h"

/* PWM: 48 MHz timer clock, 60000-tick period = 800 Hz. The hooks in
 * sim.resc decode a motor's pulse as PWM_PERIOD_TICKS - TAMATCHR. */
#define PWM_PERIOD_TICKS 60000U
#define NUM_MOTORS       4

#define LOOP_MS          10
#define LOOP_DT          0.01f
#define HEARTBEAT_LOOPS  100      /* one UART status line per second */
#define DISTURBANCE_LOOP 300      /* kick the roll axis at t = 3 s */

/* Motor index order matches sim.resc: 0=FL 1=FR 2=RL 3=RR. */
enum { M_FL, M_FR, M_RL, M_RR };

#define HOVER_THROTTLE   0.50f
#define DUTY_MIN         0.05f
#define DUTY_MAX         0.95f

/* Simulated airframe - degrees and degrees/second. */
#define TORQUE_GAIN      3000.0f  /* deg/s^2 per unit of normalized torque */
#define DAMPING          2.0f     /* 1/s */

typedef struct {
    float kp, ki, kd;
    float integral;
    float prev_measured;
    float limit;                  /* symmetric output and integral clamp */
} pid_ctrl_t;

typedef struct {
    float angle;                  /* deg */
    float rate;                   /* deg/s */
} axis_t;

/* One GPT per motor, indexed by the M_* enum above. */
static const uint32_t kMotorTimerBase[NUM_MOTORS] = { GPT0_BASE, GPT1_BASE, GPT2_BASE, GPT3_BASE };

static const char kQuadPwmMarker[] = "EMULICA_CC2640R2F_QUADCOPTER_PWM_OK\r\n";

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/* Derivative is taken on the measurement, not the error, so a setpoint
 * change never produces a derivative kick. */
static float pid_step(pid_ctrl_t *pid, float setpoint, float measured) {
    float error = setpoint - measured;
    pid->integral = clampf(pid->integral + pid->ki * error * LOOP_DT, -pid->limit, pid->limit);
    float derivative = -(measured - pid->prev_measured) / LOOP_DT;
    pid->prev_measured = measured;
    return clampf(pid->kp * error + pid->integral + pid->kd * derivative, -pid->limit, pid->limit);
}

/* GPT0..GPT3 Timer A as 16-bit PWM, all idle (0 % duty) until the first
 * control step. Order matters: clock gate first, then configure, then
 * arm (TAEN) last - the same order real driverlib uses. */
static void quadcopter_pwm_init(void) {
    PRCM_GPTCLKGR = 0xFU;                         /* GPT0..GPT3 clocks */
    PRCM_CLKLOADCTL = PRCM_CLKLOADCTL_LOAD;
    while (!(PRCM_CLKLOADCTL & PRCM_CLKLOADCTL_LOAD_DONE)) { }

    for (uint32_t m = 0; m < NUM_MOTORS; m++) {
        uint32_t base = kMotorTimerBase[m];
        GPT_CTL(base) = 0;                         /* disabled while configuring */
        GPT_CFG(base) = GPT_CFG_16BIT;
        GPT_TAMR(base) = GPT_TAMR_PWM;
        GPT_TAPR(base) = 0;
        GPT_TAILR(base) = PWM_PERIOD_TICKS;
        GPT_TAMATCHR(base) = PWM_PERIOD_TICKS;     /* pulse = 0 ticks */
        GPT_CTL(base) |= GPT_CTL_TAEN;
    }
    /* The PWM outputs' IOC pad routing is intentionally not done here:
     * the emulator has no pad/GPT model to route them to. */
}

static void motor_write(uint32_t motor, float duty) {
    uint32_t pulse = (uint32_t)(clampf(duty, DUTY_MIN, DUTY_MAX) * (float)PWM_PERIOD_TICKS);
    GPT_TAMATCHR(kMotorTimerBase[motor]) = PWM_PERIOD_TICKS - pulse;
}

/* Tiny integer-only formatter - no printf/float formatting, which would
 * pull the (large) newlib float printf into a 128 KB flash / 20 KB SRAM
 * part. Appends a signed decimal to buf, returns the new end. */
static char *put_int(char *p, int v) {
    char tmp[12];
    int n = 0;
    unsigned int u;
    if (v < 0) { *p++ = '-'; u = 0U - (unsigned int)v; } else { u = (unsigned int)v; }
    do { tmp[n++] = (char)('0' + (u % 10U)); u /= 10U; } while (u);
    while (n) { *p++ = tmp[--n]; }
    return p;
}

static char *put_str(char *p, const char *s) {
    while (*s) { *p++ = *s++; }
    return p;
}

static void report(const axis_t *roll, const axis_t *pitch, const axis_t *yaw) {
    char line[96];
    char *p = line;
    p = put_str(p, "EMULICA_CC2640R2F_QUADCOPTER_HEARTBEAT roll=");
    p = put_int(p, (int)roll->angle);
    p = put_str(p, " pitch=");
    p = put_int(p, (int)pitch->angle);
    p = put_str(p, " yaw=");
    p = put_int(p, (int)yaw->angle);
    p = put_str(p, "\r\n");
    *p = '\0';
    uart0_put_string_safe(line);
}

void StartQuadcopterTask(void const *argument) {
    (void)argument;

    /* Gains chosen for a ~10 rad/s, ~0.7-damped closed loop against the
     * simulated airframe (wn^2 = TORQUE_GAIN*kp, 2*zeta*wn = DAMPING +
     * TORQUE_GAIN*kd). Yaw is gentler since it only trims heading. */
    pid_ctrl_t pid_roll  = { 0.030f, 0.020f, 0.0040f, 0.0f, 0.0f, 0.35f };
    pid_ctrl_t pid_pitch = { 0.030f, 0.020f, 0.0040f, 0.0f, 0.0f, 0.35f };
    pid_ctrl_t pid_yaw   = { 0.015f, 0.010f, 0.0020f, 0.0f, 0.0f, 0.20f };

    /* Start tilted and slightly off-heading so there is real error for
     * the PID to remove (and so the first PWM writes are not all equal). */
    axis_t roll  = { 10.0f, 0.0f };
    axis_t pitch = { -6.0f, 0.0f };
    axis_t yaw   = {  5.0f, 0.0f };

    quadcopter_pwm_init();
    uart0_put_string_safe(kQuadPwmMarker);

    uint32_t loop = 0;
    for (;;) {
        /* 1. Control: attitude setpoint is level, zero heading change. */
        float roll_out  = pid_step(&pid_roll,  0.0f, roll.angle);
        float pitch_out = pid_step(&pid_pitch, 0.0f, pitch.angle);
        float yaw_out   = pid_step(&pid_yaw,   0.0f, yaw.angle);

        /* 2. Mix into four motor duties (X configuration). */
        float duty[NUM_MOTORS];
        duty[M_FL] = HOVER_THROTTLE + roll_out + pitch_out - yaw_out;
        duty[M_FR] = HOVER_THROTTLE - roll_out + pitch_out + yaw_out;
        duty[M_RL] = HOVER_THROTTLE + roll_out - pitch_out + yaw_out;
        duty[M_RR] = HOVER_THROTTLE - roll_out - pitch_out - yaw_out;

        /* 3. Actuate: real PWM register writes. */
        for (uint32_t m = 0; m < NUM_MOTORS; m++) {
            duty[m] = clampf(duty[m], DUTY_MIN, DUTY_MAX);
            motor_write(m, duty[m]);
        }

        /* 4. Simulated airframe: torques from the duties actually
         * written (post-clamp), so saturation feeds back into the plant. */
        float roll_tq  = 0.25f * ((duty[M_FL] + duty[M_RL]) - (duty[M_FR] + duty[M_RR]));
        float pitch_tq = 0.25f * ((duty[M_FL] + duty[M_FR]) - (duty[M_RL] + duty[M_RR]));
        float yaw_tq   = 0.25f * ((duty[M_FR] + duty[M_RL]) - (duty[M_FL] + duty[M_RR]));

        roll.rate  += (TORQUE_GAIN * roll_tq  - DAMPING * roll.rate)  * LOOP_DT;
        pitch.rate += (TORQUE_GAIN * pitch_tq - DAMPING * pitch.rate) * LOOP_DT;
        yaw.rate   += (TORQUE_GAIN * yaw_tq   - DAMPING * yaw.rate)   * LOOP_DT;
        roll.angle  += roll.rate  * LOOP_DT;
        pitch.angle += pitch.rate * LOOP_DT;
        yaw.angle   += yaw.rate   * LOOP_DT;

        loop++;
        if (loop == DISTURBANCE_LOOP) {
            roll.rate += 30.0f;        /* a gust: watch the duties react */
        }
        if (loop % HEARTBEAT_LOOPS == 0) {
            report(&roll, &pitch, &yaw);
        }

        osDelay(LOOP_MS);
    }
}
