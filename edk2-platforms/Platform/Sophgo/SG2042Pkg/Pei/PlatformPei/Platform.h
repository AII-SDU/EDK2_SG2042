/** @file
  Platform PEI module include file.

  Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PLATFORM_PEI_H_INCLUDED_
#define PLATFORM_PEI_H_INCLUDED_

VOID
AddIoMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  );

VOID
AddIoMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  EFI_PHYSICAL_ADDRESS  MemoryLimit
  );

VOID
AddMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  );

VOID
AddMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  EFI_PHYSICAL_ADDRESS  MemoryLimit
  );

VOID
AddUntestedMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  );

VOID
AddReservedMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  );

VOID
AddUntestedMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  EFI_PHYSICAL_ADDRESS  MemoryLimit
  );

VOID
AddressWidthInitialization (
  VOID
  );

EFI_STATUS
PublishPeiMemory (
  IN EFI_PHYSICAL_ADDRESS  MemoryBase,
  IN UINT64                MemorySize
  );

UINT32
GetSystemMemorySizeBelow4gb (
  VOID
  );

VOID
InitializeRamRegions (
  IN EFI_PHYSICAL_ADDRESS  SystemMemoryBase,
  IN UINT64                SystemMemorySize
  );

/**
  Perform Memory PEIM initialization.

  @return EFI_SUCCESS     The platform initialized successfully.
  @retval  Others        - As the error code indicates

**/
EFI_STATUS
EFIAPI
MemoryPeimInitialization (
  VOID
  );

EFI_STATUS
PeiFvInitialization (
  VOID
  );

EFI_STATUS
InitializeXen (
  VOID
  );

/**
  Build processor and platform information for the U5 platform

  @return EFI_SUCCESS     Status.

**/
EFI_STATUS
BuildRiscVSmbiosHobs (
  VOID
  );

#endif // _PLATFORM_PEI_H_INCLUDED_
