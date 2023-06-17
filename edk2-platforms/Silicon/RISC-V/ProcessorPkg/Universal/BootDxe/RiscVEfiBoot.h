/** @file
 RiscV Efi Boot protocol.

Copyright (c) 2023, AII-SDU. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __RISCV_EFI_BOOT_PROTOCOL_H__
#define __RISCV_EFI_BOOT_PROTOCOL_H__

#define RISCV_EFI_BOOT_PROTOCOL_GUID \
  { \
    0xCCD15FEC, 0x6F73, 0x4EEC, {0x83, 0x95, 0x3E, 0x69, 0xE4, 0xB9, 0x40, 0xBF } \
  }


typedef struct _RISCV_EFI_BOOT_PROTOCOL RISCV_EFI_BOOT_PROTOCOL;


/**
  Causes the driver to get boot hartid.

  @param  This       Protocol instance pointer.
  @param  BootHartId Boot hartid. 

  @retval EFI_SUCCESS           The file was loaded.
  @retval EFI_UNSUPPORTED       The device does not support the provided BootPolicy
  @retval EFI_INVALID_PARAMETER FilePath is not a valid device path, or
                                BufferSize is NULL.
  @retval EFI_NO_MEDIA          No medium was present to load the file.
  @retval EFI_DEVICE_ERROR      The file was not loaded due to a device error.
  @retval EFI_NO_RESPONSE       The remote system did not respond.
  @retval EFI_NOT_FOUND         The file was not found.
  @retval EFI_ABORTED           The file load process was manually cancelled.
  @retval EFI_WARN_FILE_SYSTEM  The resulting Buffer contains UEFI-compliant file system.
**/
typedef
EFI_STATUS
(EFIAPI *GET_BOOT_HARTID)(
  IN RISCV_EFI_BOOT_PROTOCOL           *This,
  IN OUT UINTN                        *BootHartId
  );

///
/// The RISCV_EFI_BOOT_PROTOCOL is a simple protocol used to get boot hartid from pcd value.
///
struct _RISCV_EFI_BOOT_PROTOCOL {
  UINTN revision;
  GET_BOOT_HARTID    GetBootHartId;
};

extern EFI_GUID  gRiscVEfiBootProtocolGuid;

#endif