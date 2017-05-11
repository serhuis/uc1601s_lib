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

  switch (i++ % 7) {
    case 0:
			LCD_clear();
			LCD_string("Hello world!", 0, 64, FONT_TYPE_10x15, INVERSE_TYPE_NOINVERSE);
      break;
    case 1:
			LCD_clear();
      LCD_string("Hello world!", 0, 56, FONT_TYPE_5x8, INVERSE_TYPE_NOINVERSE);
      break;
    case 2:
			LCD_clear();
      LCD_string("Hello world!", 0, 48, FONT_TYPE_5x15, INVERSE_TYPE_NOINVERSE);
      break;
    case 3:
			LCD_clear();
      LCD_string("Hello world!", 0, 40, FONT_TYPE_10x8, INVERSE_TYPE_NOINVERSE);
      break;
    case 4: {
      char *string = "Hello world!";
			LCD_clear();
      LCD_rect(LINE_TYPE_BLACK, ANGLE_TYPE_ROUNDED, 1, FILL_TYPE_GRAY, 8, 8,100, 15); //tool_strlen(string) * 6 + 5 - calculate rect width for place str
//      LCD_string("Hello world!", 10, 10, FONT_TYPE_5x8, INVERSE_TYPE_INVERSE);
      break;
    }
    case 5:
			LCD_clear();
      LCD_rect(LINE_TYPE_BLACK, ANGLE_TYPE_RECT, 1, FILL_TYPE_TRANSPARENT, 0, 0,
          132, 64);
      LCD_rect(LINE_TYPE_BLACK, ANGLE_TYPE_ROUNDED, 1, FILL_TYPE_TRANSPARENT, 8,
          8, 20, 10);
      LCD_rect(LINE_TYPE_DOT, ANGLE_TYPE_RECT, 1, FILL_TYPE_WHITE, 14, 14, 20,
          10);
      LCD_rect(LINE_TYPE_BLACK, ANGLE_TYPE_RECT, 1, FILL_TYPE_GRAY, 20, 20, 20,
          10);
      break;
    case 6: {
      uint8_t j;
      for (j = 0; j < LCD_WIDTH; j+=3) {
        LCD_line(LINE_TYPE_DOT, j, 0, j, LCD_HEIGHT - 1);
      }
      break;
    }
  }
}

