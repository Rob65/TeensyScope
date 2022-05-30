/*
  MyLCD.cpp - 320x480 LCD interface for Teensy 4
*/

#include "MyLCD.h"

#define DRAW_RETICLE // Undefine when no reticle should be drawn

/*
 * I/O Pin definitions for the LCD interface
 * The LCD uses a 16 bits 8080 series parallel interface
 * with 16 data bits and 4 extra signals for chip select,
 * write, register select (data/command) and reset signals.
 *
 * These can be placed at any of the available I/O pins of
 * the Teensy 4.0 or 4.1
 * Please note that on a Teensy 4, only 4 I/O pins are left
 * 
 */
 
#define DB_0_PIN  40
#define DB_1_PIN  39
#define DB_2_PIN  38
#define DB_3_PIN  37
#define DB_4_PIN  36
#define DB_5_PIN  35
#define DB_6_PIN  34
#define DB_7_PIN  33
#define DB_8_PIN  32
#define DB_9_PIN  31
#define DB_10_PIN 30
#define DB_11_PIN 29
#define DB_12_PIN 28
#define DB_13_PIN 27
#define DB_14_PIN 26
#define DB_15_PIN 25

#define RS_PIN    12
#define WR_PIN    24
#define CS_PIN    41
#define RST_PIN   13

#define DISPLAY_COLUMNS 320 // Number of LCD colums
#define DISPLAY_ROWS 480    // Number of LCD rows

// Some helper defines

#define swap(type, i, j) {type t = i; i = j; j = t;}
#define set_pixel(color) write_word(color); 

/*
 * Create a pulse on the WR line in order to write the data
 * to the LCD.
 * The delay between setting the pin low and high again
 * is needed in order to get the proper timing.
 * With a 10 ns delay, the pulsewidth is 24 ns
 * The ILI9486 datasheet specifies a min. 15 ns pulse but
 * (I guess) due to the ACL245 chips with a 5V power supply on
 * the LCD module, we need a bit longer timing.
 * Please note that the 2 ns delay after setting WR high again
 * is needed to make sure that, in certain special cases,
 * the WR pin is high long enough
 */
inline void pulse_WR()
{
  digitalWriteFast(WR_PIN, LOW);
  delayNanoseconds(10);
  digitalWriteFast(WR_PIN,HIGH);
  delayNanoseconds(2);
}

#define bitmapdatatype unsigned short*

MyLCD::MyLCD()
{ 
    pinMode(DB_0_PIN , OUTPUT);
    pinMode(DB_1_PIN , OUTPUT);
    pinMode(DB_2_PIN , OUTPUT);
    pinMode(DB_3_PIN , OUTPUT);
    pinMode(DB_4_PIN , OUTPUT);
    pinMode(DB_5_PIN , OUTPUT);
    pinMode(DB_6_PIN , OUTPUT);
    pinMode(DB_7_PIN , OUTPUT);
    pinMode(DB_8_PIN , OUTPUT);
    pinMode(DB_9_PIN , OUTPUT);
    pinMode(DB_10_PIN, OUTPUT);
    pinMode(DB_11_PIN, OUTPUT);
    pinMode(DB_12_PIN, OUTPUT);
    pinMode(DB_13_PIN, OUTPUT);
    pinMode(DB_14_PIN, OUTPUT);
    pinMode(DB_15_PIN, OUTPUT);
    
    pinMode(RS_PIN,OUTPUT);
    pinMode(WR_PIN,OUTPUT);
    pinMode(CS_PIN,OUTPUT);
    pinMode(RST_PIN,OUTPUT);

}

/*
 * Writes a command to the LCD
 * The register select pin is set low in order to
 * address the command register
 * Note that commands are always 8 bits so there is
 * no need to set The 8..15 bits on the LCD interface
 */
void MyLCD::write_command(uint8_t cmd)  
{   
    digitalWriteFast(RS_PIN, LOW);
    digitalWriteFast(DB_0_PIN , cmd & 0x01);
    digitalWriteFast(DB_1_PIN , cmd & 0x02);
    digitalWriteFast(DB_2_PIN , cmd & 0x04);
    digitalWriteFast(DB_3_PIN , cmd & 0x08);
    digitalWriteFast(DB_4_PIN , cmd & 0x10);
    digitalWriteFast(DB_5_PIN , cmd & 0x20);
    digitalWriteFast(DB_6_PIN , cmd & 0x40);
    digitalWriteFast(DB_7_PIN , cmd & 0x80);
    pulse_WR();
    digitalWriteFast(RS_PIN, HIGH);
}

/*
 * Writes 16 bits data to the LCD
 */
void MyLCD::write_word(uint16_t d)
{
    digitalWriteFast(DB_0_PIN , d & 0x01);
    digitalWriteFast(DB_1_PIN , d & 0x02);
    digitalWriteFast(DB_2_PIN , d & 0x04);
    digitalWriteFast(DB_3_PIN , d & 0x08);
    digitalWriteFast(DB_4_PIN , d & 0x10);
    digitalWriteFast(DB_5_PIN , d & 0x20);
    digitalWriteFast(DB_6_PIN , d & 0x40);
    digitalWriteFast(DB_7_PIN , d & 0x80);
    d >>=8;
    digitalWriteFast(DB_8_PIN , d & 0x01);
    digitalWriteFast(DB_9_PIN , d & 0x02);
    digitalWriteFast(DB_10_PIN, d & 0x04);
    digitalWriteFast(DB_11_PIN, d & 0x08);
    digitalWriteFast(DB_12_PIN, d & 0x10);
    digitalWriteFast(DB_13_PIN, d & 0x20);
    digitalWriteFast(DB_14_PIN, d & 0x40);
    digitalWriteFast(DB_15_PIN, d & 0x80);
    
    pulse_WR();
}

/*
 * Writes 8 bits data to the LCD
 */
void MyLCD::write_byte(uint8_t b)
{
    digitalWriteFast(DB_0_PIN , b & 0x01);
    digitalWriteFast(DB_1_PIN , b & 0x02);
    digitalWriteFast(DB_2_PIN , b & 0x04);
    digitalWriteFast(DB_3_PIN , b & 0x08);
    digitalWriteFast(DB_4_PIN , b & 0x10);
    digitalWriteFast(DB_5_PIN , b & 0x20);
    digitalWriteFast(DB_6_PIN , b & 0x40);
    digitalWriteFast(DB_7_PIN , b & 0x80);
    
    pulse_WR();
}

/*
 * Fill area with one color
 * We only need to create a series of write pulses
 * after setting the data.
 * An extra 5 ns delay is added to make sure the
 * WR line is high for at least 15 ns
 */
void MyLCD::fast_fill(uint16_t d, long pix)
{
    digitalWriteFast(DB_0_PIN , d & 0x01);
    digitalWriteFast(DB_1_PIN , d & 0x02);
    digitalWriteFast(DB_2_PIN , d & 0x04);
    digitalWriteFast(DB_3_PIN , d & 0x08);
    digitalWriteFast(DB_4_PIN , d & 0x10);
    digitalWriteFast(DB_5_PIN , d & 0x20);
    digitalWriteFast(DB_6_PIN , d & 0x40);
    digitalWriteFast(DB_7_PIN , d & 0x80);
    d >>= 8;
    digitalWriteFast(DB_8_PIN , d & 0x01);
    digitalWriteFast(DB_9_PIN , d & 0x02); 
    digitalWriteFast(DB_10_PIN, d & 0x04);
    digitalWriteFast(DB_11_PIN, d & 0x08);
    digitalWriteFast(DB_12_PIN, d & 0x10);
    digitalWriteFast(DB_13_PIN, d & 0x20);
    digitalWriteFast(DB_14_PIN, d & 0x40);
    digitalWriteFast(DB_15_PIN, d & 0x80);

    for(int i=0; i<pix; i++) {
        pulse_WR(); delayNanoseconds(5);
    }
}

void MyLCD::InitLCD(byte orientation)
{
    orient=orientation;
    
    digitalWriteFast(RST_PIN, HIGH);
    delay(5); 
    digitalWriteFast(RST_PIN, LOW);
    delay(15);
    digitalWriteFast(RST_PIN, HIGH);
    delay(15);
    
    digitalWriteFast(CS_PIN, 0);
#if defined ILI9481
    write_command(0x11);
    delay(20);
    write_command(0xD0);
    write_byte(0x07);
    write_byte(0x42);
    write_byte(0x18);

    write_command(0xD1);
    write_byte(0x00);
    write_byte(0x07);
    write_byte(0x10);

    write_command(0xD2);
    write_byte(0x01);
    write_byte(0x02);

    write_command(0xC0);
    write_byte(0x10);
    write_byte(0x3B);
    write_byte(0x00);
    write_byte(0x02);
    write_byte(0x11);

    write_command(0xC5);
    write_byte(0x03);

    write_command(0xC8);
    write_byte(0x00);
    write_byte(0x32);
    write_byte(0x36);
    write_byte(0x45);
    write_byte(0x06);
    write_byte(0x16);
    write_byte(0x37);
    write_byte(0x75);
    write_byte(0x77);
    write_byte(0x54);
    write_byte(0x0C);
    write_byte(0x00);

    write_command(0x36);
    write_byte(0x0A);


    write_command(0x3A);
    write_byte(0x55);

    write_command(0x2A);
    write_byte(0x00);
    write_byte(0x00);
    write_byte(0x01);
    write_byte(0x3F);

    write_command(0x2B);
    write_byte(0x00);
    write_byte(0x00);
    write_byte(0x01);
    write_byte(0xE0);
    delay(120);
    write_command(0x29);
    
#elif defined ILI9486
    write_command(0x11);    // Sleep OUT
    delay(50);
    
    write_command(0xC0);    // Power Control 1
    write_byte(0x0d);
    write_byte(0x0d);
    
    write_command(0xC1);    // Power Control 2
    write_byte(0x43);
    write_byte(0x00);
    
    write_command(0xC2);    // Power Control 3
    write_byte(0x00);
    
    write_command(0xC5);    // VCOM Control
    write_byte(0x00);       // Do not program NV Memory
    write_byte(0x48);       // VCOM = -0.875
    
    write_command(0xB6);    // Display Function Control
    write_byte(0x00);       // Use memory mode
    write_byte(0x22);       // 0x42 = Rotate display 180 deg.
    write_byte(0x3B);
    
    write_command(0xE0);    // PGAMCTRL (Positive Gamma Control)
    write_byte(0x0f);
    write_byte(0x24);
    write_byte(0x1c);
    write_byte(0x0a);
    write_byte(0x0f);
    write_byte(0x08);
    write_byte(0x43);
    write_byte(0x88);
    write_byte(0x32);
    write_byte(0x0f);
    write_byte(0x10);
    write_byte(0x06);
    write_byte(0x0f);
    write_byte(0x07);
    write_byte(0x00);
    
    write_command(0xE1);    // NGAMCTRL (Negative Gamma Control)
    write_byte(0x0F);
    write_byte(0x38);
    write_byte(0x30);
    write_byte(0x09);
    write_byte(0x0f);
    write_byte(0x0f);
    write_byte(0x4e);
    write_byte(0x77);
    write_byte(0x3c);
    write_byte(0x07);
    write_byte(0x10);
    write_byte(0x05);
    write_byte(0x23);
    write_byte(0x1b);
    write_byte(0x00); 
    
    write_command(0x20);    // Display Inversion OFF
    write_byte(0x00);    
    
    write_command(0x36);    // Memory Access Control
    write_byte(0x0A);
    
    write_command(0x3A);    // Interface Pixel Format
    write_byte(0x55); 
    write_command(0x29);    // Display ON
#else
    #error Enable one LCD type in MyLCD.h
#endif

    digitalWriteFast(CS_PIN, HIGH);
    
    setColor(255, 255, 255);
    setBackColor(0, 0, 0);
    cfont.font=0;
    _transparent = false;
}

/*
 * Set the display area were data is written to.
 * In this way, we can just update a rectangular area
 * by just sending a stream of data without having to
 * update any address information for pixel locations.
 */
void MyLCD::set_display_area(int x1, int y1, int x2, int y2)
{
    if (orient==LANDSCAPE)
    {
        swap(int, x1, y1);
        swap(int, x2, y2)
        y1=DISPLAY_ROWS-1-y1;
        y2=DISPLAY_ROWS-1-y2;
        swap(int, y1, y2)
    }

    /*
     * ILI948x specific commands
     */
    write_command(0x2a);   // Column Address Set
    write_word(x1>>8); //   Start column
    write_word(x1);
    write_word(x2>>8); //   End column
    write_word(x2);
    write_command(0x2b);   // Page Address Set
    write_word(y1>>8); // Start line
    write_word(y1);
    write_word(y2>>8); // End line
    write_word(y2);
    write_command(0x2c); 
}

/*
 * Resets the display area to the full display.
 * Be sure to swap the X and Y dimensions in case of 
 * landscape orientation.
 */
void MyLCD::reset_display_area()
{
    if (orient==PORTRAIT)
        set_display_area(0,0,DISPLAY_COLUMNS-1,DISPLAY_ROWS-1);
    else
        set_display_area(0,0,DISPLAY_ROWS-1,DISPLAY_COLUMNS-1);
}

void MyLCD::fillScr(uint16_t color)
{
    digitalWriteFast(CS_PIN, LOW);
    reset_display_area();
    fast_fill(color,((DISPLAY_COLUMNS)*(DISPLAY_ROWS)));
    
    digitalWriteFast(CS_PIN, HIGH);
}

void MyLCD::fillScr(uint8_t r, uint8_t g, uint8_t b)
{
  uint16_t color = ((r&248)<<8 | (g&252)<<3 | (b&248)>>3);
  fillScr(color);
}

void MyLCD::clrScr()
{
    fillScr(VGA_BLACK);
}

void MyLCD::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    front_color = (r & 0b11111000) << 8 | (g & 0b11111100) << 3 | (b & 0b11111000) >> 3;
}

void MyLCD::setColor(word color)
{
    front_color = color;
}

word MyLCD::getColor()
{
    return front_color;
}

void MyLCD::setBackColor(uint8_t r, uint8_t g, uint8_t b)
{
    back_color = (r & 0b11111000) << 8 | (g & 0b11111100) << 3 | (b & 0b11111000) >> 3;
    _transparent=false;
}

void MyLCD::setBackColor(uint32_t color)
{
    if (color==VGA_TRANSPARENT) {
        _transparent=true;
    } else {
        back_color = color;
        _transparent=false;
    }
}

word MyLCD::getBackColor()
{
    return back_color;
}

void MyLCD::drawPixel(int x, int y)
{
    digitalWriteFast(CS_PIN, LOW);
    set_display_area(x, y, x, y);
    set_pixel(front_color);
    digitalWriteFast(CS_PIN, HIGH);
}

void MyLCD::draw_hor_line(int x, int y, int len)
{
    if (len<0) {
        len = -len;
        x -= len;
    }
    
    digitalWriteFast(CS_PIN, LOW);
    set_display_area(x, y, x+len, y);
    fast_fill(front_color,len+1);
    digitalWriteFast(CS_PIN, HIGH);
}

void MyLCD::draw_vert_line(int x, int y, int len)
{
    if (len<0) {
        len = -len;
        y -= len;
    }

    digitalWriteFast(CS_PIN, LOW);
    set_display_area(x, y, x, y+len);
    fast_fill(front_color,len+1);
    
    digitalWriteFast(CS_PIN, HIGH);
}

void MyLCD::drawLine(int x1, int y1, int x2, int y2)
{
    if (y1==y2) {
        draw_hor_line(x1, y1, x2-x1);
    } else if (x1==x2) {
        draw_vert_line(x1, y1, y2-y1);
    } else {
        unsigned int  dx = (x2 > x1 ? x2 - x1 : x1 - x2);
        short     xstep =  x2 > x1 ? 1 : -1;
        unsigned int  dy = (y2 > y1 ? y2 - y1 : y1 - y2);
        short     ystep =  y2 > y1 ? 1 : -1;
        int       col = x1, row = y1;
      
        digitalWriteFast(CS_PIN, LOW);
        if (dx < dy) {
            int t = - (dy >> 1);
            while (true) {
                set_display_area (col, row, col, row);
                write_word (front_color);
                if (row == y2)
                    break;
                row += ystep;
                t += dx;
                if (t >= 0) {
                    col += xstep;
                    t   -= dy;
                }
            } 
        }
        else
        {
            int t = - (dx >> 1);
            while (true) {
                set_display_area (col, row, col, row);
                write_word (front_color);
                if (col == x2)
                    break;
                col += xstep;
                t += dy;
                if (t >= 0) {
                    row += ystep;
                    t   -= dx;
                }
            } 
        }
        digitalWriteFast(CS_PIN, HIGH);
    }
}

void MyLCD::drawRect(int x1, int y1, int x2, int y2)
{
    if (x1>x2)
    {
        swap(int, x1, x2);
    }
    if (y1>y2)
    {
        swap(int, y1, y2);
    }
    
    draw_hor_line(x1, y1, x2-x1);
    draw_hor_line(x1, y2, x2-x1);
    draw_vert_line(x1, y1, y2-y1);
    draw_vert_line(x2, y1, y2-y1);
}

void MyLCD::fillRect(int x1, int y1, int x2, int y2)
{
    if (x1>x2)
    {
        swap(int, x1, x2);
    }
    if (y1>y2)
    {
        swap(int, y1, y2);
    }
    digitalWriteFast(CS_PIN, LOW);
    set_display_area(x1, y1, x2, y2);
    fast_fill(front_color,((long(x2-x1)+1)*(long(y2-y1)+1)));
    digitalWriteFast(CS_PIN, HIGH);
}


void MyLCD::printChar(unsigned char c, int x, int y)
{
    byte i,ch;
    word j;
    word temp; 
    
    digitalWriteFast(CS_PIN, LOW);
    
    if (!_transparent) {
        if (orient==PORTRAIT) {
            set_display_area(x,y,x+cfont.x_size-1,y+cfont.y_size-1);
          
            temp=((c-cfont.offset)*((cfont.x_size/8)*cfont.y_size))+4;
            for(j=0;j<((cfont.x_size/8)*cfont.y_size);j++) {
                ch=cfont.font[temp];
                for(i=0;i<8;i++) {   
                    if((ch&(1<<(7-i)))!=0) {
                        set_pixel(front_color);
                    } else {
                        set_pixel(back_color);
                    }   
                }
                temp++;
            }
        } else {
          temp=((c-cfont.offset)*((cfont.x_size/8)*cfont.y_size))+4;
      
          for(j=0;j<((cfont.x_size/8)*cfont.y_size);j+=(cfont.x_size/8)) {
              set_display_area(x,y+(j/(cfont.x_size/8)),x+cfont.x_size-1,y+(j/(cfont.x_size/8)));
              for (int zz=(cfont.x_size/8)-1; zz>=0; zz--) {
                  ch=cfont.font[temp+zz];
                  for(i=0;i<8;i++) {
                      if((ch&(1<<i))!=0) {
                          set_pixel(front_color);
                      } else {
                          set_pixel(back_color);
                      }
                  }
              }
              temp+=(cfont.x_size/8);
            }
        }
    } else {
        temp=((c-cfont.offset)*((cfont.x_size/8)*cfont.y_size))+4;
        for(j=0;j<cfont.y_size;j++) {
            for (int zz=0; zz<(cfont.x_size/8); zz++) {
                ch=cfont.font[temp+zz]; 
                for(i=0;i<8;i++) { 
                    if((ch&(1<<(7-i)))!=0) {
                        set_display_area(x+i+(zz*8),y+j,x+i+(zz*8)+1,y+j+1);
                        set_pixel(front_color);
                    } 
                }
            }
            temp+=(cfont.x_size/8);
        }
    }
    
    digitalWriteFast(CS_PIN, HIGH);
}

void MyLCD::rotateChar(unsigned  char c, int x, int y, int pos, int deg)
{
    byte i,j,ch;
    word temp; 
    int newx,newy;
    double radian;
    radian=deg*0.0175;  
    
    digitalWriteFast(CS_PIN, LOW);
    
    temp=((c-cfont.offset)*((cfont.x_size/8)*cfont.y_size))+4;
    for(j=0;j<cfont.y_size;j++) 
    {
        for (int zz=0; zz<(cfont.x_size/8); zz++)
        {
            ch=cfont.font[temp+zz]; 
            for(i=0;i<8;i++)
            {  
                newx=x+(((i+(zz*8)+(pos*cfont.x_size))*cos(radian))-((j)*sin(radian)));
                newy=y+(((j)*cos(radian))+((i+(zz*8)+(pos*cfont.x_size))*sin(radian)));
          
                set_display_area(newx,newy,newx+1,newy+1);
                
                if((ch&(1<<(7-i)))!=0)   
                {
                    set_pixel(front_color);
                } 
                else  
                {
                    if (!_transparent)
                        set_pixel(back_color);
                }   
            }
        }
        temp+=(cfont.x_size/8);
    }
    digitalWriteFast(CS_PIN, HIGH);
}

void MyLCD::print(char *st, int x, int y, int deg)
{
    int stl, i;
    
    stl = strlen(st);
    
    if (orient==PORTRAIT)
    {
        if (x==RIGHT)
            x=(DISPLAY_COLUMNS)-(stl*cfont.x_size);
        if (x==CENTER)
            x=((DISPLAY_COLUMNS)-(stl*cfont.x_size))/2;
    }
    else
    {
        if (x==RIGHT)
            x=(DISPLAY_ROWS)-(stl*cfont.x_size);
        if (x==CENTER)
            x=((DISPLAY_ROWS)-(stl*cfont.x_size))/2;
    }
    
    for (i=0; i<stl; i++)
        if (deg==0)
            printChar(*st++, x + (i*(cfont.x_size)), y);
        else
            rotateChar(*st++, x, y, i, deg);
}

void MyLCD::print(char const *st, int x, int y, int deg)
{
    //char buf[st.length()+1];
    
    //st.toCharArray(buf, st.length()+1);
    print((char *)st, x, y, deg);
}

void MyLCD::printNumI(long num, int x, int y, int length, char filler)
{
    char buf[25];
    char st[27];
    boolean neg=false;
    int c=0, f=0;
    
    if (num==0) {
        if (length!=0) {
            for (c=0; c<(length-1); c++)
                st[c]=filler;
            st[c]=48;
            st[c+1]=0;
        } else {
            st[0]=48;
            st[1]=0;
        }
    } else {
        if (num<0) {
            neg=true;
            num=-num;
        }
        
        while (num>0) {
            buf[c]=48+(num % 10);
            c++;
            num=(num-(num % 10))/10;
        }
        buf[c]=0;
        
        if (neg)
        {
            st[0]=45;
        }
        
        if (length>(c+neg))
        {
            for (int i=0; i<(length-c-neg); i++)
            {
                st[i+neg]=filler;
                f++;
            }
        }
      
        for (int i=0; i<c; i++)
        {
            st[i+neg+f]=buf[c-i-1];
        }
        st[c+neg+f]=0;
    
    }
    
    print(st,x,y);
}

void MyLCD::_convert_float(char *buf, double num, int width, byte prec)
{
    char format[10];
    
    sprintf(format, "%%%i.%if", width, prec);
    sprintf(buf, format, num);
}

void MyLCD::printNumF(double num, byte dec, int x, int y, char divider, int length, char filler)
{
    char st[27];
    boolean neg=false;
    
    if (dec<1)
        dec=1;
    else if (dec>5)
        dec=5;
    
    if (num<0)
        neg = true;
    
    _convert_float(st, num, length, dec);
    
    if (divider != '.') {
        for (unsigned i=0; i<sizeof(st); i++)
            if (st[i]=='.')
                st[i]=divider;
    }
    
    if (filler != ' ') {
        if (neg) {
            st[0]='-';
            for (unsigned i=1; i<sizeof(st); i++)
                if ((st[i]==' ') || (st[i]=='-'))
                    st[i]=filler;
        } else {
            for (unsigned i=0; i<sizeof(st); i++)
                if (st[i]==' ')
                    st[i]=filler;
        }
    }
    
    print(st,x,y);
}

void MyLCD::setFont(uint8_t* font)
{
    cfont.font=font;
    cfont.x_size=cfont.font[0];
    cfont.y_size=cfont.font[1];
    cfont.offset=cfont.font[2];
    cfont.numchars=cfont.font[3];
}

uint8_t* MyLCD::getFont()
{
    return cfont.font;
}

uint8_t MyLCD::getFontXsize()
{
    return cfont.x_size;
}

uint8_t MyLCD::getFontYsize()
{
    return cfont.y_size;
}

/*
 * drawScope is a modified version of drawBitmap.
 * Instead of drawing a standard 16 bits bitmap, this interprets the XY matrix with intensities
 * for the XY display
 */
void MyLCD::draw_xy_scope(int x, int y, int sx, int sy, uint16_t *data)
{
    unsigned int col;
    int tx, ty, tc;

    if (orient==PORTRAIT) {
        digitalWriteFast(CS_PIN, LOW);
        set_display_area(x, y, x+sx-1, y+sy-1);
        for (tc=0; tc<(sx*sy); tc++) {
            col=pgm_read_word(&data[tc]);
            write_word(col);
        }
        digitalWriteFast(CS_PIN, HIGH);
    } else {
        digitalWriteFast(CS_PIN, LOW);
        for (ty=0; ty<sy; ty++) {
            set_display_area(x, y+sy-ty-1, x+sx-1, y+sy-ty-1);
            for (tx=sx-1; tx>=0; tx--) {
                //col=pgm_read_word(&data[(ty*sx)+tx]);
                col=pgm_read_word(&data[(tx*sy)+ty]);
                // (r & 0b11111000) << 8 | (g & 0b11111100) << 3 | (b & 0b11111000) >> 3;
                if(col > 255) col = 255;
                col = (col & 0b11111000) << 8 | (col & 0b11111100) << 3;
                /*
                 * Check and draw reticle
                 * only when no data at this point
                 */
#ifdef DRAW_RETICLE
                if(col == 0) {
                    // Check for reticle to be drawn
                    if(((tx+1)%(sx/10) == 0) && ((ty+1)%(sy/40) == 0)) col=0xffff;
                    else if(((tx+1)%(sx/50) == 0) && ((ty+1)%(sy/8) == 0)) col=0xffff;
                    else if((tx == 0) || (tx == (sx-1)) || (ty == 0) || (ty == (sy-1))) col=0xffff;
                    else if(((tx >= (sx/2-3))&&(tx <= (sx/2+1))) && ((ty+1)%(sy/40) == 0)) col = 0xffff;
                    else if(((ty >= (sy/2-3))&&(ty <= (sy/2+1))) && ((tx+1)%(sx/50) == 0)) col = 0xffff;
                }
#endif
                write_word(col);
            }
        }
        digitalWriteFast(CS_PIN, HIGH);
    }
}
