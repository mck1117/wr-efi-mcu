
#ifndef __FPGA_H
#define __FPGA_H

void Init_FPGA();

void FPGA_Read();
void FPGA_WriteRun();


#define FPGA_REG_ENABLES	0x00
#define FPGA_REG_TOOTH_COUNT 0x01
#define FPGA_REG_TEETH_MISSING 0x02

#define FPGA_REG_CYL_PHASE_BASE 0x03
#define FPGA_REG_TIMING 0x07
#define FPGA_REG_DWELL 0x08
#define FPGA_REG_INJ_PW1 0x09
#define FPGA_REG_INJ_PW2 0x0A



#define FPGA_REG_STATUS 0x00
#define FPGA_REG_RPM 0x01


#endif
