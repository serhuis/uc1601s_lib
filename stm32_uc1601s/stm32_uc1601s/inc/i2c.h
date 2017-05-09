#ifndef __I2C_H
#define __I2C_H

void I2C_LowLevel_Init(void);
void I2C_LowLevel_DeInit(void);
uint32_t I2C_WrBuf(uint8_t DevAddr, uint8_t *buf, uint32_t cnt);
uint32_t I2C_RdBuf(uint8_t DevAddr, uint8_t *buf, uint32_t cnt);
uint32_t I2C_RdBufEasy(uint8_t DevAddr, uint8_t *buf, uint32_t cnt);

#endif //__I2C_H
