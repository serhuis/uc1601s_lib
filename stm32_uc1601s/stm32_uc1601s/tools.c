#include "stm32f10x.h"
#include "inc/tools.h"

void tool_delay_ms(uint32_t ms)
{
  volatile uint32_t nCount = (SystemCoreClock/10000)*ms;
  for (; nCount!=0; nCount--);
}

uint8_t tool_strlen(char * str) {
  uint8_t s=0;
  while(str[s++]);
  return s - 1;
}

