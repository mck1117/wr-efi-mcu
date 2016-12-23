
#ifndef __FPGA_H
#define __FPGA_H

void Init_FPGA();

void FPGA_Read();
void FPGA_WriteRun();



#define FPGA_REG_TOOTH_COUNT 0x01
#define FPGA_REG_TOOTH_WIDTH 0x02
#define FPGA_REG_TEETH_MISSING 0x03

#define FPGA_REG_QUANTA_PER_REV 0x05
#define FPGA_REG_CYL_PHASE 0x06
#define FPGA_REG_TIMING 0x0A
#define FPGA_REG_DWELL 0x0B
#define FPGA_REG_INJ_PW1 0x0C
#define FPGA_REG_INJ_PW2 0x0D



#endif
