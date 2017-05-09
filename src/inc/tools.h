#ifndef __TOOLS_H
#define __TOOLS_H

void tool_delay_ms(uint32_t ms);
uint8_t tool_strlen(char * str);

#define TOOL_SET_BIT(var,bitno)   ( (var) |=  (1 << (bitno)) );
#define TOOL_CLEAR_BIT(var,bitno) ( (var) &= ~(1 << (bitno)) );
#define TOOL_ABS(a)               ( ( (a) > 0 ) ? (a) : -(a) )

#endif //__TOOLS_H
