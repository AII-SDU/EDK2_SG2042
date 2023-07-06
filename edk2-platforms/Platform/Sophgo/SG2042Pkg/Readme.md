# Introduction to Sophgo SG2042 Platform #


This document provides guidelines for building UEFI firmware for Sophgo SG2042.
Sophgo SG2042 is a 64 and processor of RISC-V architecture.
Sophgo SG2042 UEFI can currently use Opensbi+UEFI firmware+GRUB to successfully enter the Linux distribution.

## How to build (X86 Linux Environment)

### SG2042 EDK2 Initial Environment  ###

####statement：The operating environment of this project is deployed on the Sophgo original environment.  
1. Build Sophgo SG2042 original environment，The specific compilation process is https://github.com/sophgo/sophgo-doc.  
  Note: The ZSBL mentioned in the original environment is later called FSBL.

2. Install package on ubuntu  

	`$ sudo apt-get install autoconf automake autotools-dev curl python3 libmpc-dev libmpfr-dev`
	`libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev`   
	`libexpat-dev ninja-build uuide-dev` 

3. Follow edk2-platforms/Readme.md to obtaining source code,And config build env. For Example:  

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

4. Config cross compiler prefix. For Example:  

	`export GCC5_RISCV64_PREFIX=riscv64-unknown-elf-`  
	`export PYTHON_COMMAND=python3`

5. Set up the build environment And build BaseTool. For Example:  

	`$. edk2/edksetup.sh`  
	`$ make -C edk2/BaseTools`

6. Build platform. For Exmaple:  

	`build -a RISCV64 -t GCC5 -p Platform/Sophgo/SG2042Pkg/SG2042_EVB_Board/SG2042.dsc`  
After a successful build, the resulting images can be found in Build/{Platform Name}/{TARGET}_{TOOL_CHAIN_TAG}/FV/SG2042.fd.  
7. The SG2042.fd file will be renamed to riscv64_Image using the "mv" command.  
   `$ mv SG2042.fd riscv64_Image`
8. Now go to replace the original riscv64_Image file under SD boot,then you can enter the EDK2 Shell.

9. Run to EDK2 Shell, you can use GRUB2 to boot the linux OS, you can build GRUB2 yourself ( https://www.gnu.org/software/grub/grub.html), or use the built (https://github.com/AII-SDU/GRUB.git).  
Put the completed files into the fs0: directory for execution.
   

## Platform Status ##  
**SG2042_EVB_Board** Currently the binary built from SG2042 edk2 package can boot Sophgo SG2042 EVB to EFI shell with console,And using GRUB2, the operating system will be booted and the Linux operating system will be entered for execution. Please refer to 
https://github.com/AII-SDU/EDK2_SG2042/blob/separate-v2/edk2-platforms/Platform/Sophgo/SG2042Pkg/About_Sophgo_platform.md 
for booting to Linux OS.

## Supported Operating Systems
The preliminary running test of the following operating systems has been completed on the EVB test board, and the desktop environment has been deployed.
1. Ubuntu
2. Fedora
3. openKylin
4. opemEuler

## Known Issues and Limitations
This test only runs on SG2042 EVB with RISC-V RV64 architecture






