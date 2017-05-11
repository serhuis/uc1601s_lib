#ifndef __UC1601S_H
#define __UC1601S_H

#include <stdint.h>

// select your display
#define LCD154
//#define LCD077
//#define LCD120

#ifdef LCD154 //132-64
  #define LCD_WIDTH 132
  #define LCD_HEIGHT 64
#endif

#ifdef LCD120//64-32
  #define LCD_WIDTH 64
  #define LCD_HEIGHT 32
#endif

#ifdef LCD077//128-64
  #define LCD_WIDTH 128
  #define LCD_HEIGHT 66
#endif

typedef enum {
  INVERSE_TYPE_NOINVERSE = 0,
  INVERSE_TYPE_INVERSE = 1
} inverse_type;

typedef enum {
  LINE_TYPE_WHITE = 0,
  LINE_TYPE_BLACK = 1,
  LINE_TYPE_DOT = 2
} line_type;

typedef enum {
  ANGLE_TYPE_RECT = 0,
  ANGLE_TYPE_ROUNDED = 1
} angle_type;

typedef enum {
  FILL_TYPE_TRANSPARENT = 0,
  FILL_TYPE_WHITE = 1,
  FILL_TYPE_BLACK = 2,
  FILL_TYPE_GRAY = 3,
  FILL_TYPE_SEA = 4
} fill_type;

typedef enum  {
  FONT_TYPE_5x8,
  FONT_TYPE_5x15,
  FONT_TYPE_10x15,
  FONT_TYPE_10x8,
} font_type;

void LCD_init (void);
void LCD_fill(uint8_t type);
void LCD_clear(void);
void LCD_cursor(uint8_t x,uint8_t y);
void LCD_symbol(char code, uint8_t width, uint8_t height, inverse_type inverse);
void LCD_string(char *str, uint8_t x,  uint8_t y, font_type font, inverse_type inverse);

// primitives
void LCD_pixel(uint8_t pixel_type, uint8_t x, uint8_t y);
void LCD_line(line_type line_type, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
void LCD_rect(line_type frame_type, angle_type ang_type, uint8_t border_width,
    fill_type fill, uint8_t x0, uint8_t y0, uint8_t width, uint8_t height);

#endif //__UC1601S_H
