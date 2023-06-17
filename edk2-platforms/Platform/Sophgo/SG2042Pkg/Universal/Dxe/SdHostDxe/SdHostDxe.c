/** @file
 *
 *  Copyright (c) 2017, Andrei Warkentin <andrey.warkentin@gmail.com>
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 **/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DmaLib.h>
#include <Library/TimerLib.h>

#include <Protocol/EmbeddedExternalDevice.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>

#include "Sdhci.h"

#define SDHOST_BLOCK_BYTE_LENGTH            512

#define DEBUG_MMCHOST_SD       DEBUG_VERBOSE
#define DEBUG_MMCHOST_SD_INFO  DEBUG_INFO
#define DEBUG_MMCHOST_SD_ERROR DEBUG_ERROR


STATIC BOOLEAN mCardIsPresent = FALSE;
STATIC CARD_DETECT_STATE mCardDetectState = CardDetectRequired;


STATIC BOOLEAN
SdIsReadOnly (
  IN EFI_MMC_HOST_PROTOCOL *This
  )
{
  return FALSE;
}

STATIC EFI_STATUS
SdBuildDevicePath (
  IN EFI_MMC_HOST_PROTOCOL       *This,
  IN EFI_DEVICE_PATH_PROTOCOL    **DevicePath
  )
{
  EFI_DEVICE_PATH_PROTOCOL *NewDevicePathNode;
  EFI_GUID DevicePathGuid = EFI_CALLER_ID_GUID;

  DEBUG ((DEBUG_MMCHOST_SD, "SdHost: SdBuildDevicePath()\n"));

  NewDevicePathNode = CreateDeviceNode (HARDWARE_DEVICE_PATH, HW_VENDOR_DP, sizeof (VENDOR_DEVICE_PATH));
  CopyGuid (&((VENDOR_DEVICE_PATH*)NewDevicePathNode)->Guid, &DevicePathGuid);
  *DevicePath = NewDevicePathNode;

  return EFI_SUCCESS;
}

STATIC EFI_STATUS
SdSendCommand (
  IN EFI_MMC_HOST_PROTOCOL    *This,
  IN MMC_CMD                  MmcCmd,
  IN UINT32                   Argument,
  IN MMC_RESPONSE_TYPE        Type,
  IN UINT32*                  Buffer
  )
{
  EFI_STATUS Status = EFI_SUCCESS;

  Status = bm_sd_send_cmd(MmcCmd, Argument, Type, Buffer);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdSendCommand Error, Status=%r.\n", Status));
    return Status;
  }

  return Status;
}

STATIC EFI_STATUS
SdReadBlockData (
  IN EFI_MMC_HOST_PROTOCOL    *This,
  IN EFI_LBA                  Lba,
  IN UINTN                    Length,
  IN UINT32*                  Buffer
  )
{
  //DEBUG ((DEBUG_MMCHOST_SD_INFO, "SdHost: SdReadBlockData(LBA: 0x%x, Length: 0x%x, Buffer: 0x%x)\n",(UINT32)Lba, Length, Buffer));

  ASSERT (Buffer != NULL);
  ASSERT (Length % 4 == 0);

  EFI_STATUS Status = EFI_SUCCESS;

  Status = bm_sd_read(Lba, Buffer, Length);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdReadBlockData Error, Status=%r.\n", Status));
    return Status;
  }

  return Status;
}

STATIC EFI_STATUS
SdWriteBlockData (
  IN EFI_MMC_HOST_PROTOCOL    *This,
  IN EFI_LBA                  Lba,
  IN UINTN                    Length,
  IN UINT32*                  Buffer
  )
{
  DEBUG ((DEBUG_MMCHOST_SD_INFO, "SdHost: SdWriteBlockData(LBA: 0x%x, Length: 0x%x, Buffer: 0x%x)\n",(UINT32)Lba, Length, Buffer));

  ASSERT (Buffer != NULL);
  ASSERT (Length % SDHOST_BLOCK_BYTE_LENGTH == 0);

  EFI_STATUS Status = EFI_SUCCESS;

  Status = bm_sd_write(Lba, Buffer, Length);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdWriteBlockData Error, Status=%r.\n", Status));
    return Status;
  }

  return Status;
}

STATIC EFI_STATUS
SdSetIos (
  IN EFI_MMC_HOST_PROTOCOL      *This,
  IN  UINT32                    BusClockFreq,
  IN  UINT32                    BusWidth
  )
{
  DEBUG ((DEBUG_MMCHOST_SD_INFO, "Setting Freq %u Hz\n", BusClockFreq));
  DEBUG ((DEBUG_MMCHOST_SD_INFO, "Setting BusWidth %u\n", BusWidth));

  EFI_STATUS Status = EFI_SUCCESS;

  Status = bm_sd_set_ios(BusClockFreq,BusWidth);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdSetIos Error, Status=%r.\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC EFI_STATUS
SdPrepare (
  IN EFI_MMC_HOST_PROTOCOL    *This,
  IN EFI_LBA                  Lba,
  IN UINTN                    Length,
  IN UINTN                    Buffer
  )
{
  //DEBUG ((DEBUG_MMCHOST_SD_INFO, "SdHost: SdPrepare(LBA: 0x%x, Length: 0x%x, Buffer: 0x%x)\n",(UINT32)Lba, Length, Buffer));

  EFI_STATUS Status = EFI_SUCCESS;

  Status = bm_sd_prepare(Lba, Buffer, Length);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "SdPrepare Error, Status=%r.\n", Status));
    return Status;
  }

  return EFI_SUCCESS;
}

STATIC EFI_STATUS
SdNotifyState (
  IN EFI_MMC_HOST_PROTOCOL    *This,
  IN MMC_STATE                State
  )
{

  // Stall all operations except init until card detection has occurred.
  if (State != MmcHwInitializationState && mCardDetectState != CardDetectCompleted) {
    return EFI_NOT_READY;
  }

  switch (State) {
  case MmcHwInitializationState:
    DEBUG ((DEBUG_MMCHOST_SD_INFO, "MmcHwInitializationState\n", State));

    EFI_STATUS Status = SdInit (SD_USE_PIO);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_MMCHOST_SD_ERROR,"SdHost: SdNotifyState(): Fail to initialize!\n"));
      return Status;
    }
    break;
  case MmcIdleState:
    DEBUG ((DEBUG_MMCHOST_SD, "MmcIdleState\n", State));
    break;
  case MmcReadyState:
    DEBUG ((DEBUG_MMCHOST_SD, "MmcReadyState\n", State));
    break;
  case MmcIdentificationState:
    DEBUG ((DEBUG_MMCHOST_SD, "MmcIdentificationState\n", State));
    break;
  case MmcStandByState:
    DEBUG ((DEBUG_MMCHOST_SD, "MmcStandByState\n", State));
    break;
  case MmcTransferState:
    DEBUG ((DEBUG_MMCHOST_SD, "MmcTransferState\n", State));
    break;
  case MmcSendingDataState:
    DEBUG ((DEBUG_MMCHOST_SD, "MmcSendingDataState\n", State));
    break;
  case MmcReceiveDataState:
    DEBUG ((DEBUG_MMCHOST_SD, "MmcReceiveDataState\n", State));
    break;
  case MmcProgrammingState:
    DEBUG ((DEBUG_MMCHOST_SD, "MmcProgrammingState\n", State));
    break;
  case MmcDisconnectState:
  case MmcInvalidState:
  default:
    DEBUG ((DEBUG_MMCHOST_SD_ERROR, "SdHost: SdNotifyState(): Invalid State: %d\n", State));
    ASSERT (0);
  }

  return EFI_SUCCESS;
}

STATIC BOOLEAN
SdIsCardPresent (
  IN EFI_MMC_HOST_PROTOCOL *This
  )
{
  //
  // If we are already in progress (we may get concurrent calls)
  // or completed the detection, just return the current value.
  //
  if (mCardDetectState != CardDetectRequired) {
    return mCardIsPresent;
  }

  mCardDetectState = CardDetectInProgress;
  mCardIsPresent = FALSE;

  if (bm_sd_card_detect() == 1) {
    mCardIsPresent = TRUE;
    goto out;
  }
  else {
    DEBUG ((DEBUG_ERROR, "SdIsCardPresent: Error bm_sd_card_detect.\n"));
    mCardDetectState = CardDetectRequired;
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "SdIsCardPresent: Not detected.\n"));

out:
  mCardDetectState = CardDetectCompleted;
  return mCardIsPresent;
}

BOOLEAN
SdIsMultiBlock (
  IN EFI_MMC_HOST_PROTOCOL *This
  )
{
  return TRUE;
}

EFI_MMC_HOST_PROTOCOL gMmcHost =
  {
    MMC_HOST_PROTOCOL_REVISION,
    SdIsCardPresent,
    SdIsReadOnly,
    SdBuildDevicePath,
    SdNotifyState,
    SdSendCommand,
    SdReadBlockData,
    SdWriteBlockData,
    SdSetIos,
    SdPrepare,
    SdIsMultiBlock
  };

EFI_STATUS
SdHostInitialize (
  IN EFI_HANDLE          ImageHandle,
  IN EFI_SYSTEM_TABLE    *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_HANDLE Handle = NULL;

  DEBUG ((DEBUG_MMCHOST_SD, "SdHost: Initialize\n"));

  Status = gBS->InstallMultipleProtocolInterfaces (
    &Handle,
    &gSophgoMmcHostProtocolGuid,
    &gMmcHost,
    NULL
  );
  ASSERT_EFI_ERROR (Status);
  return Status;
}
