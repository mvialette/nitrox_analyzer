#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ADS1X15.h"
#include "RunningAverage.h"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
//#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //Define OLED display

// potentiometre read
const int analogPin = A0;

/*taille du tableau de destination définie*/
char destination [ 8 ] ;

ADS1115 ADS(0x48); // Define ADC address

const int calCount = 320;         //Calibration samples
const int readDelay = 3;          //ADC read delay between samples

#define RA_SIZE 20            //Define running average pool size
RunningAverage RA(RA_SIZE);   //Initialize Running Average

float currentmv = 0;              //Define ADC reading value
const float multiplier = 0.0625F; //ADC value/bit for gain of 2
float calValue = 0;               //Calibration value (%/mV)

float percentO2;                  //% O2

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600); // only needed for debugging

 // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
 
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.clearDisplay();        //CLS
  display.display();             //CLS
  display.setTextColor(WHITE);   //WHITE for monochrome
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println(F("  +++++++++++"));
  display.println(F("    START-UP"));
  display.println(F("   CALIBRATION"));
  display.println(F("  +++++++++++"));
  display.display();
  display.clearDisplay();        //CLS

  Serial.print("ADS1X15_LIB_VERSION: ");
  Serial.println(ADS1X15_LIB_VERSION);
  ADS.setGain(2);	// +/- 2.048 V, 	0.0625 mV (multiplier)
  
  ADS.begin();

  RA.clear();
  for(int i=0; i<= calCount; i++) 
  {
    float millivolts = 0;
    millivolts = ADS.readADC_Differential_0_1();  //Read differental voltage between ADC pins 0 & 1
    Serial.print("cal=");
    Serial.println(millivolts);
    RA.addValue(millivolts);
    delay(readDelay);
  }

   //Compute calValue
  currentmv = RA.getAverage();
  Serial.print(">>>>>>>> getAverage =");
  Serial.println(currentmv);
  
  currentmv = currentmv * multiplier;
  Serial.print(">>>>>>>> currentmv * multiplier =");
  Serial.println(currentmv);
  
  calValue = 20.9000/currentmv; // determine witch calValue (coefficient to apply air PpO2 = 20.9% O2)
  Serial.print(">>>>>>>> calValue =");
  Serial.println(calValue);

  display.display();             //CLS execute
  delay(2000); // Pause for 2 seconds
}

// the loop routine runs over and over again forever:
void loop() {
  //Running measurement
  RA.clear();
  for(int x=0; x<= RA_SIZE; x++) {
    float millivolts = 0;
    millivolts = ADS.readADC_Differential_0_1();
    RA.addValue(millivolts);
    delay(16);
  //Serial.println(millivolts*multiplier,3);    //mV serial print for debugging
  }
  
  currentmv = RA.getAverage();
  float tension_A0_A1 = ADS.toVoltage(currentmv);	// Valeur exprimée en volts, fonction de la valeur mesurée par l’ADC, et du gain sélectionné.
  
  display.print(F("PMU / MOD : "));
  currentmv = currentmv*multiplier;
  percentO2 = currentmv*calValue;   //Convert mV ADC reading to % O2
  

  /**
  Preview of what we can observe in serial monitor : 
  cal=-193.00

  >>>>>>>> getAverage =-196.00
  >>>>>>>> currentmv * multiplier =-12.25
  >>>>>>>> calValue =-1.71

  >>>>>>>> RA.getAverage() = -196.25, ADS.toVoltage(currentmv) =-0.01, multiplier=0.06, currentmv*multiplier=-12.27, calValue=-1.71, currentmv*calValue=20.93
  >>>>>>>> RA.getAverage() = -196.25, ADS.toVoltage(currentmv) =-0.01, multiplier=0.06, currentmv*multiplier=-12.27, calValue=-1.71, currentmv*calValue=20.93
  */
  // blue = center
  // white = border
  // RA.getAverage() = -196.00, ADS.toVoltage(currentmv) =-0.01, multiplier=0.06, currentmv*multiplier=-12.25, calValue=-1.70, currentmv*calValue=20.82

  Serial.print(">>>>>>>> RA.getAverage() = ");
  Serial.print(RA.getAverage());
  Serial.print(", ADS.toVoltage(currentmv) =");
  Serial.print(tension_A0_A1);
  Serial.print(", multiplier=");
  Serial.print(multiplier);
  Serial.print(", currentmv*multiplier=");
  Serial.print(currentmv);
  Serial.print(", calValue=");
  Serial.print(calValue);
  Serial.print(", currentmv*calValue=");
  Serial.println(percentO2);

  display.clearDisplay();   
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print(percentO2,1);
  display.println(" % O2");
  display.setCursor(0,17);
  display.print(currentmv,2);
  display.println(" mV");

  display.print(F("PMU / MOD : "));

  if(percentO2 == 0) 
  {
    display.print(F("N/A"));
  }else{
    dtostrf(((((1.6 / percentO2) * 100)-1)*10), 1, 0, destination);
    display.print(destination);
    display.print(F(" m"));
  }

  display.display();
}
