## @file
#  Definitions of Flash definition file on SiFive Freedom U540 HiFive Unleashed RISC-V platform
#
#  Copyright (c) 2021, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
[Defines]
DEFINE BLOCK_SIZE        = 0x1000

DEFINE FW_BASE_ADDRESS   = 0x02000000
DEFINE FW_SIZE           = 0x00800000
DEFINE FW_BLOCKS         = 0x800

#
# 0x000000-0x7DFFFF code
# 0x7E0000-0x800000 variables
#
DEFINE CODE_BASE_ADDRESS = 0x02000000
DEFINE CODE_SIZE         = 0x00700000
DEFINE CODE_BLOCKS       = 0x700
DEFINE VARS_BLOCKS       = 0x20

#
# SEC + opensbi library is the root FW domain.
# The base address must be round up to log2.
#
DEFINE SECFV_OFFSET        = 0x00000000
DEFINE SECFV_SIZE          = 0x00040000
DEFINE ROOT_FW_DOMAIN_SIZE = $(SECFV_SIZE)

#
# Other FV regions are in the second FW domain.
# The size of memory region must be power of 2.
# The base address must be aligned with the size.
#
# FW memory region
#
DEFINE PEIFV_OFFSET                  = 0x00400000
DEFINE PEIFV_SIZE                    = 0x00080000
DEFINE FVMAIN_OFFSET                 = 0x00480000
DEFINE FVMAIN_SIZE                   = 0x00280000

#
# EFI Variable memory region.
# The total size of EFI Variable FD must include
# all of sub regions of EFI Variable
#
DEFINE VARS_OFFSET                   = 0x00700000
DEFINE VARS_SIZE                     = 0x00007000
DEFINE VARS_FTW_WORKING_OFFSET       = 0x00707000
DEFINE VARS_FTW_WORKING_SIZE         = 0x00001000
DEFINE VARS_FTW_SPARE_OFFSET         = 0x00708000
DEFINE VARS_FTW_SPARE_SIZE           = 0x00018000


DEFINE FW_DOMAIN_SIZE    = $(FVMAIN_OFFSET) + $(FVMAIN_SIZE) - $(PEIFV_OFFSET)
DEFINE VARIABLE_FW_SIZE  = $(VARS_FTW_SPARE_OFFSET) + $(VARS_FTW_SPARE_SIZE) - $(VARS_OFFSET)

SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdRootFirmwareDomainBaseAddress = $(CODE_BASE_ADDRESS) + $(SECFV_OFFSET)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdRootFirmwareDomainSize        = $(ROOT_FW_DOMAIN_SIZE)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdFirmwareDomainBaseAddress     = $(CODE_BASE_ADDRESS) + $(PEIFV_OFFSET)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdFirmwareDomainSize            = $(FW_DOMAIN_SIZE)

SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdVariableFdBaseAddress = $(FW_BASE_ADDRESS) + $(VARS_OFFSET)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdVariableFdSize        = $(VARS_SIZE) + $(VARS_FTW_WORKING_SIZE) + $(VARS_FTW_SPARE_SIZE)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdVariableFdBlockSize   = $(BLOCK_SIZE)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdVariableFirmwareRegionBaseAddress = $(CODE_BASE_ADDRESS) + $(VARS_OFFSET)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdVariableFirmwareRegionSize        = $(VARIABLE_FW_SIZE)

# SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdOpenSbiStackSize = 8192
# SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdScratchRamBase   = $(CODE_BASE_ADDRESS) + $(SCRATCH_OFFSET)
# SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdScratchRamSize   = $(SCRATCH_SIZE)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdTemporaryRamBase = $(CODE_BASE_ADDRESS) + $(FW_SIZE)
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdTemporaryRamSize = 0x10000


SET gUefiRiscVPkgTokenSpaceGuid.PcdRiscVMachineTimerFrequencyInHerz    = 50000000
# SET gSophgoSG2042PlatformsPkgTokenSpaceGuid.PcdU5PlatformSystemClock = 500000000 # 1GHz system clock
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdBootHartId                  = 0          # Boot hart ID

#
# The bootable hart number the platform would like to use during boot.
#
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdBootableHartNumber          = 64
#
# Only use hart ID 1, 2, 3, 4
#
# SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdBootableHartIndexToId       =  {0x0}
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdBootableHartIndexToId       = {0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f}
                                                                                    # during boot
SET gSophgoSG2042PlatformsPkgTokenSpaceGuid.PcdNumberofU5Cores       = 64         # Total U5 cores enabled on U540 platform
SET gSophgoSG2042PlatformsPkgTokenSpaceGuid.PcdE5MCSupported         = True       # E51 MC exists.
SET gUefiRiscVPlatformPkgTokenSpaceGuid.PcdPeiCorePrivilegeMode        = 1          # Set PeiCore to S-Mode

SET gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase               = 0x7040000000
SET gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate                    = 115200
SET gHisiTokenSpaceGuid.PcdSerialPortSendDelay                         = 10000000
SET gHisiTokenSpaceGuid.PcdUartClkInHz                                 = 500000000