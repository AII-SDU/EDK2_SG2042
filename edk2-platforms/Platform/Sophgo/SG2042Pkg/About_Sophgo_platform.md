# Sophogo SG2042 EDK2 RISC-V Platform Project    
## Sophgo SG2042 Platform
The development work for this project was carried out around the Sophgo SG2042 EVB development board. 
The deployment of EDK2 was completed on the EVB test version. Below is the external appearance display of the EVB version.  
![image](https://github.com/AII-SDU/EDK2_SG2042/blob/separate-v2/edk2-platforms/Platform/Sophgo/SG2042Pkg/Documents/Media/Sophgo_SG2042_EVB.png)  

SG2042 is a server processor based on the RISC-V architecture.It has 64 RISC-V cores and supports up to 2-way SG2042 interconnectivity.
Each SG2042 CPU can be configured with a maximum of 4 DIMM memory slots (RDIMM & UDIMM), supporting a maximum memory capacity of 256GB. It also supports a maximum of 32 PCIe 4.0 channels.  
![image](https://github.com/AII-SDU/EDK2_SG2042/blob/separate-v2/edk2-platforms/Platform/Sophgo/SG2042Pkg/Documents/Media/SG2042_CPU.png)

## SG2042 EDK2 RISC-V Design and the Boot Processes  
The SG2042 RISC-V EDK2 project is developed based on the original Sophgo SG2042 project.SG2042 RISC-V edk2 is designed and optimized based on the edk2 startup phase. The startup process can be roughly divided into the following stages:**ZSBL+FSBL+OpenSBI+EDK2+GRUB2+OS**.The design diagram and start-up process are shown in the figure below.

**SG2042 RISC-V EDK2 startup flowchart**
![image](https://github.com/AII-SDU/EDK2_SG2042/blob/separate-v2/edk2-platforms/Platform/Sophgo/SG2042Pkg/Documents/Media/EDK2_SDU_Programme.png)




