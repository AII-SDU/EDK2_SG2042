/** @file
  Sophgo SG2042 Coreplex library definitions.

  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2023, 山东大学智能创新研究院（Academy of Intelligent Innovation）. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef SG2042_COREPLEX_H_
#define SG2042_COREPLEX_H_

#include <PiPei.h>

#include <SmbiosProcessorSpecificData.h>
#include <ProcessorSpecificHobData.h>

/**
  Build processor and platform information for the SG2042 platform

  @return EFI_SUCCESS     Status.

**/
EFI_STATUS
BuildRiscVSmbiosHobs (
  VOID
);

#endif
