/** @file
  Sophgo c920 Core library definitions.

  Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2023, Academy of Intelligent Innovation. All rights reserved.<BR>
  
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef SOPHGO_C920_CORE_H_
#define SOPHGO_C920_CORE_H_

#include <PiPei.h>

#include <ProcessorSpecificHobData.h>


/**
  Function to build cache related SMBIOS information. RISC-V SMBIOS DXE driver collects
  this information and builds SMBIOS Type 7 record.

  The caller can adjust the allocated hob data to their needs.

  @param  ProcessorUid      Unique ID of physical processor which owns this core.
  @param  L1CacheDataHobPtr Pointer to allocated HOB data.

**/
VOID
EFIAPI
CreateC920SmbiosType7L1DataHob (
  IN UINTN                             ProcessorUid,
  OUT RISC_V_PROCESSOR_TYPE7_HOB_DATA  **L1CacheDataHobPtr
);

/**
  Function to build processor related SMBIOS information. RISC-V SMBIOS DXE driver collects
  this information and builds SMBIOS Type 4 record.

  The caller can adjust the allocated hob data to their needs.

  @param  ProcessorUid      Unique ID of physical processor which owns this core.
  @param  ProcessorDataHobPtr Pointer to allocated HOB data.

**/
VOID
EFIAPI
CreateC920SmbiosType4DataHob (
  IN UINTN                             ProcessorUid,
  OUT RISC_V_PROCESSOR_TYPE4_HOB_DATA  **ProcessorDataHobPtr
);

#endif
