/** @file SpmiResetDxe.c
  SPMI PMIC Arbiter probe for the mars/m2468 chain (compact, crash-dump friendly).

  Why this shape
  --------------
  Two earlier attempts to get the data off the device both failed:

    1. Writing \EFI\spmi.txt on the ESP needs SimpleFileSystem -> FAT -> DiskIo
       -> UFSDxe -> UFS -> PMIC power -> PMIC init -> working SPMI.  Circular.
       The driver never dispatched because SimpleFileSystem was never installed.

    2. Depex = CpuArch dispatches early, but FAT is not up yet, so the write is
       skipped and DEBUG() output stays in the Qcom debug buffer -- which is only
       drained to the panel by the crash handler.

  Mu does crash later, at ButtonsDxe (~0xC3C8C5AC) because PmicGpioProtocol is
  missing.  The crash handler dumps the debug buffer, so if we log through
  DEBUG() and keep the output small (< 1 KB), the arbiter state will come out
  with the crash dump.

  Therefore: Depex = TRUE (dispatch as early as possible), DEBUG only, and only
  the 20 registers that actually decide "who owns the channels".

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/BaseLib.h>

#define SPMI_CORE   0x0C400000ULL
#define SPMI_CHNLS  0x0C500000ULL
#define SPMI_CNFG   0x0C42D000ULL

EFI_STATUS
EFIAPI
SpmiResetDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT32  Ch;
  UINT32  Off;

  DEBUG ((DEBUG_ERROR, "\n[SPMI] === probe begin ===\n"));

  //
  // core: VERSION proves the block is reachable; 0x8/0xC are the int/status
  // words that a stuck arbiter would light up.
  //
  DEBUG ((DEBUG_ERROR, "[SPMI] core VER  = %08x\n", MmioRead32 ((UINTN)(SPMI_CORE + 0x000))));
  DEBUG ((DEBUG_ERROR, "[SPMI] core 008  = %08x\n", MmioRead32 ((UINTN)(SPMI_CORE + 0x008))));
  DEBUG ((DEBUG_ERROR, "[SPMI] core 00C  = %08x\n", MmioRead32 ((UINTN)(SPMI_CORE + 0x00C))));

  //
  // cnfg: the five EE config words SPMIDxe reads (0xE60..0xE70).
  //
  DEBUG ((DEBUG_ERROR, "[SPMI] EE E60    = %08x\n", MmioRead32 ((UINTN)(SPMI_CNFG + 0xE60))));
  DEBUG ((DEBUG_ERROR, "[SPMI] EE E64    = %08x\n", MmioRead32 ((UINTN)(SPMI_CNFG + 0xE64))));
  DEBUG ((DEBUG_ERROR, "[SPMI] EE E68    = %08x\n", MmioRead32 ((UINTN)(SPMI_CNFG + 0xE68))));
  DEBUG ((DEBUG_ERROR, "[SPMI] EE E6C    = %08x\n", MmioRead32 ((UINTN)(SPMI_CNFG + 0xE6C))));
  DEBUG ((DEBUG_ERROR, "[SPMI] EE E70    = %08x\n", MmioRead32 ((UINTN)(SPMI_CNFG + 0xE70))));

  //
  // channels 0..3: CMD / STATUS decide whether something is pending.
  //
  for (Ch = 0; Ch < 4; Ch++) {
    Off = Ch * 0x1000;
    DEBUG ((DEBUG_ERROR, "[SPMI] ch%d cmd=%08x sts=%08x%s\n",
            Ch,
            MmioRead32 ((UINTN)(SPMI_CHNLS + Off + 0x00)),
            MmioRead32 ((UINTN)(SPMI_CHNLS + Off + 0x08)),
            (MmioRead32 ((UINTN)(SPMI_CHNLS + Off + 0x08)) & 1) ? " BUSY" : ""));
  }

  DEBUG ((DEBUG_ERROR, "[SPMI] === probe end ===\n\n"));

  return EFI_SUCCESS;
}
