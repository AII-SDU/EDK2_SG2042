make -C edk2/BaseTools
build -a RISCV64 -t GCC5 -p Platform/Sophgo/SG2042Pkg/SG2042_EVB_Board/SG2042.dsc
cd $WORKSPACE/Build/SG2042_EVB/DEBUG_GCC5/FV
mv SG2042.fd riscv64_Image
cd $WORKSPACE
