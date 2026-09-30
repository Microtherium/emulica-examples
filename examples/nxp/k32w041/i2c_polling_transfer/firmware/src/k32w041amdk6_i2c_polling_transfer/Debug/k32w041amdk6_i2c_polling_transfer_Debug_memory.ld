/*
 * GENERATED FILE - DO NOT EDIT
 * Copyright 2008-2013 Code Red Technologies Ltd,
 * Copyright 2013-2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 * Generated linker script file for K32W041AM
 * Created from memory.ldt by FMCreateLinkMemory
 * Using Freemarker v2.3.30
 * MCUXpresso IDE v25.6 [Build 136] [2025-06-27] on May 16, 2026, 5:11:35 PM
 */

MEMORY
{
  /* Define each memory region */
  PROGRAM_FLASH (rx) : ORIGIN = 0x0, LENGTH = 0xa0000 /* 640K bytes (alias Flash) */  
  PROGRAM_FLASH_QSPI (rx) : ORIGIN = 0x10000000, LENGTH = 0x100000 /* 1M bytes (alias Flash2) */  
  SRAM (rwx) : ORIGIN = 0x4000000, LENGTH = 0x15fe0 /* 90080 bytes (alias RAM) */  
  SRAM1 (rwx) : ORIGIN = 0x4020000, LENGTH = 0x10000 /* 64K bytes (alias RAM2) */  
}

  /* Define a symbol for the top of each memory region */
  __base_PROGRAM_FLASH = 0x0  ; /* PROGRAM_FLASH */  
  __base_Flash = 0x0 ; /* Flash */  
  __top_PROGRAM_FLASH = 0x0 + 0xa0000 ; /* 640K bytes */  
  __top_Flash = 0x0 + 0xa0000 ; /* 640K bytes */  
  __base_PROGRAM_FLASH_QSPI = 0x10000000  ; /* PROGRAM_FLASH_QSPI */  
  __base_Flash2 = 0x10000000 ; /* Flash2 */  
  __top_PROGRAM_FLASH_QSPI = 0x10000000 + 0x100000 ; /* 1M bytes */  
  __top_Flash2 = 0x10000000 + 0x100000 ; /* 1M bytes */  
  __base_SRAM = 0x4000000  ; /* SRAM */  
  __base_RAM = 0x4000000 ; /* RAM */  
  __top_SRAM = 0x4000000 + 0x15fe0 ; /* 90080 bytes */  
  __top_RAM = 0x4000000 + 0x15fe0 ; /* 90080 bytes */  
  __base_SRAM1 = 0x4020000  ; /* SRAM1 */  
  __base_RAM2 = 0x4020000 ; /* RAM2 */  
  __top_SRAM1 = 0x4020000 + 0x10000 ; /* 64K bytes */  
  __top_RAM2 = 0x4020000 + 0x10000 ; /* 64K bytes */  
