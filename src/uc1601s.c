#include "stm32f10x.h"
#include "inc/uc1601s.h"
#include "inc/i2c.h"
#include "inc/tools.h"

//set column address (duoble-byte command)
#define SET_COL_ADDR_LSB(col_addr_lsb)	0x00 | col_addr_lsb			// + column address CA[3:0] in bits [3:0]
#define SET_COL_ADDR_MSB(col_addr_msb)	0x10 | col_addr_msb			// + column address CA[7:4] in bits [3:0]

//temperature compensation
#define SET_TEMP_COMPENS				0x24	//+	TEMP_COMP value below
#define SET_TEMP_COMPENS_0_05		0x24|0x00
#define SET_TEMP_COMPENS_0_1		0x24|0x01
#define SET_TEMP_COMPENS_0_15		0x24|0x02
#define SET_TEMP_COMPENS_0			0x24|0x03

//power control
#define SET_POWER_CTRL		0x27
#define LOADING_0_15_nF		0x00
#define LOADING_15_24_nF	0x01
#define EXTERNAL_VLCD			0x00
#define INTERNAL_VLCD			0x06

//set scroll line
#define	SET_SCROLL_LINE(line_num)		0x40|line_num	//	+ line value in bits [5:0]

//set page address
#define	SET_PAGE_ADDR(paddr)			0xb0|paddr	// + page address in bits [3:0]

//LCD bias ratio
#define SET_BIAS_RATIO		0xE8
#define SET_BIAS_RATIO_6	0xE8|0x00
#define SET_BIAS_RATIO_7	0xE8|0x01
#define SET_BIAS_RATIO_8	0xE8|0x02
#define SET_BIAS_RATIO_9	0xE8|0x03

//Bias potentiometer (double-byte command, second byte is pot. value)
#define SET_BIAS_POT			0x81

//partial display control
#define PARTIAL_DISP_EN		0x81
#define PARTIAL_DISP_DIS	0x80

//RAM address control
#define SET_RAM_ADDR_CTRL(WA, IO, PID)		0x88 | WA | IO | PID
	//WA, wrap around enable bit
#define	WRAP_AROUND					0x01
#define	NO_WRAP_AROUND			0x00
	//IO, increment oreder (column or page first)
#define INC_COL_FIRST				0x00
#define INC_PAGE_FIRST			0x02
	//PID page increment direction, when WA == 1
#define PAGE_INC_DIR_NORMAL	0x00
#define PAGE_INC_DIR_RVERSE 0x04


//frame rate
#define SET_FRAME_RATE_80		0x20
#define SET_FRAME_RATE_100	0x21

//set all pixels on
#define SET_PIXELS_ON				0xA5

//set inverse display
#define SET_INVERSE_DISPL		0xA7

//set display enable
#define SET_DISPL_ENABLE		0xAF

//set mapping control
#define	SET_MAPPING_CONTROL(MX, MY)	(0xC | MX | MY)
#define MIRROR_X						0x20		
#define MIRROR_Y						0x40

//Systen reset
#define SYSTEM_RESET			0xE2


uint8_t cursorX, cursorY; // current position

// Symbol masks
const char chargen[];

/**
 * Initializaton
 */
void LCD_init(void)
{
  GPIO_InitTypeDef gpio_port;
	
  //Init reset pin (PC0)
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

  gpio_port.GPIO_Pin = GPIO_Pin_0;
  gpio_port.GPIO_Mode = GPIO_Mode_Out_PP;
  gpio_port.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_Init(GPIOC, &gpio_port);
  GPIO_ResetBits(GPIOC, GPIO_Pin_0 );

  GPIO_WriteBit(GPIOC, GPIO_Pin_0, Bit_SET); // Unreset
  I2C_LowLevel_Init();
  tool_delay_ms(10); // 1ms - 10ms
  {
	//    uint8_t buf[] = { b11100010 }; //System Reset
		uint8_t buf[] = { SYSTEM_RESET }; //System Reset
    I2C_WrBuf(0x70, buf, sizeof(buf));
  }
  tool_delay_ms(10); // 1ms - 10ms

#ifdef LCD154
  {
    //Set LCD Bias Ratio 11(9) - between V_LCD and V_D,
    //Set VBIAS Potentiometer (double-byte command) 120
    //Mirror X SEG/Column sequence inversion ON
    //Display On
    uint8_t buf[] =
//    { 0b11101011, 0b10000001, 120, 0b11000010, 0b10101111};
		{SET_BIAS_RATIO_9, SET_BIAS_POT, 120, SET_MAPPING_CONTROL(MIRROR_X, 0), SET_DISPL_ENABLE};
    I2C_WrBuf(0x70, buf, sizeof(buf));
  }
#else
  {
    // same as for LCD154 but with Mirror X SEG/Column sequence inversion OFF
//    uint8_t buf[] = { 0b11101011, 0b10000001, 120, 0b11000000, 0b10101111 };
		uint8_t buf[] = {SET_BIAS_RATIO_9, SET_BIAS_POT, 120, SET_MAPPING_CONTROL(0, 0), SET_DISPL_ENABLE};
    I2C_WrBuf(0x70, buf, sizeof(buf));
  }
#endif

  LCD_clear(0);
}

/**
 * Clear display
 * @param type: 0 - white, 1 - black, 2 - gray 50%
 */
void LCD_clear(uint8_t type)
{
  uint16_t j;
  {
    // Set Page Address 0, Set Column Address LSB, Set Column Address MSB
    uint8_t buf[] = 
//									{ 0b10110000, 0b00000000, 0b00010000 };
								{SET_PAGE_ADDR(0), SET_COL_ADDR_LSB(0), SET_COL_ADDR_MSB(0)};
		
    I2C_WrBuf(0x70, buf, sizeof(buf));
  }

  if (type == 0)
  {
    uint8_t buf[] = {0};
    for (j = 0; j < 1056; j++)
    {
      I2C_WrBuf(0x70 | 2, buf, sizeof(buf));
    }
  }
  else if (type == 1)
  {
    uint8_t buf[] = {0xFF};
    for (j = 0; j < 1056; j++)
    {
      I2C_WrBuf(0x70 | 2, buf, sizeof(buf));
    }
  }
  else
  {
    uint8_t buf[] = { 0xAA, 0x55 };
    for (j = 0; j < 528; j++)
    {
      I2C_WrBuf(0x70 | 2, buf, sizeof(buf));
    }
  }
}

/**
 * Plot pixel
 * @param pixel_type 0 - white, 1 - black
 * @param x 0-LCD_WIDTH-1
 * @param y 0-LCD_HEIGHT-1
 */
void LCD_pixel(uint8_t pixel_type, uint8_t x, uint8_t y)
{

  uint8_t page, page_num, bit_num;
	uint8_t buf[3];

  bit_num = y % 8; // bit number in page, which need be modified
  page_num = y / 8; // page number

  // set cursor
  {
    // setup page, Set Column Address LSB,  Set Column Address MSB
//    uint8_t buf[] = { 0b10110000 | page_num, x & 0b00001111, (x >> 4) | 0b00010000 };
    buf[0] =  SET_PAGE_ADDR(page_num);
		buf[1] = SET_COL_ADDR_LSB(x & 0x0f);
		buf[2] = SET_COL_ADDR_MSB(x >> 4);

		I2C_WrBuf(0x70, buf, sizeof(buf));
  }

  I2C_RdBuf(0x70 | 2, &page, sizeof(page)); //fake read
  I2C_RdBuf(0x70 | 2, &page, sizeof(page)); //read background

  // modify
  if (pixel_type)
  {
    TOOL_SET_BIT(page, bit_num);
  }
  else
  {
    TOOL_CLEAR_BIT(page, bit_num);
  }

  // set cursor again (it was moved during reading)
  {
    // setup page, Set Column Address LSB,  Set Column Address MSB
//    uint8_t buf[] = { 0b10110000 | page_num, x & 0b00001111, (x >> 4) | 0b00010000 };
    buf[0] =  SET_PAGE_ADDR(page_num);
		buf[1] = SET_COL_ADDR_LSB(x & 0x0f);
		buf[2] = SET_COL_ADDR_MSB(x >> 4);
		
    I2C_WrBuf(0x70, buf, sizeof(buf));
  }

  // write page again
  I2C_WrBuf(0x70 | 2, &page, sizeof(page));
}

/**
 * Bresenham's line algorithm
 * @param type: LINE_TYPE_WHITE, LINE_TYPE_BLACK, LINE_TYPE_DOT
 * @param x0
 * @param y0
 * @param x1
 * @param y1
 */
void LCD_line(line_type type, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{

  uint8_t step, t, pixel_type, x, y, ystep;
  int16_t deltax, deltay, error;

  step = (TOOL_ABS(y1-y0) > TOOL_ABS(x1-x0));

  if (step)
  {
    t = x0;
    x0 = y0;
    y0 = t;
    t = x1;
    x1 = y1;
    y1 = t;
  }
  if (x0 > x1)
  {
    t = x0;
    x0 = x1;
    x1 = t;
    t = y0;
    y0 = y1;
    y1 = t;
  }

  deltax = x1 - x0;
  deltay = TOOL_ABS(y1-y0);
  error = 0;
  y = y0;

  if (y0 < y1)
    ystep = 1;
  else
    ystep = -1;
  for (x = x0; x < x1 + 1; x++)
  {
    if (type == LINE_TYPE_WHITE)
      pixel_type = 0;
    else if (type == LINE_TYPE_BLACK)
      pixel_type = 1;
    else
      pixel_type = x & ((uint8_t) type - 1);
    if (step)
      LCD_pixel(pixel_type, y, x);
    else
      LCD_pixel(pixel_type, x, y);
    error += deltay;
    if ((error << 1) >= deltax)
    {
      y += ystep;
      error -= deltax;
    }
  }
}

/**
 * Draw rectangle
 * @param frame_type: LINE_TYPE_WHITE, LINE_TYPE_BLACK, LINE_TYPE_DOT, others - pattern
 * @param ang_type: ANGLE_TYPE_RECT , ANGLE_TYPE_ROUNDED
 * @param border_width: 0 - no, but with 1 px indent
 * @param fill: FILL_TRANSPARENT, FILL_WHITE, FILL_GRAY, FILL_SEA, other pattern
 * @param x0
 * @param y0
 * @param width
 * @param height
 */
void LCD_rect(line_type frame_type, angle_type ang_type, uint8_t border_width,
    fill_type fill, uint8_t x0, uint8_t y0, uint8_t width, uint8_t height)
{

  char a, b, x1, y1;

  char filler = 0;
  char steep, t, vid;
  int deltax, deltay, error;
  char x, y, poi;
  char ystep;
  char zx0, zy0, zy1, zx1;
  uint8_t RIS;

  x1 = x0 + width - 1;
  y1 = y0 + height - 1;
  if (border_width)
  {
    if (ang_type == ANGLE_TYPE_RECT)
    {
      b = 0;
      for (a = 0; a < border_width; a++)
      {
        LCD_line(frame_type, x0 + b, y0 + a, x1 - b, y0 + a);
        LCD_line(frame_type, x1 - a, y0 + b, x1 - a, y1 - b);
        LCD_line(frame_type, x1 - b, y1 - a, x0 + b, y1 - a);
        LCD_line(frame_type, x0 + a, y1 - b, x0 + a, y0 + b);
        b++;
      }
      y0 = y0 + a - 1;
      y1 = y1 - a + 1;
      x0 = x0 + a - 1;
      x1 = x1 - a + 1;
    }
    else // rounded
    {
      b = 0;
      for (a = 0; a < border_width; a++)
      {
        LCD_line(frame_type, x0 + 4 + b, y0 + a, x1 - 4 - b, y0 + a);
        LCD_line(frame_type, x1 - a, y0 + 4 + b, x1 - a, y1 - 4 - b);
        LCD_line(frame_type, x1 - 4 - b, y1 - a, x0 + 4 + b, y1 - a);
        LCD_line(frame_type, x0 + a, y1 - 4 - b, x0 + a, y0 + 4 + b);

        LCD_pixel(frame_type, x0 + 1 + b, y0 + 2 + b);
        LCD_pixel(frame_type, x0 + 1 + b, y0 + 3 + b);
        LCD_pixel(frame_type, x0 + 2 + b, y0 + 1 + b);
        LCD_pixel(frame_type, x0 + 3 + b, y0 + 1 + b);

        LCD_pixel(frame_type, x1 - 1 - b, y0 + 2 + b);
        LCD_pixel(frame_type, x1 - 1 - b, y0 + 3 + b);
        LCD_pixel(frame_type, x1 - 2 - b, y0 + 1 + b);
        LCD_pixel(frame_type, x1 - 3 - b, y0 + 1 + b);

        LCD_pixel(frame_type, x1 - 1 - b, y1 - 2 - b);
        LCD_pixel(frame_type, x1 - 1 - b, y1 - 3 - b);
        LCD_pixel(frame_type, x1 - 3 - b, y1 - 1 - b);
        LCD_pixel(frame_type, x1 - 2 - b, y1 - 1 - b);

        LCD_pixel(frame_type, x0 + 1 + b, y1 - 2 - b);
        LCD_pixel(frame_type, x0 + 1 + b, y1 - 3 - b);
        LCD_pixel(frame_type, x0 + 2 + b, y1 - 1 - b);
        LCD_pixel(frame_type, x0 + 3 + b, y1 - 1 - b);

        b++;
      }
      y0 = y0 + a - 1;
      y1 = y1 - a + 1;
      x0 = x0 + a - 1;
      x1 = x1 - a + 1;
    }
  }
  // fill
  if ((uint8_t) fill > 0)
  {
    for (a = 1; a < (x1 - x0); a++)
    {
      zy0 = y0 + 1;
      zy1 = y1 - 1;
      zx1 = zx0 = x0 + a;
      if ((uint8_t) ang_type)
      {
        if (a > 3)
          poi = width - a;
        else
          poi = a;
        switch (poi)
        {
          case 1:
            zy0 = zy0 + 3;
            zy1 = zy1 - 3;
            break;
          case 2:
          case 3:
            zy0 = zy0 + 1;
            zy1 = zy1 - 1;
            break;
        }
      }

      steep = (TOOL_ABS(zy1-zy0) > TOOL_ABS(zx1-zx0));
      if (steep)
      {
        t = zx0;
        zx0 = zy0;
        zy0 = t;
        t = zx1;
        zx1 = zy1;
        zy1 = t;
      }
      if (zx0 > zx1)
      {
        t = zx0;
        zx0 = zx1;
        zx1 = t;
        t = zy0;
        zy0 = zy1;
        zy1 = t;
      }
      deltax = zx1 - zx0;
      deltay = TOOL_ABS(zy1 - zy0);
      error = 0;
      y = zy0;
      if (zy0 < zy1)
        ystep = 1;
      else
        ystep = -1;
      for (x = zx0; x < zx1 + 1; x++)
      {
        if (fill == 1)
          vid = 0;
        else if (fill == 2)
          vid = 1;
        else if (RIS)
          vid = (x & 1);
        else
          vid = !(x & 1);
        if (steep)
          LCD_pixel(vid, y, x);
        else
          LCD_pixel(vid, x, y);
        error += deltay;
        if ((error << 1) >= deltax)
        {
          y += ystep;
          error -= deltax;
        }
      }

      if (fill > 2)
      {
        filler++;
        if (filler == fill - 2)
        {
          RIS = !RIS;
          filler = 0;
        }
      }
    }
  }
}

/**
 * Print string on screen
 * @param str - simple 0-terminated string, like "string"
 * @param x
 * @param y
 * @param font_type: FONT_TYPE_5x8, FONT_TYPE_5x15, FONT_TYPE_10x15, FONT_TYPE_10x8
 * @param inverse: INVERSE_TYPE_NOINVERSE, INVERSE_TYPE_INVERSE
 */
void LCD_string(char *str, uint8_t x, uint8_t y, font_type font,
    inverse_type inverse)
{
  uint8_t ptr = 0;
  uint8_t height, width;
  switch (font)
  {
    case FONT_TYPE_5x8:
      width = 1;
      height = 0;
      break;
    case FONT_TYPE_5x15:
      width = 1;
      height = 1;
      break;
    case FONT_TYPE_10x15:
      width = 2;
      height = 1;
      break;
    case FONT_TYPE_10x8:
      width = 2;
      height = 0;
      break;
    default:
      break;
  }

  LCD_cursor(x, y);
  while (str[ptr] != 0)
    LCD_symbol(str[ptr++], width, height, inverse);
}

/**
 * Display symbol on current cursor position
 * @param code: symbol
 * @param width: 1, 2 ...
 * @param height: 0, 1, 2 ...
 * @param inverse: INVERSE_TYPE_NOINVERSE, INVERSE_TYPE_INVERSE
 */
void LCD_symbol(char code, uint8_t width, uint8_t height, inverse_type inverse)
{
  uint8_t vert_offset, b, a, c, z, widthf, heightf;

  uint32_t buf, fon, vline, mask, mask1, mask2;

  uint16_t chargen_index = (code - 0x20) * 5; // character generator(chargen) consists of symbols strating
  //from 0x20 symbol (space). 5 - count of bytes, that determinate char:
  // each byte is vertical pixels(at total 5x8 pixels for one character)

  widthf = (width + 1) & 0x07; // width from 0 to 6
  heightf = height & 0x01; // hight only 0 or 1

  // vertical offset
  if ((cursorY / 8) == 0)
  {
    vert_offset = cursorY % 8;
  }
  else
  {
    vert_offset = (cursorY % 8) + 8;
  }

  // go by bytes in chargen
  for (b = 0; b < 5; b++)
  {
    if (heightf == 0) //char-s of single (1) height
    {
      if ((uint8_t) inverse)
      {
        vline = ~(uint8_t) chargen[chargen_index + b];
      }
      else
      {
        vline = chargen[chargen_index + b]; //else single load
      }

      vline = vline << vert_offset;
      if ((uint8_t) inverse)
      {
        TOOL_SET_BIT(vline, vert_offset-1);
      }
    }
    else //char-s of double (2) height
    {
      if ((uint8_t) inverse)
      {
        buf = ~chargen[chargen_index + b];
      }
      else
      {
        buf = chargen[chargen_index + b];
      }
      mask = 1;
      vline = 0;
      for (z = 0; z < 8; z++)
      {
        vline = vline >> 2;
        if (buf & mask)
        {
          vline |= 0xC000; //0b11000000 00000000;
        }
        mask = mask << 1;
      }
      vline = vline << vert_offset; //+1 коррекция сдвига символов двойной высоты (одна строка снизу)
      if ((uint8_t) inverse)
      {
        TOOL_SET_BIT(vline, vert_offset-1);
      }
    }

    // copy column of pixels by horisont widthf-1 times
    for (a = 1; a < widthf; a++)
    {
      if (cursorX > 131) // out of display 131
      {
		//        uint8_t buf[] = { 0b10001001 }; // move by column +
				uint8_t buf[] = {SET_RAM_ADDR_CTRL(WRAP_AROUND,INC_COL_FIRST,PAGE_INC_DIR_NORMAL)}; // move by column +
        I2C_WrBuf(0x70, buf, sizeof(buf));

        return;
      }
      LCD_cursor(cursorX, cursorY);
      // read all column (4 page)
      {
//        uint8_t buf[] = { 0b10001011 }; // move by page +
				uint8_t buf[] = { SET_RAM_ADDR_CTRL(WRAP_AROUND,INC_PAGE_FIRST,PAGE_INC_DIR_NORMAL) }; // move by page +
        I2C_WrBuf(0x70, buf, sizeof(buf));
      }

      //read all display column (32 bit)
      {
        uint8_t buf[6]; //0-th byte and 5-fth are fake
        I2C_RdBuf(0x70 | 2, buf, sizeof(buf));

        fon = buf[4];
        fon = fon << 8;
        fon += buf[3];
        fon = fon << 8;
        fon += buf[2];
        fon = fon << 8;
        fon += buf[1];
      }

      //generate mask for clear vertical line region
      if ((((uint8_t) inverse) == INVERSE_TYPE_INVERSE) || (cursorY == 0)) //if inverse or first line
      {
        if (heightf == 0)
        {
//          mask = 0b11111111 11111111 11111111 00000000;
						mask = 0xFFFFFFF0;
					
        }
        else
        {
//        mask = 0b11111111 11111111 00000000 00000000;
					mask = 0xFFFF0000;
        }
      }
      else
      {
        if (heightf == 0)
        {
//          mask = 0b11111111111111111111111100000000;
					mask = 0xFFFFFF00;
        }
        else
        {
//          mask = 0b11111111111111110000000000000000;
					mask = 0xFFFF0000;
        }
      }

      // shift vert_offset times. So mask will contain 0 at places, where we need change pixels
      for (c = 0; c < vert_offset; c++)
      {
        mask1 = (mask << 1);
        mask2 = (mask >> 31);
        mask = mask1 | mask2;
        //	mask=(mask << 1)|(mask >> 31); // rotate right
      }
      // apply mask to fon
      fon = fon & mask; //clean part, which will be replaced
      // and apply char line
      buf = vline | fon;

      // correct cursor
      LCD_cursor(cursorX, cursorY);
      cursorX++;

      {
//        uint8_t buf[] = { 0b10001011 }; // move by page +
				uint8_t buf[] = { SET_RAM_ADDR_CTRL(WRAP_AROUND,INC_PAGE_FIRST,PAGE_INC_DIR_NORMAL) }; // move by page +
        I2C_WrBuf(0x70, buf, sizeof(buf));
      }

      {
        uint8_t buff[4];
				buff[0] = buf & 0xff;
				buff[1] = (buf >> 8) & 0xff;
				buff[2] = (buf >> 16) & 0xff;
				buff[3] = (buf >> 24) & 0xff ;
 
        I2C_WrBuf(0x70 | 2, buff, sizeof(buf));
      }
    }
  }

  LCD_cursor(cursorX, cursorY); //correct cursor

  // == Clear separator line (1px between chars) ==
  {
//    uint8_t buf[] = { 0b10001011 }; // move by page +
		uint8_t buf[] = { SET_RAM_ADDR_CTRL(WRAP_AROUND,INC_PAGE_FIRST,PAGE_INC_DIR_NORMAL) }; // move by page +
    I2C_WrBuf(0x70, buf, sizeof(buf));
  }

  {
    uint8_t buf[6];
    I2C_RdBuf(0x70 | 2, buf, sizeof(buf));

    fon = buf[4];
    fon = fon << 8;
    fon += buf[3];
    fon = fon << 8;
    fon += buf[2];
    fon = fon << 8;
    fon += buf[1];
  }

  //generate mask for clear vertical line region
  if (((uint8_t) inverse) == 0)
  {
    if (heightf == 0)
    {
//      mask = 0b11111111111111111111111100000000;
			mask = 0xFFFFFF00;
    }
    else
    {
//      mask = 0b11111111111111110000000000000000;
			mask = 0xFFFF0000;
    }

    for (c = 0; c < vert_offset; c++)
    {
      mask1 = (mask << 1);
      mask2 = (mask >> 31);
      mask = mask1 | mask2;
    }
    fon = fon & mask;
  }
  else
  {
    if (heightf == 0)
    {
//      mask = 0b00000000000000000000000011111111;
			mask = 0x0000000F;
    }
    else
    {
//      mask = 0b00000000000000001111111111111111;
			mask = 0x0000FFFF;
    }
    for (c = 0; c < vert_offset; c++)
    {
      mask1 = (mask << 1);
      mask2 = (mask >> 31);
      mask = mask1 | mask2;
    }
    fon = fon | mask;
  }
  buf = fon;

  // correct cursor
  LCD_cursor(cursorX, cursorY);
  cursorX++;

  {
//    uint8_t buf[] = { 0b10001011 };
//    I2C_WrBuf(0x70, buf, sizeof(buf));
		uint8_t buf = SET_RAM_ADDR_CTRL(WRAP_AROUND,INC_PAGE_FIRST,PAGE_INC_DIR_NORMAL);
		I2C_WrBuf(0x70, &buf, sizeof(buf));
		
  }

  {
//    uint8_t buff[] = { buf & 0xff, (buf >> 8) & 0xff, (buf >> 16) & 0xff, (buf >> 24) & 0xff };
		uint8_t buff[4]
    I2C_WrBuf(0x70 | 2, buff, sizeof(buf));
  }

  {
    uint8_t buf[] = { 0b10001001 };
    I2C_WrBuf(0x70, buf, sizeof(buf));
  }

}

/**
 * Setup graphic cursor
 * @param X 0-LCD_WIDTH;
 * @param Y 0-LCD_HEIGHT
 */
void LCD_cursor(uint8_t x, uint8_t y)
{
  cursorY = y;
  cursorX = x;
  if ((y / 8) == 0)
    y = (y / 8);
  else
    y = (y / 8) - 1;
  {
    // setup page, Set Column Address LSB,  Set Column Address MSB
    uint8_t buf[] = { 0b10110000 | y, x & 0b00001111, (x >> 4) | 0b00010000 };
    I2C_WrBuf(0x70, buf, sizeof(buf));
  }
}

// знакогенератор CP1251
const char chargen[] = { 0x00, 0x00, 0x00, 0x00, 0x00, // 0x20	пробел
    0x00, 0x00, 0xF2, 0x00, 0x00, // 0x21	!
    0x00, 0xE0, 0x00, 0xE0, 0x00, // 0x22	"
    0x28, 0xFE, 0x28, 0xFE, 0x28, // 0x23	#
    0x24, 0x54, 0xFE, 0x54, 0x48, // 0x24	$
    0xC6, 0xC8, 0x10, 0x26, 0xC6, // 0x25	%
    0x6C, 0x92, 0xAA, 0x44, 0x0A, // 0x26	&
    0x00, 0xA0, 0xC0, 0x00, 0x00, // 0x27	'
    0x38, 0x44, 0x82, 0x00, 0x00, // 0x28	(
    0x00, 0x00, 0x82, 0x44, 0x38, // 0x29	)
    0x28, 0x10, 0x7C, 0x10, 0x28, // 0x2A	*
    0x10, 0x10, 0x7C, 0x10, 0x10, // 0x2B	+
    0x00, 0x00, 0x0A, 0x0C, 0x00, // 0x2C	,
    0x10, 0x10, 0x10, 0x10, 0x10, // 0x2D	-
    0x00, 0x00, 0x02, 0x00, 0x00, // 0x2E	.   0x00, 0x00, 0x06, 0x06, 0x06
    0x04, 0x08, 0x10, 0x20, 0x40, // 0x2F	/

    0x7C, 0x8A, 0x92, 0xA2, 0x7C, // 0x30	0
    0x00, 0x42, 0xFE, 0x02, 0x00, // 0x31	1
    0x42, 0x86, 0x8A, 0x92, 0x62, // 0x32	2
    0x84, 0x82, 0xA2, 0xD2, 0x8C, // 0x33	3
    0x18, 0x28, 0x48, 0xFE, 0x08, // 0x34	4
    0xE4, 0xA2, 0xA2, 0xA2, 0x9C, // 0x35	5
    0x3C, 0x52, 0x92, 0x92, 0x0C, // 0x36	6
    0x80, 0x8E, 0x90, 0xA0, 0xC0, // 0x37	7
    0x6C, 0x92, 0x92, 0x92, 0x6C, // 0x38	8
    0x60, 0x92, 0x92, 0x94, 0x78, // 0x39	9
    0x00, 0x00, 0x6C, 0x6C, 0x00, // 0x3A	:
    0x00, 0x00, 0x6A, 0x6C, 0x00, // 0x3B	//
    0x10, 0x28, 0x44, 0x82, 0x00, // 0x3C	<
    0x28, 0x28, 0x28, 0x28, 0x28, // 0x3D	=
    0x00, 0x82, 0x44, 0x28, 0x10, // 0x3E	>
    0x40, 0x80, 0x8A, 0x90, 0x60, // 0x3F	?

    0x4C, 0x92, 0x9C, 0x42, 0x3C, // 0x40	@
    0x7E, 0x88, 0x88, 0x88, 0x7E, // 0x41	А
    0xFE, 0x92, 0x92, 0x92, 0x6C, // 0x42	В
    0x7C, 0x82, 0x82, 0x82, 0x44, // 0x43	С
    0xFE, 0x82, 0x82, 0x44, 0x38, // 0x44	D
    0xFE, 0x92, 0x92, 0x92, 0x82, // 0x45	Е
    0xFE, 0x90, 0x90, 0x90, 0x80, // 0x46	F
    0x7C, 0x82, 0x92, 0x92, 0x5E, // 0x47	G
    0xFE, 0x10, 0x10, 0x10, 0xFE, // 0x48	Н
    0x00, 0x82, 0xFE, 0x82, 0x00, // 0x49	I
    0x04, 0x02, 0x82, 0xFC, 0x80, // 0x4A	J
    0xFE, 0x10, 0x28, 0x44, 0x82, // 0x4B	К
    0xFE, 0x02, 0x02, 0x02, 0x02, // 0x4C	L
    0xFE, 0x40, 0x20, 0x40, 0xFE, // 0x4D	М
    0xFE, 0x20, 0x10, 0x08, 0xFE, // 0x4E	N
    0x7C, 0x82, 0x82, 0x82, 0x7C, // 0x4F	О

    0xFE, 0x90, 0x90, 0x90, 0x60, // 0x50	Р
    0x7C, 0x82, 0x8A, 0x84, 0x7A, // 0x51	Q
    0xFE, 0x90, 0x98, 0x94, 0x62, // 0x52	R
    0x62, 0x92, 0x92, 0x92, 0x8C, // 0x53	S
    0x80, 0x80, 0xFE, 0x80, 0x80, // 0x54	Т
    0xFC, 0x02, 0x02, 0x02, 0xFC, // 0x55	U
    0xF8, 0x04, 0x02, 0x04, 0xF8, // 0x56	V
    0xFC, 0x02, 0x1C, 0x02, 0xFC, // 0x57	W
    0xC6, 0x28, 0x10, 0x28, 0xC6, // 0x58	Х
    0xE0, 0x10, 0x1E, 0x10, 0xE0, // 0x59	Y
    0x86, 0x8A, 0x92, 0xA2, 0xC2, // 0x5A	Z
    0x00, 0xFE, 0x82, 0x82, 0x00, // 0x5B	[
    0x18, 0x24, 0x7E, 0x24, 0x18, // 0x5C	ф
    0x00, 0x82, 0x82, 0xFE, 0x00, // 0x5D	]
    0x20, 0x40, 0x80, 0x40, 0x20, // 0x5E	^
    0x02, 0x02, 0x02, 0x02, 0x02, // 0x5F	_

    0x00, 0x00, 0x80, 0x40, 0x00, // 0x60	'
    0x04, 0x2A, 0x2A, 0x2A, 0x1E, // 0x61	а
    0xFE, 0x12, 0x22, 0x22, 0x1C, // 0x62	b
    0x1C, 0x22, 0x22, 0x22, 0x04, // 0x63	с
    0x1C, 0x22, 0x22, 0x12, 0xFE, // 0x64	d
    0x1C, 0x2A, 0x2A, 0x2A, 0x18, // 0x65	е
    0x10, 0x7E, 0x90, 0x80, 0x40, // 0x66	f
    0x10, 0x2A, 0x2A, 0x2A, 0x3C, // 0x67	g
    0xFE, 0x10, 0x20, 0x20, 0x1E, // 0x68	h
    0x00, 0x22, 0xBE, 0x02, 0x00, // 0x69	i
    0x04, 0x02, 0x22, 0xBC, 0x00, // 0x6A	j
    0x00, 0xFE, 0x08, 0x14, 0x22, // 0x6B	k
    0x00, 0x82, 0xFE, 0x02, 0x00, // 0x6C	l
    0x3E, 0x20, 0x18, 0x20, 0x1E, // 0x6D	m
    0x3E, 0x10, 0x20, 0x20, 0x1E, // 0x6E	n

    0x1C, 0x22, 0x22, 0x22, 0x1C, // 0x6F	o
    0x3E, 0x28, 0x28, 0x28, 0x10, // 0x70	р
    0x10, 0x28, 0x28, 0x28, 0x3E, // 0x71	q
    0x3E, 0x10, 0x20, 0x20, 0x10, // 0x72	r
    0x12, 0x2A, 0x2A, 0x2A, 0x24, // 0x73	s
    0x20, 0xFC, 0x22, 0x02, 0x04, // 0x74	t
    0x3C, 0x02, 0x02, 0x04, 0x3E, // 0x75	u
    0x38, 0x04, 0x02, 0x04, 0x38, // 0x76	v
    0x3C, 0x02, 0x0C, 0x02, 0x3C, // 0x77	w
    0x22, 0x14, 0x08, 0x14, 0x22, // 0x78	х
    0x30, 0x0A, 0x0A, 0x0A, 0x30, // 0x79	у
    0x22, 0x26, 0x2A, 0x32, 0x22, // 0x7A	z
    0x00, 0x10, 0x6C, 0x82, 0x00, // 0x7B	{
    0x00, 0x00, 0xFE, 0x00, 0x00, // 0x7C	|
    0x00, 0x82, 0x6C, 0x10, 0x00, // 0x7D	}
    0x08, 0x10, 0x10, 0x08, 0x08, // 0x7E	~

    0xFF, 0x80, 0x80, 0x80, 0x80, // 0x7F символы псевдографики
    0x80, 0x80, 0x80, 0x80, 0x80, // 0x80
    0x80, 0x80, 0xFF, 0x80, 0x80, // 0x81
    0x80, 0x80, 0x80, 0x80, 0xFF, // 0x82
    0xFF, 0x00, 0x00, 0x00, 0x00, // 0x83
    0x00, 0x00, 0xFF, 0x00, 0x00, // 0x84
    0x00, 0x00, 0x00, 0x00, 0xFF, // 0x85
    0xFF, 0x10, 0x10, 0x10, 0x10, // 0x86
    0x10, 0x10, 0x10, 0x10, 0x10, // 0x87
    0x10, 0x10, 0xFF, 0x10, 0x10, // 0x88
    0x10, 0x10, 0x10, 0x10, 0xFF, // 0x89
    0xFF, 0x10, 0x10, 0x10, 0x10, // 0x8A
    0x10, 0x10, 0x10, 0x10, 0x10, // 0x8B
    0x10, 0x10, 0xFF, 0x10, 0x10, // 0x8C
    0x10, 0x10, 0x10, 0x10, 0xFF, // 0x8D
    0xFF, 0x80, 0xBF, 0xA0, 0xA0, // 0x8E
    0xA0, 0xA0, 0xA0, 0xA0, 0xA0, // 0x8F
    0xA0, 0xBF, 0x80, 0xBF, 0xA0, // 0x90
    0xA0, 0xA0, 0xBF, 0x80, 0xFF, // 0x91
    0xFF, 0x00, 0xFF, 0x00, 0x00, // 0x92
    0x00, 0xFF, 0x00, 0xFF, 0x00, // 0x93
    0x00, 0x00, 0xFF, 0x00, 0xFF, // 0x94
    0xFF, 0x00, 0xEF, 0x28, 0x28, // 0x95
    0x28, 0x28, 0x28, 0x28, 0x28, // 0x96
    0x28, 0xEF, 0x00, 0xEF, 0x28, // 0x97
    0x28, 0x28, 0xEF, 0x00, 0xFF, // 0x98
    0xFF, 0x01, 0xFD, 0x05, 0x05, // 0x99
    0x05, 0x05, 0x05, 0x05, 0x05, // 0x9A
    0x05, 0xFD, 0x01, 0xFD, 0x05, // 0x9B
    0x05, 0x05, 0xFD, 0x01, 0xFF, // 0x9C

    0x00, 0x00, 0x3E, 0x22, 0x3E, // 0x9D 0 маленькие цифры
    0x00, 0x00, 0x00, 0x00, 0x3E, // 0x9E 1
    0x00, 0x00, 0x2E, 0x2A, 0x3A, // 0x9F 2
    0x00, 0x00, 0x2A, 0x2A, 0x3E, // 0xA0 3
    0x00, 0x00, 0x38, 0x08, 0x3E, // 0xA1 4
    0x00, 0x00, 0x3A, 0x2A, 0x2E, // 0xA2 5
    0x00, 0x00, 0x3E, 0x2A, 0x2E, // 0xA3 6
    0x00, 0x00, 0x20, 0x20, 0x3E, // 0xA4 7
    0x00, 0x00, 0x3E, 0x2A, 0x3E, // 0xA5 8
    0x82, 0xBA, 0xAA, 0x92, 0xBA, // 0xA6 dli
    0x8A, 0x8A, 0x82, 0xBA, 0x82, // 0xA7
    0x88, 0x54, 0x22, 0x88, 0x22, // 0xA8 нагрев
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, // 0xA9 дымогенератор
    0x10, 0x20, 0x10, 0x10, 0x20, // 0xAA	~// spe *
    0x20, 0x40, 0xFE, 0x40, 0x20, // 0xAB	su * стрелка верх

    0x08, 0x04, 0xFE, 0x04, 0x08, // 0xAC	sd * // вниз
    0x10, 0x10, 0x54, 0x38, 0x10, // 0xAD	sr * стрелка в право ->
    0x10, 0x38, 0x54, 0x10, 0x10, // 0xAE	sl * <- стрелка в влево
    0x00, 0x07, 0x08, 0x13, 0x24, // 0xAF	верхний левый угол
    0x28, 0x28, 0x28, 0x28, 0x28, // 0xB0	горизонтальные линии верняя
    0x28, 0x28, 0x13, 0x08, 0x07, // 0xB1	верний правый угол
    0x00, 0xFF, 0x00, 0xFF, 0x00, // 0xB2	веритикальные левые линии
    0x00, 0x00, 0xFF, 0x00, 0xFF, // 0xB3	вертикальные правые линии
    0x00, 0xE0, 0x10, 0xC8, 0x24, // 0xB4	нижний левый угол
    0x14, 0x24, 0xC8, 0x10, 0xE0, // 0xB5	нижний правый угол
    0x14, 0x14, 0x14, 0x14, 0x14, // 0xB6	горизонтальные линии нижняя
    0x10, 0x38, 0x7C, 0xFE, 0x00, // 0xB7	треугольник влево
    0x00, 0xFE, 0x7C, 0x38, 0x10, // 0xB8	треугольник вправо
    0x08, 0x78, 0xFC, 0x78, 0x08, // 0xB9	kol * колокольчик

    0x63, 0x25, 0x18, 0x18, 0xA4, // 0xBA	вентилятор
    0x7E, 0x42, 0x42, 0x42, 0x7E, // 0xBB	квадрат пустой
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, // 0xBC	квадрат полный
    0x30, 0x0C, 0x30, 0x0C, 0x30, // 0xBD	птичка
    0x80, 0xFF, 0x01, 0x01, 0xFF, // 0xBE	импульс
    0x60, 0x90, 0x90, 0x60, 0x00, // 0xBF	градус *

    0x7E, 0x88, 0x88, 0x88, 0x7E, // 0xC0	А
    0xFE, 0x92, 0x92, 0x92, 0x0C, // Б
    0xFE, 0x92, 0x92, 0x92, 0x6C, // В
    0xFE, 0x80, 0x80, 0x80, 0xC0, // Г
    0x07, 0x8A, 0xF2, 0x82, 0xFF, // Д
    0xFE, 0x92, 0x92, 0x92, 0x82, // Е
    0xEE, 0x10, 0xFE, 0x10, 0xEE, // 0хС6 Ж
    0x92, 0x92, 0x92, 0x92, 0x6C, // З
    0xFE, 0x08, 0x10, 0x20, 0xFE, // И
    0x3E, 0x84, 0x48, 0x90, 0x3E, // Й
    0xFE, 0x10, 0x28, 0x44, 0x82, // К
    0x04, 0x82, 0xFC, 0x80, 0xFE, // Л
    0xFE, 0x40, 0x20, 0x40, 0xFE, // М
    0xFE, 0x10, 0x10, 0x10, 0xFE, // Н
    0x7C, 0x82, 0x82, 0x82, 0x7C, // О
    0xFE, 0x80, 0x80, 0x80, 0xFE, // П
    0xFE, 0x90, 0x90, 0x90, 0x60, // Р
    0x7C, 0x82, 0x82, 0x82, 0x44, // С
    0x80, 0x80, 0xFE, 0x80, 0x80, // Т
    0xE2, 0x14, 0x08, 0x10, 0xE0, // У
    0x18, 0x24, 0xFE, 0x24, 0x18, // ф
    0xC6, 0x28, 0x10, 0x28, 0xC6, // Х

    0xFE, 0x02, 0x02, 0x02, 0xFF, // Ц
    0xE0, 0x10, 0x10, 0x10, 0xFE, // Ч
    0xFE, 0x02, 0xFE, 0x02, 0xFE, // Ш
    0xFE, 0x02, 0xFE, 0x02, 0xFF, // Щ
    0x80, 0xFE, 0x12, 0x12, 0x0C, // Ъ
    0xFE, 0x12, 0x0C, 0x00, 0xFE, // Ы
    0xFE, 0x12, 0x12, 0x12, 0x0C, // Ь
    0x44, 0x82, 0x92, 0x92, 0x7C, // Э
    0xFE, 0x10, 0x7C, 0x82, 0x7C, // Ю
    0x62, 0x94, 0x98, 0x90, 0xFE, // Я
    0x04, 0x2A, 0x2A, 0x2A, 0x1E, // а
    0x3C, 0x52, 0x52, 0x92, 0x8C, // б
    0x3E, 0x2A, 0x2A, 0x14, 0x00, // в
    0x3E, 0x20, 0x20, 0x20, 0x30, // г

    0x07, 0x2A, 0x32, 0x22, 0x3F, // д
    0x1C, 0x2A, 0x2A, 0x2A, 0x18, // е
    0x36, 0x08, 0x3E, 0x08, 0x36, // ж
    0x22, 0x22, 0x2A, 0x2A, 0x14, // з
    0x3E, 0x04, 0x08, 0x10, 0x3E, // и
    0x1E, 0x42, 0x24, 0x48, 0x1E, // й
    0x3E, 0x08, 0x14, 0x22, 0x00, // к
    0x04, 0x22, 0x3C, 0x20, 0x3E, // л
    0x3E, 0x10, 0x08, 0x10, 0x3E, // м
    0x3E, 0x08, 0x08, 0x08, 0x3E, // н
    0x1C, 0x22, 0x22, 0x22, 0x1C, // o
    0x3E, 0x20, 0x20, 0x20, 0x3E, // п
    0x3E, 0x28, 0x28, 0x28, 0x10, // р
    0x1C, 0x22, 0x22, 0x22, 0x04, // с

    0x20, 0x20, 0x3E, 0x20, 0x20, // т
    0x30, 0x0A, 0x0A, 0x0A, 0x3C, // у
    0x38, 0x44, 0xFE, 0x44, 0x38, // Ф
    0x22, 0x14, 0x08, 0x14, 0x22, // х
    0x3E, 0x02, 0x02, 0x02, 0x3F, // ц
    0x30, 0x08, 0x08, 0x08, 0x3E, // ч
    0x3E, 0x02, 0x3E, 0x02, 0x3E, // ш
    0x3E, 0x02, 0x3E, 0x02, 0x3F, // щ
    0x20, 0x3E, 0x0A, 0x0A, 0x04, // ъ
    0x3E, 0x0A, 0x04, 0x00, 0x3E, // ы
    0x3E, 0x0A, 0x0A, 0x04, 0x00, // ь
    0x14, 0x22, 0x2A, 0x2A, 0x1C, // э
    0x3E, 0x08, 0x1C, 0x22, 0x1C, // ю
    0x10, 0x2A, 0x2C, 0x28, 0x3E, // я
    };

