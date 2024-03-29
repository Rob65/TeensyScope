#include <ADC.h>
#include <IntervalTimer.h>

#include "src/MyLCD/MyLCD.h"
#include "cli.h"

#define VERSION "0.2.0"

MyLCD lcd;

/*
 * PIN definitions
 */
#define ENABLE_HYBRID_PIN  2
#define MODE_IC_PIN        3
#define MODE_OP_PIN        4

#define WIDTH 400   // Setting for the oscilloscope display
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
#define SAMPLING_INTERVAL 25      // microseconds

#define TRIGGER_IN         MODE_OP_PIN

/*
 * Trigger state modes
 *  INIT      - Waiting for the trigger signal to become inactive (HIGH)
 *  WAITING   - Waiting for a fallling edge on the trigger signal
 *  TRIGGERED - trigger activated the sampling
 *  DONE      - The recording period has finished 
 */
#define TRIGGER_START       0
#define TRIGGER_WAITING    1
#define TRIGGERED          2
#define TRIGGER_DONE       3

ADC *adc = new ADC();

IntervalTimer sampling_timer;
IntervalTimer decay_timer;

uint16_t pixel[WIDTH][HEIGHT];

/*
 * Parameters for XY display mode
 */
uint16_t decay_val = 3;
uint16_t burn_start = 160;
uint16_t burn_inc = 40;
uint16_t burn_max = 240;

/*
 * Parameters for time based display mode
 * Note that the samples_per_pixel parameters also is used to differentiate between
 * XY and time based mode. With samples_per_pixel set to 0, the scope uses XY display mode
 */

 uint32_t samples_per_pixel = 0;
 uint32_t sample_counter;
 uint32_t x_counter;
 uint8_t  trigger_state;

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

uint32_t op_time;
volatile uint8_t  sample_op_state;

/*
 * OPTIME command functions
 * ------
 * Sample TRIGGER_IN to determine the current OP-time setting is.
 * The interrupt function is called using the IntervalTimer and then walks
 * trough 4 phases:
 *    0: wait until the TRIGGER signal is HIGH (to finish the current OP cycle
 *    1: wait until the TRIGGER signal gets LOW
 *    2: as long as the TRIGGER is LOW, count the time
 *    3: finished, signal the 'calling' function that the measurement has completed.
 *
 * The cmd_optime function is called when the optime command is given.
 * This function initializes the measurement, starts the IntervalTimer and then
 * waits for the measurement to complete (i.e. reach phase 3)
 * This function will wait  max. 25 s for the measurement to complete so even at the
 * longest OP-time setting of approx. 10s, it will always detect the current OP cycle to
 * finish and a next one to complete.
 *
 * Note that sample_optime() is called in interrupt context so no higher level functions
 * should be called here.
 */
void sample_optime()
{
    switch(sample_op_state) {
        case 0: // wait for TRIGGER to get high
                if(digitalReadFast(TRIGGER_IN) == HIGH) {
                    sample_op_state = 1;
                }
                break;
        case 1: // wait for falling edge on TRIGGER_IN
                if(digitalReadFast(TRIGGER_IN) == LOW) {
                    op_time = 1;
                    sample_op_state = 2;
                }
                break;
        case 2: // measure TRIGGER_IN period
                if(digitalReadFast(TRIGGER_IN) == LOW) {
                    op_time++;
                } else {
                    sample_op_state = 3;
                }
                break;
        case 3: // Done
                break;
    }
}

void cmd_optime(int num_params, char *param[])
{
    unsigned long time;

    // Stop sampling the ADC and initialize measurement

    sampling_timer.end(); // Stop sampling the analog signals
    Serial.println("Starting OP-time measurement");

    sample_op_state = 0;
    time = millis();
    sampling_timer.begin(sample_optime, SAMPLING_INTERVAL);

    // Wait for the measurement to complete or for a timeout
    while(millis() < (time + 25000)) {
        if(sample_op_state == 3) {
            break;
        }
    }

    // Stop the measurement and display the result

    sampling_timer.end();

    if(sample_op_state == 3) {
        Serial.printf("OP-time = %1.2f ms\n", op_time * SAMPLING_INTERVAL / 1000.0);
    } else if(sample_op_state < 2){
        Serial.println("No falling edge on TRIGGER found");
    } else {
        Serial.println("TRIGGER stays low");
    }

    // Restart regular sampling function
    adc->startSynchronizedSingleRead(0, 1); // start ADC, read A0 and A1 channels
    sampling_timer.begin(sample, 25); // Start sampling at 25 us interval
}

/*
 * TIME display command functions
 *
 * The time command initialized the time base scope display.
 * If the display was already in timing mode, the previous timing will be replaced.
 * If the display was in XY mode, the display will switch from XY to timing mode.
 */

void cmd_time(int num_params, char *param[])
{
    uint32_t usec;

    if(num_params != 1) {
        Serial.println("Error: usage is time <msec/div>");
        return;
    }

    sampling_timer.end(); // Stop sampling while reconfiguring
    /*
     * Calculate how many samples we collect per vertical line of pixels on the LCD.
     * With a width of 400 pixels we have 40 pixels/div so the default 25 us/sample
     * results in 25 * 40 = 1000 us per division so we can calculate the number
     * of samples per line using a simple division.
     */
    usec = atoi(param[0]) * 1000;
    samples_per_pixel = usec / SAMPLING_INTERVAL / (WIDTH/10);
    //samples_per_pixel = usec / 10 / (WIDTH/10);
    sample_counter = 0;
    x_counter = 0;
    trigger_state = TRIGGER_START;

    Serial.printf("Timing set to %d samples/pixel\n", samples_per_pixel);

    memset(pixel, 0, sizeof(pixel)); // clear display

    sampling_timer.begin(sample, SAMPLING_INTERVAL); // Restart sampling
    //sampling_timer.begin(sample, 10); // Restart sampling
}

void cmd_xy(int num_params, char *param[])
{
    sampling_timer.end();
    samples_per_pixel = 0;
    memset(pixel, 0, sizeof(pixel));
    sampling_timer.begin(sample, SAMPLING_INTERVAL);
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
    Serial.println("optime                   - Measure the current OP-time in msec");
    Serial.println("time <msec>              - Set the scope in time based mode with msec/div");
    Serial.println("xy                       - Set the scope in XY display mode");
    Serial.println("reset                    - Reset the Teensy, start over");
}

cli_command_t cli_commands[] = {
    {"decay", cmd_decay},
    {"burn", cmd_burn},
    {"status", cmd_status},
    {"optime", cmd_optime},
    {"time", cmd_time},
    {"xy", cmd_xy},
    {"reset", cmd_reset},
    {"?", cmd_help},
    {"\0", NULL}
};

int cnt=0; // Used to keep track of the display line number in the decay part of the interrupt

void sample() {
    uint32_t x,y;
    
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
    if(samples_per_pixel == 0) {
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
         * We now have a valid X,Y position inside the XY display image.
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
    } else {
        /*
         * Time based mode
         * The sample_counter counts from 0 to samples_per_pixel
         * and the x_xounter is the X index in the pixel[x][y] matrix
         */
        uint32_t ch1, ch2;
        int trigger;
    digitalWriteFast(9,1);

        if(x_counter < 400) {
            // Only add a new pixel when the end of the display is not reached

            // Fixed scaling to go from 0..1024 to 0..400 for X and 0..320 for Y
            ch1 = x * 100;
            ch1 /= 512; // ch1 = x/5.12
            ch2 = y * 100;
            ch2 /= 512; // ch1 = x/5.12
            ch2 += 160;

            // ToDo: wait for trigger (TRIGGER_IN or ch1/ch2 level rise/fall)
            trigger = digitalReadFast(TRIGGER_IN);

            switch(trigger_state) {
                case TRIGGER_START:
                    if(trigger == HIGH) trigger_state = TRIGGER_WAITING;
                    break;
                case TRIGGER_WAITING:
                    if(trigger == HIGH)
                        break;
                    // Continue when trigger is LOW (falling edge detected)
                    trigger_state = TRIGGERED;
                    // immediately start recording data
                case TRIGGERED:
                    digitalWriteFast(9, 1);
                    if((ch1 > 0) && (ch1 < 319)) {
                        pixel[x_counter][ch1]   = 0b1111100000011111; // Red RRRRRGGGGGGBBBBB
                        pixel[x_counter][ch1+1] = 0b1111100000011111;
                    }
                    if((ch2 > 0) && (ch2 < 319)) {
                        pixel[x_counter][ch2]   = 0b1111111111100000; // Yellow (red + green)
                        pixel[x_counter][ch2]   = 0b1111111111100000;
                    }

                    if(++sample_counter == samples_per_pixel) {
                      x_counter++;
                      sample_counter = 0;
                    }
                    if(x_counter >= 400) trigger_state = TRIGGER_DONE;
                    digitalWriteFast(9, 0);
                    break;
            }
        }
    }
    
    digitalWriteFast(11,0);
}

void display(void)
{
    digitalWriteFast(10,1); // Use pin 10 to measure the time needed to write a full image
    if(samples_per_pixel == 0) {
        // XY display
        lcd.draw_xy_scope(0, 0, WIDTH, HEIGHT, (uint16_t *)pixel);
    } else {
        // Time based display

        if(x_counter == 400) {
            lcd.draw_scope(0, 0, WIDTH, HEIGHT, (uint16_t *)pixel);
            sample_counter = 0;
            trigger_state = TRIGGER_START;
            x_counter = 0;
            memset(pixel, 0, sizeof(pixel));
        }
    }
    digitalWriteFast(10,0);
}

void setup()
{
    Serial.begin(115200);

    pinMode(ENABLE_HYBRID_PIN, OUTPUT);
    digitalWrite(ENABLE_HYBRID_PIN, HIGH); // Hybrid mode is OFF
    pinMode(MODE_IC_PIN, INPUT);
    pinMode(MODE_OP_PIN, INPUT); // Direct input from ModeOP on Hybrid connector (pin 14)
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
    pinMode(9, OUTPUT);
    digitalWrite(9,0);
    digitalWrite(10,0);
    digitalWrite(11,0);
  
    // Blue screen with black XY viewport
    lcd.setColor(0, 0, 255);
    lcd.fillRect(0,0, 479, 319);
    lcd.setColor(0,0,0);
    lcd.fillRect(1, 1, WIDTH-1, HEIGHT-1);

    adc->startSynchronizedSingleRead(0, 1); // start ADC, read A0 and A1 channels
    sampling_timer.begin(sample, SAMPLING_INTERVAL); // Start sampling at 25 us interval
}

void loop()
{
  cli_loop();
  display();
  delay(10);
}
