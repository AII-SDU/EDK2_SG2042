/** @file
  RISC-V Timer Architectural definition for U500 platform.

  Copyright (c) 2019, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef SG2042_CLINT_H_
#define SG2042_CLINT_H_

#define CLINT_REG_BASE_ADDR 0x70ac000000
    #define CLINT_REG_MTIME     0x70ac00BFF8
    #define CLINT_REG_MTIMECMP0 0x70ac004000

#endif
