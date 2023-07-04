/**@file
  Build up platform processor information.

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/BaseMemoryLib.h>

#include <SmbiosProcessorSpecificData.h>
#include <ProcessorSpecificHobData.h>
#include <SG2042Coreplex.h>
#include <Library/SophgoC920.h>

/**
  Function to build processor related SMBIOS information. RISC-V SMBIOS DXE driver collect
  this information and build SMBIOS Type4 and Type7 record.

  @param  ProcessorUid    Unique ID of physical processor which owns this core.
  @param  SmbiosHobPtr    Pointer to receive RISC_V_PROCESSOR_SMBIOS_HOB_DATA. The pointers
                          maintained in this structure is only valid before memory is discovered.
                          Access to those pointers after memory is installed will cause unexpected issues.

  @return EFI_SUCCESS     The SMBIOS Hobs were created successfully.

**/
STATIC
EFI_STATUS
EFIAPI
CreateSG2042ProcessorSmbiosDataHob (
  IN UINTN     ProcessorUid,
  OUT RISC_V_PROCESSOR_SMBIOS_HOB_DATA **SmbiosHobPtr
  )
{
  EFI_GUID *GuidPtr;
  RISC_V_PROCESSOR_TYPE7_HOB_DATA L2CacheDataHob;
  RISC_V_PROCESSOR_SMBIOS_HOB_DATA SmbiosDataHob;
  RISC_V_PROCESSOR_TYPE4_HOB_DATA *ProcessorDataHobPtr;
  RISC_V_PROCESSOR_TYPE7_HOB_DATA *L1CacheDataHobPtr;
  RISC_V_PROCESSOR_TYPE7_HOB_DATA *L2CacheDataHobPtr;
  RISC_V_PROCESSOR_SMBIOS_HOB_DATA *SmbiosDataHobPtr;

  DEBUG ((DEBUG_INFO, "%a: Entry\n", __FUNCTION__));

  if (SmbiosHobPtr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  CreateC920SmbiosType7L1DataHob (ProcessorUid, &L1CacheDataHobPtr);
  CreateC920SmbiosType4DataHob (ProcessorUid, &ProcessorDataHobPtr);

  //
  // Build up SMBIOS type 7 L2 cache record.
  //
  ZeroMem((VOID *)&L2CacheDataHob, sizeof (RISC_V_PROCESSOR_TYPE7_HOB_DATA));
  L2CacheDataHob.ProcessorGuid = *((EFI_GUID *)PcdGetPtr (PcdSophgoSG2042CoreplexGuid));
  L2CacheDataHob.ProcessorUid = ProcessorUid;
  L2CacheDataHob.SmbiosType7Cache.SocketDesignation = TO_BE_FILLED_BY_VENDOR;
  L2CacheDataHob.SmbiosType7Cache.CacheConfiguration = RISC_V_CACHE_CONFIGURATION_CACHE_LEVEL_2 | \
      RISC_V_CACHE_CONFIGURATION_LOCATION_EXTERNAL | \
      RISC_V_CACHE_CONFIGURATION_ENABLED | \
      RISC_V_CACHE_CONFIGURATION_MODE_UNKNOWN;
  L2CacheDataHob.SmbiosType7Cache.MaximumCacheSize = TO_BE_FILLED_BY_VENDOR;
  L2CacheDataHob.SmbiosType7Cache.InstalledSize = TO_BE_FILLED_BY_VENDOR;
  L2CacheDataHob.SmbiosType7Cache.SupportedSRAMType.Unknown = 1;
  L2CacheDataHob.SmbiosType7Cache.CurrentSRAMType.Unknown = 1;
  L2CacheDataHob.SmbiosType7Cache.CacheSpeed = TO_BE_FILLED_BY_VENDOR;
  L2CacheDataHob.SmbiosType7Cache.ErrorCorrectionType = TO_BE_FILLED_BY_VENDOR;
  L2CacheDataHob.SmbiosType7Cache.SystemCacheType = CacheTypeUnified;
  L2CacheDataHob.SmbiosType7Cache.Associativity = TO_BE_FILLED_BY_VENDOR;
  GuidPtr = (EFI_GUID *)PcdGetPtr (PcdProcessorSmbiosType7GuidHobGuid);
  L2CacheDataHobPtr = (RISC_V_PROCESSOR_TYPE7_HOB_DATA *)BuildGuidDataHob (GuidPtr, (VOID *)&L2CacheDataHob, sizeof (RISC_V_PROCESSOR_TYPE7_HOB_DATA));
  if (L2CacheDataHobPtr == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to create GUID HOB of Sophgo SG2042 Coreplex L2 cache RISC_V_PROCESSOR_TYPE7_HOB_DATA.\n"));
    ASSERT (FALSE);
  }

  ZeroMem((VOID *)&SmbiosDataHob, sizeof (RISC_V_PROCESSOR_SMBIOS_HOB_DATA));
  SmbiosDataHob.Processor = ProcessorDataHobPtr;
  SmbiosDataHob.L1Cache = L1CacheDataHobPtr;
  SmbiosDataHob.L2Cache = L2CacheDataHobPtr;
  SmbiosDataHob.L3Cache = NULL;

  GuidPtr = (EFI_GUID *)PcdGetPtr (PcdProcessorSmbiosGuidHobGuid);
  SmbiosDataHobPtr = (RISC_V_PROCESSOR_SMBIOS_HOB_DATA *)BuildGuidDataHob (GuidPtr, (VOID *)&SmbiosDataHob, sizeof (RISC_V_PROCESSOR_SMBIOS_HOB_DATA));
  if (SmbiosDataHobPtr == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to create GUID HOB of Sophgo SG2042 Coreplex RISC_V_PROCESSOR_SMBIOS_HOB_DATA.\n"));
    ASSERT (FALSE);
  }
  *SmbiosHobPtr = SmbiosDataHobPtr;
  DEBUG ((DEBUG_INFO, "%a: Exit\n", __FUNCTION__));

  return EFI_SUCCESS;
}

/**
  Build processor and platform information for the U5 platform

  @return EFI_SUCCESS     Status.

**/
EFI_STATUS
BuildRiscVSmbiosHobs (
  VOID
)
{
  EFI_STATUS Status;
  RISC_V_PROCESSOR_SMBIOS_HOB_DATA *SmbiosHobPtr;

  Status = CreateSG2042ProcessorSmbiosDataHob (0, &SmbiosHobPtr);
  if (EFI_ERROR (Status)) {
    ASSERT(FALSE);
  }

  DEBUG ((DEBUG_INFO, "SG2042 Coreplex SMBIOS DATA HOB at address 0x%x\n", SmbiosHobPtr));

  return EFI_SUCCESS;
}
