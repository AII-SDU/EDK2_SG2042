/**@file

  Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  Module Name:

    FvbInfo.c

  Abstract:

    Defines data structure that is the volume header found.These data is intent
    to decouple FVB driver with FV header.

**/

//
// The package level header files this module uses
//
#include <Pi/PiFirmwareVolume.h>

//
// The protocols, PPI and GUID defintions for this module
//
#include <Guid/SystemNvDataGuid.h>
//
// The Library classes this module consumes
//
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>

typedef struct {
  UINT64                      FvLength;
  EFI_FIRMWARE_VOLUME_HEADER  FvbInfo;
  //
  // EFI_FV_BLOCK_MAP_ENTRY    ExtraBlockMap[n];//n=0
  //
  EFI_FV_BLOCK_MAP_ENTRY      End[1];
} EFI_FVB_MEDIA_INFO;

EFI_FVB_MEDIA_INFO  mPlatformFvbMediaInfo[] = {
  //
  // Systen NvStorage FVB
  //
  {
    FixedPcdGet32 (PcdFlashNvStorageVariableSize) +
    FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) +
    FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize),
    {
      {
        0,
      },  // ZeroVector[16]
      EFI_SYSTEM_NV_DATA_FV_GUID,
      FixedPcdGet32 (PcdFlashNvStorageVariableSize) +
      FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) +
      FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize),
      EFI_FVH_SIGNATURE,
      EFI_FVB2_MEMORY_MAPPED |
        EFI_FVB2_READ_ENABLED_CAP |
        EFI_FVB2_READ_STATUS |
        EFI_FVB2_WRITE_ENABLED_CAP |
        EFI_FVB2_WRITE_STATUS |
        EFI_FVB2_ERASE_POLARITY |
        EFI_FVB2_ALIGNMENT_16,
      sizeof (EFI_FIRMWARE_VOLUME_HEADER) + sizeof (EFI_FV_BLOCK_MAP_ENTRY),
      0,  // CheckSum
      0,  // ExtHeaderOffset
      {
        0,
      },  // Reserved[1]
      2,  // Revision
      {
        {
          (FixedPcdGet32 (PcdFlashNvStorageVariableSize) +
           FixedPcdGet32 (PcdFlashNvStorageFtwWorkingSize) +
           FixedPcdGet32 (PcdFlashNvStorageFtwSpareSize)) /
          FixedPcdGet32 (PcdVariableFdBlockSize),
          FixedPcdGet32 (PcdVariableFdBlockSize),
        }
      } // BlockMap[1]
    },
    {
      {
        0,
        0
      }
    }  // End[1]
  }
};

/**
  Get the firmware volume information for a given firmware volume length.

  @param[in]      FvLength              Length of the firmware volume.
  @param[out]     FvbInfo               Pointer to the firmware volume information.

  @retval EFI_SUCCESS                   The firmware volume information was retrieved successfully.
  @retval EFI_NOT_FOUND                 The firmware volume information for the given length was not found.
  @retval Other                         An error occurred while retrieving the firmware volume information.

**/
EFI_STATUS
GetFvbInfo (
  IN  UINT64                        FvLength,
  OUT EFI_FIRMWARE_VOLUME_HEADER    **FvbInfo
  )
{
  STATIC BOOLEAN Checksummed = FALSE;
  UINTN Index;

  if (!Checksummed) {
    for (Index = 0;
         Index < sizeof (mPlatformFvbMediaInfo) / sizeof (EFI_FVB_MEDIA_INFO);
         Index += 1) {
      UINT16 Checksum;
      mPlatformFvbMediaInfo[Index].FvbInfo.Checksum = 0;
      Checksum = CalculateCheckSum16 (
                   (UINT16*) &mPlatformFvbMediaInfo[Index].FvbInfo,
                   mPlatformFvbMediaInfo[Index].FvbInfo.HeaderLength
                   );
      mPlatformFvbMediaInfo[Index].FvbInfo.Checksum = Checksum;
    }
    Checksummed = TRUE;
  }

  for (Index = 0;
       Index < sizeof (mPlatformFvbMediaInfo) / sizeof (EFI_FVB_MEDIA_INFO);
       Index += 1) {
    if (mPlatformFvbMediaInfo[Index].FvLength == FvLength) {
      *FvbInfo = &mPlatformFvbMediaInfo[Index].FvbInfo;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
