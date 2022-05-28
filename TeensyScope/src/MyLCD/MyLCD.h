/*
 * MyLCD.h - 320x480 LCD interface for Teensy 4
 */

#ifndef MyLCD_h
#define MyLCD_h

#define LEFT 0
#define RIGHT 9999
#define CENTER 9998

#define PORTRAIT 0
#define LANDSCAPE 1

//*********************************
// COLORS
//*********************************
// VGA color palette
#define VGA_BLACK		0x0000
#define VGA_WHITE		0xFFFF
#define VGA_RED			0xF800
#define VGA_GREEN		0x0400
#define VGA_BLUE		0x001F
#define VGA_SILVER		0xC618
#define VGA_GRAY		0x8410
#define VGA_MAROON		0x8000
#define VGA_YELLOW		0xFFE0
#define VGA_OLIVE		0x8400
#define VGA_LIME		0x07E0
#define VGA_AQUA		0x07FF
#define VGA_TEAL		0x0410
#define VGA_NAVY		0x0010
#define VGA_FUCHSIA		0xF81F
#define VGA_PURPLE		0x8010
#define VGA_TRANSPARENT	0xFFFFFFFF

/*
 * LCD controller type
 * #define only one
 */
#define ILI9481     // 3.2"IPS TFTLCD for Arduino Mega2560
//#define ILI9486     // 3.5"480x320 TFTLCD Shield for Arduino Mega2560

#include "Arduino.h"

struct _current_font
{
    uint8_t* font;
    uint8_t x_size;
    uint8_t y_size;
    uint8_t offset;
    uint8_t numchars;
};



class MyLCD
{
    public:
      	MyLCD();
      	void	InitLCD(byte orientation=LANDSCAPE);
      	void	clrScr();
      	void	drawPixel(int x, int y);
      	void	drawLine(int x1, int y1, int x2, int y2);
      	void	fillScr(uint8_t r, uint8_t g, uint8_t b);
      	void	fillScr(uint16_t color);
      	void	drawRect(int x1, int y1, int x2, int y2);
      	void	drawRoundRect(int x1, int y1, int x2, int y2);
      	void	fillRect(int x1, int y1, int x2, int y2);
      	void	fillRoundRect(int x1, int y1, int x2, int y2);
      	void	drawCircle(int x, int y, int radius);
      	void	fillCircle(int x, int y, int radius);
      	void	setColor(uint8_t r, uint8_t g, uint8_t b);
      	void	setColor(word color);
      	word	getColor();
      	void	setBackColor(uint8_t r, uint8_t g, uint8_t b);
      	void	setBackColor(uint32_t color);
      	word	getBackColor();
      	void	print(char *st, int x, int y, int deg=0);
      	void	print(char const *st, int x, int y, int deg=0);
      	void	printNumI(long num, int x, int y, int length=0, char filler=' ');
      	void	printNumF(double num, byte dec, int x, int y, char divider='.', int length=0, char filler=' ');
      	void	setFont(uint8_t* font);
      	uint8_t* getFont();
      	uint8_t	getFontXsize();
      	uint8_t	getFontYsize();
      	void	draw_xy_scope(int x, int y, int sx, int sy, uint16_t *data);
      	
      	void	lcdOff();
      	void	lcdOn();
      	void	setContrast(char c);
      	int		getDisplayXSize();
      	int		getDisplayYSize();

/*
	The functions and variables below should not normally be used.
	They have been left publicly available for use in add-on libraries
	that might need access to the lower level functions of LCD.

	Please note that these functions and variables are not documented
	and I do not provide support on how to use them.
*/
        
        
        void printChar(unsigned char c, int x, int y);
        void rotateChar(unsigned char c, int x, int y, int pos, int deg);
        void fast_fill(uint16_t d, long pix);
        void _convert_float(char *buf, double num, int width, byte prec);
    private:
        uint16_t  front_color, back_color;
        byte			orient;
        _current_font	cfont;
        boolean			_transparent;
        
        void write_command(uint8_t VL);
        void write_word(uint16_t d);
        void write_byte(uint8_t VL);
        void set_display_area(int x1, int y1, int x2, int y2);
        void reset_display_area();
        void draw_hor_line(int x, int y, int l);
        void draw_vert_line(int x, int y, int l);
};

#endif
