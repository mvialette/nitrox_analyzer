// Items : 
// Board : Arduino NANO
// Screen : SSD1306 OLED 0.96" (128 * 64 px)
// Amplifier : ADS1115

// Others items
// Box
// 


// PINs for the SSD1306 display connected to I2C
// On an arduino UNO:
// SSD1306 OLED (SDA) to Arduino NANO (A4)
// SSD1306 OLED (SCL) to Arduino NANO (A5)
// SSD1306 OLED (VCC) to Arduino NANO (5V)
// SSD1306 OLED (GND) to Arduino NANO (GND)

// The diffrents imports

// Screen OLED Gesture
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ADS (Analog & Digital Systems) is for 15 bits resolution (0 to 32676)
// Without an ADS1115, Arduino Nano only have 10 bits resolution (0 to 1023)
#include <ADS1X15.h>

// To make an average of multiple values
#include <RunningAverage.h>
// end of the import section 

// Define constants
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

// Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_RESET -1

// The address of SSD1306 display is 0x3C and cas display 124 * 64
#define SCREEN_ADDRESS 0x3C  // 0x3C == 124 * 64

// The size of Running Average array size
// #define RA_SIZE_FOR_CALIBRATION 320    //Define running average pool size during calibration
#define RA_SIZE_FOR_CALIBRATION 10    //Define running average pool size during calibration
// #define RA_SIZE_FOR_LOOP 20            //Define running average pool size inside loop
#define RA_SIZE_FOR_LOOP 3            //Define running average pool size inside loop

// Define the instances
// can display 21 chars on 8 lines
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

ADS1115 ADS(0x48);
// GND	0x48 => adresse par défaut
// VDD	0x49
// SDA	0x4A
// SCL	0x4B

RunningAverage RA(RA_SIZE_FOR_LOOP);   //Initialize Running Average

const int readDelay = 3;          //ADC read delay between samples

const int potentiometrePin = A0;
int sensorReading;
int pourcentageValue;

float currentMillivolts = 0;              //Define ADC reading value
const float multiplier = 0.0625F; //ADC value/bit for gain of 2
float calibrationValueOfAir = 0;               //Calibration value (%/mV)
const float ppO2 = 20.9000F;

float percentO2;                  //% O2

float currentMillivoltsInLoop = 0;              //Define ADC reading value
float currentMillivoltsInLoopByMultiplier = 0;

/*taille du tableau de destination définie*/
char destination [ 8 ] ;

float currentMaxiPpO2Allowed = 1.6F;

bool debuggingMode = false;

void setup() {
  Serial.begin(9600);

  Serial.print(F("Nitrox Analyzer by Maxime VIALETTE"));

  //ADS.setGain(0);		// ± 6.144 volt (par défaut). À noter que le gain réel ici sera de « 2/3 », et non zéro, comme le laisse croire ce paramètre !
  //ADS.setGain(1);	// ± 4.096 volt
  ADS.setGain(2);	// ± 2.048 volt, 	0.0625 mV (multiplier)
  //ADS.setGain(4);	// ± 1.024 volt
  //ADS.setGain(8);	// ± 0.512 volt
  //ADS.setGain(16);	// ± 0.256 volt

  ADS.setMode(1);	// 0 = CONTINUOUS, 1 = SINGLE (default)

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

  if(debuggingMode) {
    Serial.print(F("ADS1X15_LIB_VERSION: "));
    Serial.println(ADS1X15_LIB_VERSION);
  }
  
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.

  display.clearDisplay();       //CLS
  display.display();            //CLS
  

  /*
  display.clearDisplay();       //CLS
  display.display();            //CLS
  display.setTextColor(WHITE);  //WHITE for monochrome
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(F("Nitrox"));
  display.setTextSize(1);
  //display.println(F("    Nitrox Analyzer"));
  display.println(F(""));
  display.println(F(""));
  display.println("by Maxime VIALETTE");
  display.display();
  */

  display.setTextColor(WHITE);  //WHITE for monochrome
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(F("====================="));
  display.println(F("=  Nitrox Analyzer  ="));
  display.println(F("=                   ="));
  display.println(F("=   Enjoy Diving    ="));
  display.println(F("=                   ="));
  display.println(F("=  Maxime VIALETTE  ="));
  display.println(F("=                   ="));
  display.println(F("====================="));
  display.display();                //CLS execute
  delay(2000);  // Pause for 2 seconds

  display.clearDisplay();       //CLS
  display.display();            //CLS
  delay(200);  // Pause for 2 seconds
  
  // this is not mandatory
  //display.setTextColor(WHITE);  //WHITE for monochrome
  //display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(F("====================="));
  display.println(F("=  Nitrox Analyzer  ="));
  display.println(F("=                   ="));
  display.println(F("=    CALIBRATION    ="));
  display.println(F("=                   ="));
  display.println(F("=     PENDING ...   ="));
  display.println(F("=                   ="));
  display.println(F("====================="));
  display.display();                //CLS execute
  delay(2000);  // Pause for 2 seconds

  if(debuggingMode) {
    // settings up the ADS 1115 module
    Serial.print(F("coeficient = "));
    Serial.print(multiplier);
    Serial.println(" float");
  }

  // We will clear the Running Average array
  RA.clear();

  for(int i=0; i<= RA_SIZE_FOR_CALIBRATION; i++) 
  {
    float millivolts = ADS.readADC_Differential_2_3();  //Read differental voltage between ADC pins 2 & 3

    if(debuggingMode) {
      Serial.print(F("Calibration number ("));
      Serial.print(i);
      Serial.print(F(") with value = "));
      Serial.print(millivolts);
      Serial.println(F(" mv"));
    }
    
    RA.addValue(millivolts);
    delay(readDelay);
  }

  // We get the average value of the X (calCount) times values registration
  currentMillivolts = RA.getAverage();

  if(debuggingMode) {
    Serial.println(F(""));
    
    Serial.print(F("Calibration finish getAverage value is = "));
    Serial.print(currentMillivolts);
    Serial.print(F(" with "));
    Serial.print(RA_SIZE_FOR_CALIBRATION);
    Serial.println(F(" mesures"));

    //Compute calibrationValue
    //currentMillivolts = currentMillivolts * multiplier;
    Serial.print(F("currentMillivolts ("));
    Serial.print(currentMillivolts);
    Serial.print(F(") * multiplier ("));
    Serial.print(multiplier);
    Serial.print(F(") = "));
    Serial.print(currentMillivolts * multiplier);
    Serial.println(F(" mv"));
  }

  calibrationValueOfAir = ppO2 / (currentMillivolts * multiplier); // determine witch calValue (coefficient to apply air PpO2 = 20.9% O2)

  if(debuggingMode) {
    Serial.print(F("calibrationValueOfAir == ppO2 ("));
    Serial.print(ppO2);
    Serial.print(F(") / (currentMillivolts ("));
    Serial.print(currentMillivolts);
    Serial.print(F(") * multiplier ("));
    Serial.print(multiplier);
    Serial.print(F(")) == "));
    Serial.println(calibrationValueOfAir);
    
    Serial.println(F(""));
    Serial.println(F("End of cablibration"));
  }
  
  display.clearDisplay();       //CLS
  display.display();            //CLS
  delay(200);  // Pause for 2 seconds

  // this is not mandatory
  //display.setTextColor(WHITE);  //WHITE for monochrome
  //display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(F("====================="));
  display.println(F("=  Nitrox Analyzer  ="));
  display.println(F("=                   ="));
  display.println(F("=    CALIBRATION    ="));
  display.println(F("=        IS         ="));
  display.println(F("=     FINISH OK     ="));
  display.println(F("=                   ="));
  display.println(F("====================="));
  display.display();                //CLS execute

  delay(500);  // Pause for 2 second
  display.clearDisplay();       //CLS
  display.display();            //CLS
}

void displayLabels(){
  display.setCursor(0, 0);
  display.setTextSize(2);
  display.println(F("  Nitrox  "));
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.print(F("   Nx "));
  display.setTextSize(1);
  display.setCursor(0, 45);
  display.print(F("PMU / MOD = "));
  display.setCursor(75, 40);
  display.setCursor(100, 45);
  display.println(F(" m"));
  display.setCursor(0, 57);
  display.println(F("Max PpO2 ="));
}

void loop() {
  //Running measurement
  RA.clear();
  for(int x=0; x<= RA_SIZE_FOR_LOOP; x++) {
    float millivolts = ADS.readADC_Differential_2_3();

    if(debuggingMode){
      Serial.print(F("New analyse number ("));
      Serial.print(x);
      Serial.print(F(") with value = "));
      Serial.print(millivolts);
      Serial.println(F(" mv"));
    }
    
    RA.addValue(millivolts);
    delay(readDelay);
  }

  // We get the average value of the X (calCount) times values registration
  currentMillivoltsInLoop = RA.getAverage();
  float tension_A2_A3 = ADS.toVoltage(currentMillivoltsInLoop);	// Valeur exprimée en volts, fonction de la valeur mesurée par l’ADC, et du gain sélectionné.

  if(debuggingMode){
    Serial.println(F(""));

    Serial.print(F("Analyse finish getAverage value is = "));
    Serial.print(currentMillivoltsInLoop);
    Serial.print(F(" with "));
    Serial.print(RA_SIZE_FOR_LOOP);
    Serial.println(F(" mesures"));

    //Compute calibrationValue
    //currentMillivoltsInLoop = currentMillivoltsInLoop * multiplier;
    Serial.print(F("currentMillivoltsInLoop ("));
    Serial.print(currentMillivoltsInLoop);
    Serial.print(F(") * multiplier ("));
    Serial.print(multiplier);
    Serial.print(F(") = "));
    Serial.print(currentMillivoltsInLoop * multiplier);
    Serial.print(F(" mv ==> * calibrationValueOfAir ("));
    Serial.print(calibrationValueOfAir);
    Serial.print(F(") =="));
  }  

  currentMillivoltsInLoopByMultiplier = currentMillivoltsInLoop * multiplier;
  percentO2 = (currentMillivoltsInLoopByMultiplier)*calibrationValueOfAir;   //Convert mV ADC reading to % O2

  if(debuggingMode){
    Serial.println(percentO2);

    Serial.print(F(">>>>>>>> RA.getAverage() = "));
    Serial.print(RA.getAverage());
    Serial.print(F(", ADS.toVoltage(RA.getAverage()) ="));
    Serial.print(tension_A2_A3);
    Serial.print(F(", multiplier="));
    Serial.print(multiplier);
    Serial.print(F(", currentMillivoltsInLoop*multiplier="));
    Serial.print(currentMillivoltsInLoopByMultiplier);
    Serial.print(F(", calibrationValueOfAir="));
    Serial.print(calibrationValueOfAir);
    Serial.print(F(", currentMillivoltsInLoopByMultiplier*calibrationValueOfAir="));
    Serial.println(currentMillivoltsInLoopByMultiplier*calibrationValueOfAir);

    Serial.print(F("percentO2=="));
    Serial.print(percentO2);
    Serial.print(F(" % O2"));
    Serial.print(F(", PMU / MOD : "));
  }

  dtostrf(((((1.6 / percentO2) * 100)-1)*10), 1, 0, destination);

  if(debuggingMode){
    Serial.print(destination);
    Serial.println(F(" m"));
  }

  //display.clearDisplay();       //CLS
  //display.display();            //CLS
  display.clearDisplay();       //CLS
  displayLabels();

  display.setTextSize(2);
  display.setCursor(75, 20);
  display.println(percentO2, 0);
  display.setCursor(75, 40);

  currentMaxiPpO2Allowed = 1.6F;

  if(percentO2 == 0) 
  {
    display.print(F("N/A"));
  } else{
    dtostrf(((((currentMaxiPpO2Allowed / percentO2) * 100)-1)*10), 1, 0, destination);
    display.print(destination);
  }
  
  display.setTextSize(1);
  display.setCursor(75, 57);
  display.print(currentMaxiPpO2Allowed,1);
  display.display(); //CLS
}