#include "stm32f10x.h"
#include "inc/uc1601s.h"
#include "inc/tools.h"

uint8_t i = 0;

int main(void) {
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); // Enable TIM2 Periph clock

  // Timer base configuration
  TIM_TimeBaseStructure.TIM_Period = 2000;  //Period - 2s
  TIM_TimeBaseStructure.TIM_Prescaler = (uint16_t) (SystemCoreClock / 1000) - 1; //1000 Hz
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
  TIM_Cmd(TIM2, ENABLE);

  LCD_init();

  //Enable TIM2 IRQ
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  while (1) {}; // Infinity loop
}

void TIM2_IRQHandler(void) {
  TIM_ClearITPendingBit(TIM2, TIM_SR_UIF );

  // Some LCD demonstrations
  LCD_clear(0);
  switch (i++ % 7) {
    case 0:
      LCD_string("10x15", 0, 0, FONT_TYPE_10x15, INVERSE_TYPE_NOINVERSE);
      break;
    case 1:
      LCD_string("5x8", 0, 0, FONT_TYPE_5x8, INVERSE_TYPE_NOINVERSE);
      break;
    case 2:
      LCD_string("5x15", 0, 0, FONT_TYPE_5x15, INVERSE_TYPE_NOINVERSE);
      break;
    case 3:
      LCD_string("10x8", 0, 0, FONT_TYPE_10x8, INVERSE_TYPE_NOINVERSE);
      break;
    case 4: {
      char *string = "5x8i";
      LCD_rect(LINE_TYPE_BLACK, ANGLE_TYPE_ROUNDED, 1, FILL_TYPE_BLACK, 8, 8,
          tool_strlen(string) * 6 + 5, 13); //tool_strlen(string) * 6 + 5 - calculate rect width for place str
      LCD_string("5x8i", 10, 10, FONT_TYPE_5x8, INVERSE_TYPE_INVERSE);
      break;
    }
    case 5:
      LCD_rect(LINE_TYPE_BLACK, ANGLE_TYPE_RECT, 1, FILL_TYPE_TRANSPARENT, 2, 2,
          20, 10);
      LCD_rect(LINE_TYPE_BLACK, ANGLE_TYPE_ROUNDED, 1, FILL_TYPE_TRANSPARENT, 8,
          8, 20, 10);
      LCD_rect(LINE_TYPE_DOT, ANGLE_TYPE_RECT, 1, FILL_TYPE_WHITE, 14, 14, 20,
          10);
      LCD_rect(LINE_TYPE_BLACK, ANGLE_TYPE_RECT, 1, FILL_TYPE_GRAY, 20, 20, 20,
          10);
      break;
    case 6: {
      uint8_t j;
      for (j = 0; j <= LCD_WIDTH; j = j + 3) {
        LCD_line(LINE_TYPE_BLACK, j, 0, j, LCD_HEIGHT - 1);
      }
      break;
    }
  }
}

