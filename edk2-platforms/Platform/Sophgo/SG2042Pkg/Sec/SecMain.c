/** @file
  RISC-V SEC phase module for Qemu Virt.

  Copyright (c) 2008 - 2023, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2022, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SecMain.h"
#include <PiPei.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DebugPrintErrorLevelLib.h>
#include <Library/PlatformSecPpiLib.h>
#include <Library/PrintLib.h>
#include <Library/RiscVFirmwareContextLib.h>

#include <Ppi/TemporaryRamDone.h>
#include <Ppi/TemporaryRamSupport.h>

EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  );

EFI_STATUS
EFIAPI
TemporaryRamDone (
  VOID
  );

STATIC EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI mTemporaryRamSupportPpi = {
  TemporaryRamMigration
};

STATIC EFI_PEI_TEMPORARY_RAM_DONE_PPI mTemporaryRamDonePpi = {
  TemporaryRamDone
};

STATIC EFI_PEI_PPI_DESCRIPTOR mPrivateDispatchTable[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEfiTemporaryRamSupportPpiGuid,
    &mTemporaryRamSupportPpi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiTemporaryRamDonePpiGuid,
    &mTemporaryRamDonePpi
  },
};

/** Temporary RAM migration function.

  This function migrates the data from temporary RAM to permanent
  memory.

  @param[in]  PeiServices           PEI service
  @param[in]  TemporaryMemoryBase   Temporary memory base address
  @param[in]  PermanentMemoryBase   Permanent memory base address
  @param[in]  CopySize              Size to copy

**/
EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
  )
{
  VOID      *OldHeap;
  VOID      *NewHeap;
  VOID      *OldStack;
  VOID      *NewStack;
  EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT *FirmwareContext;

  DEBUG ((DEBUG_INFO,
    "%a: Temp Mem Base:0x%Lx, Permanent Mem Base:0x%Lx, CopySize:0x%Lx\n",
    __FUNCTION__,
    TemporaryMemoryBase,
    PermanentMemoryBase,
    (UINT64)CopySize
    ));

  OldHeap = (VOID*)(UINTN)TemporaryMemoryBase;
  NewHeap = (VOID*)((UINTN)PermanentMemoryBase + (CopySize >> 1));

  OldStack = (VOID*)((UINTN)TemporaryMemoryBase + (CopySize >> 1));
  NewStack = (VOID*)(UINTN)PermanentMemoryBase;

  CopyMem (NewHeap, OldHeap, CopySize >> 1);   // Migrate Heap
  CopyMem (NewStack, OldStack, CopySize >> 1); // Migrate Stack

  //
  // Reset firmware context pointer
  //
  GetFirmwareContextPointer (&FirmwareContext);
  FirmwareContext = (VOID *)FirmwareContext + (unsigned long)((UINTN)NewStack - (UINTN)OldStack);
  SetFirmwareContextPointer (FirmwareContext);

  //
  // Relocate PEI Service **
  //
  FirmwareContext->PeiServiceTable += (unsigned long)((UINTN)NewStack - (UINTN)OldStack);
  DEBUG ((DEBUG_INFO, "%a: OpenSBI Firmware Context is relocated to 0x%x\n", __FUNCTION__, FirmwareContext));
  DEBUG ((DEBUG_INFO, "OpenSBI Firmware Context at 0x%x\n", FirmwareContext));
  DEBUG ((DEBUG_INFO, "             PEI Service at 0x%x\n\n", FirmwareContext->PeiServiceTable));

  register uintptr_t a0 asm ("a0") = (uintptr_t)((UINTN)NewStack - (UINTN)OldStack);
  asm volatile ("add sp, sp, a0"::"r"(a0):);
  return EFI_SUCCESS;
}

/** Temprary RAM done function.

**/
EFI_STATUS EFIAPI TemporaryRamDone (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "%a: 2nd time PEI core, temporary ram done.\n", __FUNCTION__));
  return EFI_SUCCESS;
}

/**
  Locates a section within a series of sections
  with the specified section type.

  The Instance parameter indicates which instance of the section
  type to return. (0 is first instance, 1 is second...)

  @param[in]   Sections        The sections to search
  @param[in]   SizeOfSections  Total size of all sections
  @param[in]   SectionType     The section type to locate
  @param[in]   Instance        The section instance number
  @param[out]  FoundSection    The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindFfsSectionInstance (
  IN  VOID                       *Sections,
  IN  UINTN                      SizeOfSections,
  IN  EFI_SECTION_TYPE           SectionType,
  IN  UINTN                      Instance,
  OUT EFI_COMMON_SECTION_HEADER  **FoundSection
  )
{
  EFI_PHYSICAL_ADDRESS       CurrentAddress;
  UINT32                     Size;
  EFI_PHYSICAL_ADDRESS       EndOfSections;
  EFI_COMMON_SECTION_HEADER  *Section;
  EFI_PHYSICAL_ADDRESS       EndOfSection;

  //
  // Loop through the FFS file sections within the PEI Core FFS file
  //
  EndOfSection  = (EFI_PHYSICAL_ADDRESS)(UINTN)Sections;
  EndOfSections = EndOfSection + SizeOfSections;
  for ( ; ;) {
    if (EndOfSection == EndOfSections) {
      break;
    }

    CurrentAddress = (EndOfSection + 3) & ~(3ULL);
    if (CurrentAddress >= EndOfSections) {
      return EFI_VOLUME_CORRUPTED;
    }

    Section = (EFI_COMMON_SECTION_HEADER *)(UINTN)CurrentAddress;

    Size = SECTION_SIZE (Section);
    if (Size < sizeof (*Section)) {
      return EFI_VOLUME_CORRUPTED;
    }

    EndOfSection = CurrentAddress + Size;
    if (EndOfSection > EndOfSections) {
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Look for the requested section type
    //
    if (Section->Type == SectionType) {
      if (Instance == 0) {
        *FoundSection = Section;
        return EFI_SUCCESS;
      } else {
        Instance--;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Locates a section within a series of sections
  with the specified section type.

  @param[in]   Sections        The sections to search
  @param[in]   SizeOfSections  Total size of all sections
  @param[in]   SectionType     The section type to locate
  @param[out]  FoundSection    The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindFfsSectionInSections (
  IN  VOID                       *Sections,
  IN  UINTN                      SizeOfSections,
  IN  EFI_SECTION_TYPE           SectionType,
  OUT EFI_COMMON_SECTION_HEADER  **FoundSection
  )
{
  return FindFfsSectionInstance (
           Sections,
           SizeOfSections,
           SectionType,
           0,
           FoundSection
           );
}

/**
  Locates a FFS file with the specified file type and a section
  within that file with the specified section type.

  @param[in]   Fv            The firmware volume to search
  @param[in]   FileType      The file type to locate
  @param[in]   SectionType   The section type to locate
  @param[out]  FoundSection  The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindFfsFileAndSection (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *Fv,
  IN  EFI_FV_FILETYPE             FileType,
  IN  EFI_SECTION_TYPE            SectionType,
  OUT EFI_COMMON_SECTION_HEADER   **FoundSection
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  CurrentAddress;
  EFI_PHYSICAL_ADDRESS  EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER   *File;
  UINT32                Size;
  EFI_PHYSICAL_ADDRESS  EndOfFile;

  DEBUG ((DEBUG_INFO, "%a: DBT FV at 0x%x\n", __FUNCTION__, Fv));

  if (Fv->Signature != EFI_FVH_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "%a: FV at %p does not have FV header signature\n", __FUNCTION__, Fv));
    return EFI_VOLUME_CORRUPTED;
  }

  CurrentAddress      = (EFI_PHYSICAL_ADDRESS)(UINTN)Fv;
  EndOfFirmwareVolume = CurrentAddress + Fv->FvLength;

  //
  // Loop through the FFS files in the Boot Firmware Volume
  //
  for (EndOfFile = CurrentAddress + Fv->HeaderLength; ; ) {
    CurrentAddress = (EndOfFile + 7) & ~(7ULL);
    if (CurrentAddress > EndOfFirmwareVolume) {
      DEBUG ((DEBUG_ERROR, "%a: FV corrupted\n", __FUNCTION__));
      return EFI_VOLUME_CORRUPTED;
    }

    File = (EFI_FFS_FILE_HEADER *)(UINTN)CurrentAddress;
    Size = *(UINT32 *)File->Size & 0xffffff;
    if (Size < (sizeof (*File) + sizeof (EFI_COMMON_SECTION_HEADER))) {
      DEBUG ((DEBUG_ERROR, "%a: FV corrupted\n", __FUNCTION__));
      return EFI_VOLUME_CORRUPTED;
    }

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      DEBUG ((DEBUG_ERROR, "%a: FV corrupted\n", __FUNCTION__));
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Look for the request file type
    //
    if (File->Type != FileType) {
      DEBUG ((DEBUG_INFO, "%a: (File->Type != FileType), find next FFS\n", __FUNCTION__));
      continue;
    }

    Status = FindFfsSectionInSections (
               (VOID *)(File + 1),
               (UINTN)EndOfFile - (UINTN)(File + 1),
               SectionType,
               FoundSection
               );
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a: Get firmware file section\n", __FUNCTION__));
      return Status;
    }

    if (Status == EFI_VOLUME_CORRUPTED) {
      DEBUG ((DEBUG_ERROR, "%a: FV corrupted\n", __FUNCTION__));
      return Status;
    }

    DEBUG ((DEBUG_INFO, "%a: Find next FFS\n", __FUNCTION__));
  }
}

/**
  Locates the PEI Core entry point address.

  @param[in]  Fv                 The firmware volume to search
  @param[out] PeiCoreImageBase   The entry point of the PEI Core image

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindPeiCoreImageBaseInFv (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *Fv,
  OUT  EFI_PHYSICAL_ADDRESS       *PeiCoreImageBase
  )
{
  EFI_STATUS                 Status;
  EFI_COMMON_SECTION_HEADER  *Section;

  Status = FindFfsFileAndSection (
             Fv,
             EFI_FV_FILETYPE_PEI_CORE,
             EFI_SECTION_PE32,
             &Section
             );
  if (EFI_ERROR (Status)) {
    Status = FindFfsFileAndSection (
               Fv,
               EFI_FV_FILETYPE_PEI_CORE,
               EFI_SECTION_TE,
               &Section
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Unable to find PEI Core image\n", __FUNCTION__));
      return Status;
    }
  }

  DEBUG ((DEBUG_INFO, "%a: PeiCoreImageBase found\n", __FUNCTION__));
  *PeiCoreImageBase = (EFI_PHYSICAL_ADDRESS)(UINTN)(Section + 1);
  return EFI_SUCCESS;
}

/**
  Locates the PEI Core entry point address.

  @param[in,out]  BootFv             The firmware volume to search
  @param[out]     PeiCoreImageBase   The entry point of the PEI Core image

**/
VOID
FindPeiCoreImageBase (
  IN OUT  EFI_FIRMWARE_VOLUME_HEADER  **BootFv,
  OUT  EFI_PHYSICAL_ADDRESS           *PeiCoreImageBase
  )
{
  *PeiCoreImageBase = 0;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));
  FindPeiCoreImageBaseInFv (*BootFv, PeiCoreImageBase);
}

/**
  Find and return Pei Core entry point.

  It also find SEC and PEI Core file debug information. It will report them if
  remote debug is enabled.

  @param[in]  BootFirmwareVolumePtr   The firmware volume pointer to search
  @param[out] PeiCoreEntryPoint       The entry point of the PEI Core image


**/
VOID
FindAndReportEntryPoints (
  IN  EFI_FIRMWARE_VOLUME_HEADER  **BootFirmwareVolumePtr,
  OUT EFI_PEI_CORE_ENTRY_POINT    *PeiCoreEntryPoint
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PeiCoreImageBase;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

  FindPeiCoreImageBase (BootFirmwareVolumePtr, &PeiCoreImageBase);
  //
  // Find PEI Core entry point
  //
  Status = PeCoffLoaderGetEntryPoint ((VOID *)(UINTN)PeiCoreImageBase, (VOID **)PeiCoreEntryPoint);
  if (EFI_ERROR (Status)) {
    *PeiCoreEntryPoint = 0;
  }

  DEBUG ((DEBUG_INFO, "%a: PeCoffLoaderGetEntryPoint success: %x\n", __FUNCTION__, *PeiCoreEntryPoint));

  return;
}

/**

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.


  @param[in]  BootHartId         Hardware thread ID of boot hart.
  @param[in]  DeviceTreeAddress  Pointer to Device Tree (DTB)
**/
VOID
NORETURN
EFIAPI
SecStartup (
  IN  UINTN  BootHartId,
  IN  VOID   *DeviceTreeAddress
  )
{
  EFI_RISCV_OPENSBI_FIRMWARE_CONTEXT  FirmwareContext;
  EFI_SEC_PEI_HAND_OFF                SecCoreData;
  EFI_PEI_CORE_ENTRY_POINT            PeiCoreEntryPoint;
  EFI_FIRMWARE_VOLUME_HEADER          *BootFv;

  SerialPortInitialize ();

  //
  // Report Status Code to indicate entering SEC core
  //
  DEBUG ((
    DEBUG_INFO, "%a() BootHartId: 0x%x, DeviceTreeAddress=0x%x\n",
    __func__, BootHartId, DeviceTreeAddress
    ));

  FirmwareContext.BootHartId          = BootHartId;
  FirmwareContext.FlattenedDeviceTree = (UINT64)DeviceTreeAddress;
  SetFirmwareContextPointer (&FirmwareContext);

  BootFv = (EFI_FIRMWARE_VOLUME_HEADER *)FixedPcdGet32 (PcdRiscVPeiFvBase);
  FindAndReportEntryPoints (&BootFv, &PeiCoreEntryPoint);

  SecCoreData.DataSize               = sizeof (EFI_SEC_PEI_HAND_OFF);
  SecCoreData.BootFirmwareVolumeBase = BootFv;
  SecCoreData.BootFirmwareVolumeSize = (UINTN)BootFv->FvLength;
  SecCoreData.TemporaryRamBase       = (VOID *)(UINT64)FixedPcdGet32 (PcdTemporaryRamBase);
  SecCoreData.TemporaryRamSize       = (UINTN)FixedPcdGet32 (PcdTemporaryRamSize);
  SecCoreData.PeiTemporaryRamBase    = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize    = SecCoreData.TemporaryRamSize >> 1;
  SecCoreData.StackBase              = (UINT8 *)SecCoreData.TemporaryRamBase + (SecCoreData.TemporaryRamSize >> 1);
  SecCoreData.StackSize              = SecCoreData.TemporaryRamSize >> 1;


  (*PeiCoreEntryPoint)(&SecCoreData,(EFI_PEI_PPI_DESCRIPTOR *)&mPrivateDispatchTable);
  //
  // Should not come here.
  //
  UNREACHABLE ();
}
