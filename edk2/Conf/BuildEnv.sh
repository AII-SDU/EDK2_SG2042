# Auto-generated by BaseTools/BuildEnv
if [ -z "$WORKSPACE" ]
then
  export WORKSPACE=/home/cyq/Downloads/edk2-edk2-stable202305
fi
if [ -z "$EDK_TOOLS_PATH" ]
then
  export EDK_TOOLS_PATH=/home/cyq/Downloads/edk2-edk2-stable202305/BaseTools
fi
if [ -e /home/cyq/Downloads/edk2-edk2-stable202305/BaseTools/Bin/Linux-x86_64 ]
then
  FOUND_TOOLS_PATH_BIN=0
  for DIR in $(echo $PATH | tr ':' ' '); do
    if [ "$DIR" = "/home/cyq/Downloads/edk2-edk2-stable202305/BaseTools/Bin/Linux-x86_64" ]; then
      FOUND_TOOLS_PATH_BIN=1
    fi
  done
  if [ $FOUND_TOOLS_PATH_BIN = 0 ]
  then
    export PATH=/home/cyq/Downloads/edk2-edk2-stable202305/BaseTools/Bin/Linux-x86_64:$PATH
  fi
fi
