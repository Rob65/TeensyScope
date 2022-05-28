#include <ADC.h>
#include <IntervalTimer.h>

#include "src/MyLCD/MyLCD.h"
#include "cli.h"

#define VERSION "0.1.0"

MyLCD lcd;

#define WIDTH 400
#define HEIGHT 320

#define XDIV 10
#define YDIV 8
#define SUBDIV 5

#define ADC_RESOLUTION    10      // Resolution in bits

/*
 * Use hardware oversampling of the ADC.
 * This means that after starting an ADC conversion, the ADC will take
 * the given number of samples and report the average of this as the final result.

 */
#define ADC_OVERSAMPLING  16      // Hardware oversampling, can be set to 0, 4, 8, 16 or 32

ADC *adc = new ADC();

IntervalTimer sampling_timer;
IntervalTimer decay_timer;

uint16_t pixel[WIDTH][HEIGHT];
uint16_t reticle[WIDTH][HEIGHT]; // for future use

uint16_t decay_val = 7;
uint16_t burn_start = 160;
uint16_t burn_inc = 40;
uint16_t burn_max = 512;

/*
 * CLI command functions
 * 
 * Please not that there is no parameter parsing that checks for correct parameters.
 * There is also no range checking on parameters.
 * Any parameter given is handed to the standard atoi() C-library function.
 */

void cmd_decay(int num_params, char *param[])
{
    if(num_params < 1) {
        Serial.println("Error: usage is decay <count>");
        return;
    }
    decay_val = atoi(param[0]);
}

void cmd_burn(int num_params, char *param[])
{
    if(num_params < 3) {
        Serial.println("Error: usage is decay <start> <incr> <max>");
        return;
    }
    burn_start = atoi(param[0]);
    burn_inc   = atoi(param[1]);
    burn_max   = atoi(param[2]);
}

void cmd_status(int num_params, char *parm[])
{
    Serial.printf("decay_val %d\n", decay_val);
    Serial.printf("burn %d %d %d\n\n", burn_start, burn_inc, burn_max);
}

void cmd_reset(int num_params, char *param[])
{
    Serial.println("Resetting system");
    SCB_AIRCR = 0x05FA0004;
}

void cmd_help(int num_params, char *param[])
{
    Serial.print("TeensyScope, version: ");
    Serial.println(VERSION);
    Serial.println();
    Serial.println("decay <val>              - Set the decay value at which the 'phosphor' will fade out");
    Serial.println("burn <start> <inc> <max> - Set the values for the burn-in of the 'phosphor'");
    Serial.println("status                   - Print the current burn and decay values");
    Serial.println("reset                    - Reset the Teensy, start over");
}

cli_command_t cli_commands[] = {
    {"decay", cmd_decay},
    {"burn", cmd_burn},
    {"status", cmd_status},
    {"reset", cmd_reset},
    {"?", cmd_help},
    {"\0", NULL}
};

int cnt=0; // Used to keep track of the display line number in the decay part of the interrupt

void sample() {
    int32_t x,y;
    
    digitalWriteFast(11,1); // Use pin 11 to measure the time spent in the interrupt

    /*
     * This is the part where we read the values from the ADC.
     * Note that the ADC has already been started so we only need to
     * wait for the conversion to be complete (whic hshould already be finished by now).
     * After reading the value, we trigger the ADC to start sampling again.
     * In this way, we do not have to wait for a conversion to finish, saving ~ 3.5 us
     */
     
    while(adc->adc0->isConverting() || adc->adc1->isConverting());
    x = adc->adc0->readSingle();
    y = adc->adc1->readSingle();

    adc->startSynchronizedSingleRead(0, 1); // Restart the ADC

    /*
     * For now (purely testing purposes) I implemented a fixed scaling
     * This should be replaced with the calibration procedure as used in the TeensyLogger
     *
     * For now this just scales the 0 - 3.3 V signal to the full scale of the scope display
     * (x = 0..400 and y = 0..320)
     */
    // Fixed scaling to go from 0..1024 to 0..400 for X and 0..320 for Y
    x *= 100;
    x /= 256; // x = x/2.56
    y *= 10;
    y /= 32; // y = y/3.2
    
    /*
     * Clip the X and Y values to fall inside of the display area.
     * We are leaving the border free to keep the white border around the image
     */
    if(x<1) x=1;
    if(x>398) x=398;
    if(y<1) y=1;
    if(y>318) y=318;

    /*
     * We now have a valid X,Y position inside the display image.
     * The pixel[x][y] matrix contains the brightness for each pixel
     * To mimic the analog phosphor style CRT display, 4 parameters are
     * being used:
     * - burn_start is the initial intensity of the pixel as soon the 'beam' hits the screen
     * - burn_inc   determines how fast the intensity of a pixel increases
     *              (a slow moving beam results in more light being emited by the phosphor
     * - burn_max   is the maximum intensity of the 'phosphor'
     *              This is being used to prevent a "burn in" situation where a pixel
     *              is never extinguished.
     *              When the maximum intensity has been reached, pixels around the current pixel
     *              will also be lit to increase the size of the dot/line in a similar way as 
     *              on a CRT.
     * - decay_val  Is the speed at which a pixel will extinguish again.
     *              This has to be slower than increasing the intensity so we only do this 
     *              one line at a time. At the default 100 us interrupt rate, this performs
     *              a full screen decay cycle in 3.2 ms
     */

    // Increase pixel intensity
    if(pixel[x][y] == 0) {
        pixel[x][y] = burn_start; // Initial value
    } else {
        pixel[x][y]+=burn_inc;   // Increment brightness when pixel is already lit
    }
    
    // Increase dot size when the maximum intensity has been reached
    if(pixel[x][y] > burn_max) {
        pixel[x-1][y-1] += burn_inc;
        pixel[x-1][y] += burn_inc;
        pixel[x-1][y+1] += burn_inc;
        pixel[x][y-1] += burn_inc;
        pixel[x][y+1] += burn_inc;
        pixel[x+1][y-1] += burn_inc;
        pixel[x+1][y] += burn_inc;
        pixel[x+1][y+1] += burn_inc;
    }
  
    // decay one line
    y=cnt;
    for(x=0; x < WIDTH; x++) {
        if(pixel[x][y] >= decay_val) {
            if(pixel[x][y] > burn_max) pixel[x][y] = burn_max;
            pixel[x][y] -= decay_val;
        }
        else {
            pixel[x][y] = 0;
        }
    }
    cnt++;
    if(cnt >= HEIGHT) cnt=0;
    
    digitalWriteFast(11,0);
}

void display(void)
{
    digitalWriteFast(10,1); // Use pin 10 to measure the time needed to write a full image
    lcd.draw_xy_scope(0, 0, WIDTH, HEIGHT, (uint16_t *)pixel);
    digitalWriteFast(10,0);
}

void setup()
{
    Serial.begin(115200);
  
    pinMode(14, INPUT); // A0 and A1 are the analog inputs for the X and Y channels
    pinMode(15, INPUT);

    adc->adc0->setResolution(ADC_RESOLUTION);
    adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
    adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED);
    adc->adc0->setAveraging(ADC_OVERSAMPLING);
    adc->adc1->setResolution(ADC_RESOLUTION);
    adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::HIGH_SPEED);
    adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::VERY_HIGH_SPEED);
    adc->adc1->setAveraging(ADC_OVERSAMPLING);
  
// Setup the LCD
    lcd.InitLCD();
    lcd.clrScr();
    Serial.println("Initialized");
  
    pinMode(11, OUTPUT); // Pins 10 and 11 are used for debugging
    pinMode(10, OUTPUT); // to check timing during development
  
    // Blue screen with black XY viewport
    lcd.setColor(0, 0, 255);
    lcd.fillRect(0,0, 479, 319);
    lcd.setColor(0,0,0);
    lcd.fillRect(1, 1, WIDTH-1, HEIGHT-1);

    adc->startSynchronizedSingleRead(0, 1); // start ADC, read A0 and A1 channels
    sampling_timer.begin(sample, 25); // Start sampling at 25 us interval
}

void loop()
{
  cli_loop();
  display();
  delay(10);
}
