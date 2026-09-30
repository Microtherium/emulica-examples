from __future__ import annotations

import os
import subprocess
import sys
import textwrap
from pathlib import Path


PROJECT = "nxp/k32w041/i2c_polling_transfer"


def find_client_dir(start: Path) -> Path:
    for path in (start, *start.parents):
        if (path / "examples").is_dir():
            return path
    raise RuntimeError("Could not find Emulica src/client directory")


def find_legacy_runner(client_dir: Path) -> Path | None:
    runner = client_dir / "project_runner.py"
    return runner if runner.is_file() else None


def find_server_dir(client_dir: Path) -> Path | None:
    workspace = client_dir.parent
    server_dir = workspace / "replica-server"
    return server_dir if (server_dir / "server" / "services" / "execution_service.py").is_file() else None


def run_current_local_server_path(client_dir: Path, server_dir: Path, args: list[str]) -> int:
    if args:
        print(
            "warning: this standalone fallback runs the local server engine directly; "
            f"ignoring unsupported arguments: {' '.join(args)}",
            file=sys.stderr,
        )

    example_dir = client_dir / "examples" / PROJECT
    platform_repl = example_dir.parent / "platform.repl"
    script = textwrap.dedent(
        """
        import json
        import os
        import sys
        from pathlib import Path

        server_root = Path(os.environ["EMULICA_REPLICA_SERVER"])
        server_dir = server_root / "server"
        for path in (str(server_dir), str(server_root)):
            if path not in sys.path:
                sys.path.insert(0, path)

        import execution_pb2 as pb
        from services.execution_service import ExecutionServicer


        class Ctx:
            def is_active(self):
                return True


        example_dir = Path(os.environ["EMULICA_EXAMPLE_DIR"])
        platform_repl = Path(os.environ["EMULICA_PLATFORM_REPL"])
        request = pb.SimulationRequest(
            api_key="emulica-dev-key-2026",
            mcu="nxp.k32w041",
            resc=(example_dir / "sim.resc").read_text(encoding="utf-8"),
            platform_repl=platform_repl.read_text(encoding="utf-8"),
            firmware=(example_dir / "firmware" / "firmware.elf").read_bytes(),
            simulation_json=json.dumps({}),
            timeout_seconds=3,
        )

        for event in ExecutionServicer().RunSimulation(request, Ctx()):
            payload = event.WhichOneof("payload")
            if payload:
                print(getattr(event, payload))
        """
    )
    env = os.environ.copy()
    env["EMULICA_REPLICA_SERVER"] = str(server_dir)
    env["EMULICA_EXAMPLE_DIR"] = str(example_dir)
    env["EMULICA_PLATFORM_REPL"] = str(platform_repl)
    return subprocess.run(["pixi", "run", "python", "-c", script], cwd=server_dir, env=env).returncode


def main() -> int:
    client_dir = find_client_dir(Path(__file__).resolve().parent)
    env = os.environ.copy()
    env.setdefault("EMULICA_API_KEY", "emulica-dev-key-2026")

    runner = find_legacy_runner(client_dir)
    if runner is None:
        server_dir = find_server_dir(client_dir)
        if server_dir is None:
            raise RuntimeError(
                "Could not find project_runner.py or sibling replica-server checkout. "
                "Run this example from the VS Code extension, or clone replica-server next to replica-vscode-client."
            )
        return run_current_local_server_path(client_dir, server_dir, sys.argv[1:])

    command = [
        sys.executable,
        str(runner),
        "simulate",
        "--project",
        PROJECT,
        *sys.argv[1:],
    ]
    return subprocess.run(command, cwd=client_dir, env=env).returncode


if __name__ == "__main__":
    raise SystemExit(main())
