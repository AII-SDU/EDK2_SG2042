/** @file
  Ram flash device for EFI variable

  Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/UefiRuntimeLib.h>

#include "RamFlash.h"

/**
  Convert the pointers from RAM to Flash memory.

**/
VOID
RamFlashConvertPointers (
  VOID
  )
{
  EfiConvertPointer (0x0, (VOID **) &mFlashBase);
}
