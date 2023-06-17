/** @file
  RISC-V EFI Boot DXE module.

  Copyright (c) 2023, AII-SDU. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PcdLib.h>
#include <Library/IoLib.h>
#include <Library/RiscVFirmwareContextLib.h>

#include <RiscVEfiBoot.h>

/**
  Fet boot hartid.

  @param This     Instance pointer for this protocol
  @param BootHartId  boot hartid.

  @retval EFI_SUCCESS       InterruptState is valid
  @retval EFI_DEVICE_ERROR  InterruptState is not valid

**/
EFI_STATUS
EFIAPI
GetBootHartId (
  IN RISCV_EFI_BOOT_PROTOCOL           *This,
  IN OUT UINTN                        *BootHartId
  )
{
    EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT  *FirmwareContext;

    GetFirmwareContextPointer (&FirmwareContext);

    *BootHartId = FirmwareContext->BootHartId;

    DEBUG ((
    DEBUG_INFO,
    "Riscv efi boot with boot hart id: %d\n",
    *BootHartId
    ));

    return EFI_SUCCESS;
}


EFI_HANDLE  gRiscVEfiBootHandle = NULL;

//
// The protocol instance produced by this driver
//
RISCV_EFI_BOOT_PROTOCOL gRiscVEfiBootProtocol = {
  0x10000,
  GetBootHartId
};

/**
  Get the Boot information for the RISCV EFI BOOT Protocol

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
BootDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  // Make sure the Interrupt Controller Protocol is not already installed in the system.
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gRiscVEfiBootProtocolGuid);

  Status = gBS->InstallMultipleProtocolInterfaces(&gRiscVEfiBootHandle,
                                                  &gRiscVEfiBootProtocolGuid,   &gRiscVEfiBootProtocol,
                                                  NULL);
  ASSERT_EFI_ERROR(Status);

  return Status;
}

