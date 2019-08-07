/* interrupt routine for Rotary Encoders
 * drives NeoPixels and TFT screen over SPI
 *  
 *  A conglomeration by Craig Marston of many examples freely available
 *  
   The average rotary encoder has three pins, seen from front: A C B
   Clockwise rotation A(on)->B(on)->A(off)->B(off)
   CounterCW rotation B(on)->A(on)->B(off)->A(off)
   and may be a push switch with another two pins, pulled low at pin 8 in this case

  Suited to an ATMega328

  encoderPos is the variable that alters with the Rotary Encoder

*/

#include <Adafruit_NeoPixel.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>

#define PIN             5 // Which pin on the Arduino is connected to the NeoPixels?
uint8_t NUMPIXELS =     7; // How many NeoPixels are attached to the Arduino?
// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
// NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
// NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint16_t LEDhue = 54610; // initially magenta (16-bit colour wheel)
uint8_t LEDsat  =   255; // full saturation (8-bit)
uint8_t LEDval  =    64; // quarter brightness (8-bit)

// usually the rotary encoders three pins have the ground pin in the middle
enum PinAssignments { // I don't yet understand the 'enum' function!!
  encoderPinA = 2,   // right (labeled DT on our decoder)
  encoderPinB = 3,   // left (labeled CLK on our decoder)
  rotEncButton = 4    // switch (labeled SW on our decoder)
// connect the +5v and gnd appropriately
};
volatile uint8_t encoderPos = 0;  // a counter for the dial
// needs to be 8-bit and volatile so as to be independent of the interrupts
// note: the 8-bit limit is due to the ATMega328 processor

uint8_t lastReportedPos = 1;   // change management
uint8_t encoderPosCheck = 1;    // also change management!
static boolean rotating = false;      // debounce management
// interrupt service routine vars
boolean A_set = false;
boolean B_set = false;
boolean rotEncButtonFlag = false; // variable for previous value of 'menu_item'
uint8_t menu_item = 1;

// === use these x-coords to position the groups of elements
uint8_t xH = 18;  // x-position 
uint8_t xS = 64;  // corresponding to
uint8_t xV = 105; // each group of items
uint8_t yHSV  = 120; // vertical pos. of bullets

#define TFT_CS     10
#define TFT_RST    9  
#define TFT_DC     8

// The TFT uses the RGB565 colour-space instead of the more familiar RGB88
// You can find converters online because it's a pain to suss out…
#define RED         0x001F
#define BLUE        0xF800
#define paleBLUE    0xDEFF
#define GREEN       0x07E0
#define paleGREEN   0xDFFB
#define YELLOW      0x07FF
#define ORANGE      0x04FF
#define paleYELLOW  0xBFFF
#define PURPLE      0x780F
#define CYAN        0xFFE0
#define paleCYAN    0xFFF7
#define MAGENTA     0xF81F
#define paleMAGENTA 0xFDFF  
#define lightBLUE   0xFD20
#define GREENYELLOW 0xAFE5
#define NAVY        0x000F
#define DARKGREEN   0x03E0
#define DARKCYAN    0x03EF
#define MAROON      0x7800     
#define OLIVE       0x7BE0
#define WHITE       0xFFFF
#define cementGREY  0xBDF7
#define LIGHTGREY   0xC618
#define steelGREY   0xE71C
#define DARKGREY    0x7BEF
#define BLACK       0x0000

// https://learn.adafruit.com/adafruit-mini-tft-0-dot-96-inch-180x60-breakout/wiring-test
// Option 1 (recommended): must use the hardware SPI pins
// (for UNO thats sclk = 13 and sid = 11) and pin 10 must be
// an output. This is much faster - also required if you want
// to use the microSD card (see the image drawing example)
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

// Option 2: use any pins but a little slower!
#define TFT_SCLK 13   // set these to be whatever pins you like!
#define TFT_MOSI 11   // set these to be whatever pins you like!
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);


// =====================================================================================================
// *°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°◊ Setup ◊°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*
// =====================================================================================================
void setup() {
  // Use this initializer if you're using TFT
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  tft.fillScreen(ST7735_BLACK);


  // ===== Keep all the static graphic elements within 'Setup' =====
  //tft.drawRoundRect(x,y,w,h,r,t); // bullets for menu options
  tft.drawRoundRect(xH,yHSV,13,6,2,DARKGREEN);
  tft.drawRoundRect(xS,yHSV,13,6,2,DARKGREEN);
  tft.drawRoundRect(xV,yHSV,13,6,2,DARKGREEN);

  rainbow();  // these icons are only drawn once and don't change
  satIcon();
  valIcon();
  delay(1000);
  
  pinMode(encoderPinA, INPUT_PULLUP);   // pulled-up internally
  pinMode(encoderPinB, INPUT_PULLUP);   // input is active when
  pinMode(rotEncButton, INPUT_PULLUP);  // pulled to ground by switch

  attachInterrupt(digitalPinToInterrupt(2), doEncoderA, CHANGE);  // pin2 int0

  attachInterrupt(digitalPinToInterrupt(3), doEncoderB, CHANGE);  // pin3 int1
                                                  // h/ware interrupts on ATMega328
  
  Serial.begin(9600);  // output
  strip.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
}


// =====================================================================================================
// *°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°◊ Main Loop ◊°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*°*
// =====================================================================================================
void loop() {

  //tft.invertDisplay(false);
  
  rotating = true;  // reset the debouncer
  
  if (lastReportedPos != encoderPos) 
  {
    Serial.print("Index:");
    Serial.println(encoderPos, DEC);
    lastReportedPos = encoderPos;
  }

  
  if (digitalRead(rotEncButton) == LOW )  
  {
    delay(5); // crude de-bounce
    
    if (menu_item == 3 && rotEncButtonFlag == false)
    {
      rotEncButtonFlag = true;
      menu_item = 1; // hue
      encoderPos = LEDhue;
    }
        
    if (menu_item == 2 && rotEncButtonFlag == false)
    {
      rotEncButtonFlag = true;
      menu_item = 3; // value
      encoderPos = LEDval; 
      
    }

    if (menu_item == 1 && rotEncButtonFlag == false)
    {
      rotEncButtonFlag = true;
      menu_item = 2; // saturation
      encoderPos = LEDsat;
    }
    
    
    strip.clear();
    strip.show();
    //delay(100);
    
    while (digitalRead(rotEncButton) == LOW ) 
    {
      strip.setPixelColor(menu_item, strip.gamma32(strip.ColorHSV(32768, 255, 192))); // cyan
      // using 'menu_item' alters the chosen pixel — handy for testing!
      strip.show();

      delay(250);
      strip.clear();
      strip.show();
    }
  }

  switch (menu_item) 
  {
    case 1:
      LEDhue = encoderPos; // hue is a 16-bit value
      // map to a 16-bit value map(value, fromLow, fromHigh, toLow, toHigh)
      LEDhue = map(LEDhue, 0, 255, 0, 65535); // this operation expands the 
                                              // 8-bit values to 16-bit
      // this means that the hue 'jumps' over quite a lot of values, but
      // you don't have to turn the encoder as much to cycle through!!
        
      rotEncButtonFlag = false;
      //tft.fillRoundRect(x,y,w,h,r,t); // Hue
      tft.fillRoundRect(xH,yHSV,13,6,2,DARKGREEN);
      tft.fillRoundRect(xS+1,yHSV+1,11,4,2,BLACK);  // the black covers over
      tft.fillRoundRect(xV+1,yHSV+1,11,4,2,BLACK);  // the other bullets
      break;
      
    case 2:
      LEDsat = encoderPos;
      rotEncButtonFlag = false;
      tft.fillRoundRect(xH+1,yHSV+1,11,4,2,BLACK);
      tft.fillRoundRect(xS,yHSV,13,6,2,DARKGREEN); 
      tft.fillRoundRect(xV+1,yHSV+1,11,4,2,BLACK);
      break;
      
    case 3:
      LEDval = encoderPos;
      rotEncButtonFlag = false;
      tft.fillRoundRect(xH+1,yHSV+1,11,4,2,BLACK);
      tft.fillRoundRect(xS+1,yHSV+1,11,4,2,BLACK); 
      tft.fillRoundRect(xV,yHSV,13,6,2,DARKGREEN);
      break;
  }
  
  
  for ( uint8_t i = 0; i<strip.numPixels(); i++) // cycle through each pixel
  {
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(LEDhue, LEDsat, LEDval))); // 
    strip.show();
  }

  if (encoderPosCheck != encoderPos)
  {
    uint8_t yh = LEDhue/937; // ============= hue slider
    // the above variable (16-bit, max 65535!) divided into 70 steps 
    //tft.fillRect(x,y,w,h,t)
    tft.fillRect(xH-5,0,23,80, BLACK); // x-position set in definitions
    tft.fillRect(xH-5,5+(70-yh),23,4,WHITE); // slider bar
    tft.fillRect(xH+1,6+(70-yh),11,2,BLACK);
    
    rainbowSlider(); // coloured vertical bar
  
    uint8_t ys = LEDsat/4; // =============== saturation histogram 
    // 8-bit value divided into 64 steps
    uint8_t ySV = 5;
    //tft.drawRect(x,y,w,h,t)
    tft.drawRect(xS-3,ySV,18,75, WHITE); // outline rectangle
    
    //tft.fillRect(x,y,w,h,t)
    tft.fillRect(xS,ySV+2,12,72,WHITE); // solid white
  
    tft.fillRect(xS,ySV+2,12,64-ys,BLACK); // black curtain that 
                                            // changes with the variable
    // this gives the impression that the white rectangle changes ;•)
    tft.drawRect(xS-2,ySV+67,16,3,BLACK);
    tft.drawRect(xS-2,ySV+71,16,3,BLACK);
  

    uint8_t yv = LEDval/4; // ================ value histogram
    // 8-bit value divided into 64 steps
    //uint8_t xv = 102; //105
    //tft.drawRect(x,y,w,h,t)
    tft.drawRect(xV-1,ySV,18,75, WHITE); // outline rectangle
    
    //tft.fillRect(x,y,w,h,t)
    tft.fillRect(xV+2,ySV+2,12,72,WHITE); 
  
    tft.fillRect(xV+2,ySV+2,12,64-yv,BLACK);  // black curtain that 
                                            // changes with the variable
    tft.drawRect(xV,ySV+67,16,3,BLACK);
    tft.drawRect(xV,ySV+71,16,3,BLACK);
  }

  encoderPosCheck = encoderPos; // this remembers the RotEnc position 
}




// Interrupt on A changing state
void doEncoderA(){
  // debounce
  if ( rotating ) delay (1);  // wait a little until the bouncing is done
  // Test transition, did things really change?
  if( digitalRead(encoderPinA) != A_set ) {  // debounce once more
    A_set = !A_set;
    // adjust counter + if A leads B
    if ( A_set && !B_set )
      encoderPos += 1;
    rotating = false;  // no more debouncing until loop() hits again
  }
}

// Interrupt on B changing state, same as A above
void doEncoderB(){
  if ( rotating ) delay (1);
  if( digitalRead(encoderPinB) != B_set ) {
    B_set = !B_set;
    //  adjust counter – 1 if B leads A
    if( B_set && !A_set )
      encoderPos -= 1;
    rotating = false;
  }
}

void rainbow()
{
  uint8_t x = xH+6;
  uint8_t y = 100;
  uint8_t r = 6;
  tft.fillCircle(x,y,r+7,RED);
  tft.fillCircle(x,y,r+4,CYAN);
  tft.fillCircle(x,y,r+2,MAGENTA);
  //tft.drawCircle(x,y,r,t)
  tft.drawCircle(x,y,r,paleMAGENTA);
  tft.drawCircle(x,y,r+2,BLUE);
  tft.drawCircle(x,y,r+4,GREEN);
  tft.drawCircle(x,y,r+5,YELLOW);
  tft.drawCircle(x,y,r+6,RED);
  tft.fillCircle(x,y,r-1,BLACK);
  
  uint8_t w = 2*(r+7)+1;
  uint8_t h = r+8;
  uint8_t xr = x-(r+7);
  //tft.fillRoundRect(x,y,w,h,r,t)
  tft.fillRect(xr,y,w,h,BLACK);
}

void satIcon() // colour-block
{
  //uint8_t x = xS+6; //64
  uint8_t y = 88;
  uint8_t w = 12;
  uint8_t h = 3;
  tft.fillRect(xS-6,y,w,h, RED);
  tft.fillRect(xS-6,y+h,w,h, GREEN);
  tft.fillRect(xS-6,y+(2*h),w,h, BLUE);
  tft.fillRect(xS-6,y+(3*h),w,h, YELLOW);
  tft.fillRect(xS-6+w,y,w,h, paleMAGENTA);
  tft.fillRect(xS-6+w,y+h,w,h, paleGREEN);
  tft.fillRect(xS-6+w,y+(2*h),w,h, paleBLUE);
  tft.fillRect(xS-6+w,y+(3*h),w,h, paleYELLOW);
  
}

void valIcon() // brightness symbol
{
  uint8_t x = xV+7; // 105
  uint8_t y = 94; // centre of circle
  uint8_t r = 3;
  uint8_t h = 6*r;
  uint8_t w = h;
  //tft.drawFastVLine(x,y,h,t);
  //tft.drawFastHLine(x,y,w,t); 
  tft.drawFastVLine(x,y-(h/2),h+1, WHITE);
  tft.drawFastHLine(x-(h/2),y,w+1, WHITE);
  //tft.drawLine(xi,yi,xj,yj,t);
  tft.drawLine(x-(2*r),y+(2*r),x+(2*r),y-(2*r),WHITE);
  tft.drawLine(x+(2*r),y+(2*r),x-(2*r),y-(2*r),WHITE);
  //tft.fillCircle(x,y,r,t)
  tft.fillCircle(x,y,r+2, BLACK);
  tft.fillCircle(x,y,r, WHITE); 
}


void rainbowSlider() // rainbow bar, FOR HUE SLIDER
{
      //tft.drawFastVLine(x,y,h,t)
    tft.drawFastVLine(xH+5,10,10, MAGENTA);
    tft.drawFastVLine(xH+5,20,10, BLUE);
    tft.drawFastVLine(xH+5,30,10, CYAN);
    tft.drawFastVLine(xH+5,40,10, GREEN);
    tft.drawFastVLine(xH+5,50,10, YELLOW);
    tft.drawFastVLine(xH+5,60,10, ORANGE);
    tft.drawFastVLine(xH+5,70,10, RED);
    
    tft.drawFastVLine(xH+6,10,12, MAGENTA);
    tft.drawFastVLine(xH+6,20,10, BLUE);
    tft.drawFastVLine(xH+6,30,10, CYAN);
    tft.drawFastVLine(xH+6,40,10, GREEN);
    tft.drawFastVLine(xH+6,50,10, YELLOW);
    tft.drawFastVLine(xH+6,60,10, ORANGE);
    tft.drawFastVLine(xH+6,70,10, RED);

    tft.drawFastVLine(xH+7,10,10, MAGENTA);
    tft.drawFastVLine(xH+7,20,10, BLUE);
    tft.drawFastVLine(xH+7,30,10, CYAN);
    tft.drawFastVLine(xH+7,40,10, GREEN);
    tft.drawFastVLine(xH+7,50,10, YELLOW);
    tft.drawFastVLine(xH+7,60,10, ORANGE);
    tft.drawFastVLine(xH+7,70,10, RED);
}
