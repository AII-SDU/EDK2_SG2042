# Introduction to Sophgo SG2042 Platform #


This document provides guidelines for building UEFI firmware for Sophgo SG2042.

Sophgo SG2042 is a 64 and processor of RISC-V architecture.

Sophgo SG2042 UEFI can currently use Opensbi+UEFI firmware+GRUB to successfully enter the Linux distribution.

# How to build (X86 Linux Environment)#

## SG2042 EDK2 Initial Environment  ##

1. Use the original environment of SG2042 to load some content such as ZSBL into the SD card.

2. Install package on ubuntu  

	`$ sudo apt-get install autoconf automake autotools-dev curl python3 libmpc-dev libmpfr-dev`
	`libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev`   
	`libexpat-dev ninja-build uuide-dev` 

1. Follow edk2-platforms/Readme.md to obtaining source code,And config build env. For Example:  

	`$ export WORKSPACE=/work/git/tianocore`  
	`$ mkdir -p $WORKSPACE`  
	`$ cd $WORKSPACE` 
	`$ git clone https://github.com/tianocore/edk2.git`  
	`$ cd edk2`  
	`$ git submodule update --init`  
	`$ cd ..`  
	`$ git clone https://github.com/tianocore/edk2-platforms.git`  
	`$ cd edk2-platforms`  
	`$ git submodule update --init`  
	`$ cd ..`  
	`$ git clone https://github.com/tianocore/edk2-non-osi.git`  
	`$ export PACKAGES_PATH=$PWD/edk2:$PWD/edk2-platforms:$PWD/edk2-non-osi`  

1. Config cross compiler prefix. For Example:  

	`export GCC5_RISCV64_PREFIX=riscv64-unknown-elf-`  
	`export PYTHON_COMMAND=python3`

1. Set up the build environment And build BaseTool. For Example:  

	`$. edk2/edksetup.sh`  
	`$ make -C edk2/BaseTools`

1. Build platform. For Exmaple:  

	`build -a RISCV64 -t GCC5 -p Platform/Sophgo/SG2042Pkg/SG2042_EVB_v1_0_Board/SG2042.dsc`  
After a successful build, the resulting images can be found in Build/{Platform Name}/{TARGET}_{TOOL_CHAIN_TAG}/FV/SG2042.fd.


## Compile opensbi for SG2042 ##

Compile Opensbi and let it jump to the specified location.

1. Download code  
    `https://github.com/sophgo/opensbi.git`
1. Compile according to the following example  
    `export CROSS_COMPILE=riscv64-linux-gnu-`  
	`make BUILD_INFO=y DEBUG=1 PLATFORM=generic FW_PAYLOAD_PATH=/Build/RiscVVirtQemu/DEBUG_GCC5/FV/SG2042.fd FW_JUMP_ADDR=0x22000000 FW_JUMP_FDT_ADDR=0xbfe00000 -j32`

# start process  #


