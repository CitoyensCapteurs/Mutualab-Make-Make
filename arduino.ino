///////////////////
//   Includes    //
///////////////////

#include <avr/sleep.h>
#include <avr/power.h>
#include "printf.h"
#include <EmonLib.h>   // https://github.com/openenergymonitor/EmonLib
#include <LCD5110_Graph.h> //http://http://www.rinkydinkelectronics.com/download.php?f=LCD5110_Graph.zip

///////////////////////
//   CONFIGURATION   //
///////////////////////

#define DEBUG 0
#define SERIAL_BAUDRATE 57600

//Value text wich will change
String ConsoW = "Conso (W)";
String ConsoWh = "Conso (Wh)";

// Calibration factor for the intensity
double ICAL = 85.0;
uint8_t valeurs[40];

// Set this to 0 to enable voltage measurement.
// Else, set this to your mean voltage.
double VOLTAGE = 240.0;

// Number of samples over which the mean must be done for the current measurement
int NUMBER_SAMPLES_I = 1480;

// Pin for current measurement
const int CURRENT_PIN = A0;

// Pin for voltage measurement
const int VOLTAGE_PIN = 0;

// Pin for the green LED
const int GREEN_LED_PIN = 2;

// Pin for the red LED
const int RED_LED_PIN = 3;

// Pin for the LCD BackLight
const int BACKLIGHT_PIN = 8;


//external data (from other file)
extern uint8_t CitizenLogo[];
extern uint8_t SmallFont[];
extern uint8_t TinyFont[];
extern uint8_t MediumNumbers[];

// Energy Monitor object
EnergyMonitor emon1;


///////////////////////
//   Declarations    //
///////////////////////

#define TIMEOUT 250 // timeout in ms

// Struct to send RF data
typedef struct {
  int power;
  int voltage;
  int battery;
  unsigned long timer;
  long padding4;
  int padding2;
} PayloadTX;
PayloadTX nrf={0,0,0,0,0,0};

////////////////////////////////
//   Calculations             //
////////////////////////////////
int nb_ech =0; // Number of captations
int tot_ech =0; // 
float Conso_cumu = 0.0;


////////////////////////////////
//   Hardware configuration   //
////////////////////////////////

// LCD Definition 
LCD5110 myGLCD(13, 12, 11, 9, 10);

//////////////////////////////
//   Sleep configuration    //
//////////////////////////////

typedef enum { wdt_16ms = 0, wdt_32ms, wdt_64ms, wdt_128ms, wdt_250ms, wdt_500ms, wdt_1s, wdt_2s, wdt_4s, wdt_8s } wdt_prescalar_e;

void setup_watchdog(uint8_t prescalar);
void do_sleep(void);
unsigned long start, finished, elapsed;
//////////////////////////
//   Setup operation    //
//////////////////////////

void setup(void)
{

  Serial.begin(57600);
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println(F("/!\\ STARTING CitizenWatt Sensor"));
  Serial.println(F("//////////////////////////////"));
  Serial.println(F("//    CitizenWatt sensor    //"));
  Serial.println(F("//    citizenwatt.paris     //"));
  Serial.println(F("////////////////////////////// \n"));

  //Define "put" mode of each thing
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BACKLIGHT_PIN, OUTPUT);
  //startup's light
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);
  digitalWrite(BACKLIGHT_PIN, HIGH);

  //Start screen
  myGLCD.InitLCD(60);
  myGLCD.clrScr();
  myGLCD.drawBitmap(0, 0, CitizenLogo, 84, 48);
  myGLCD.update();
  myGLCD.setFont(TinyFont);
  myGLCD.print("Initialisation...", CENTER, 40);
  myGLCD.print("Le Mutualab presente", CENTER, 1);
  myGLCD.update();

  // Init EnergyMonitor, with calibration factor for R1 = 22 Ohms
  emon1.current(0, ICAL);
  //
  // Prepare sleep parameters
  //
  setup_watchdog(wdt_1s); // /!\ 8s sleeping
  //Calibration of the sensor 
  for (int i=0; i<9;i++){
    digitalWrite(GREEN_LED_PIN, HIGH);
    nrf.power = (int) (emon1.calcIrms(NUMBER_SAMPLES_I) * VOLTAGE);
    digitalWrite(GREEN_LED_PIN, LOW);
      do_sleep();

  }
  //Same but with a coolest screen :)
  for (int fg=0; fg<15; fg++) {
        digitalWrite(GREEN_LED_PIN, HIGH);
    nrf.power = (int) (emon1.calcIrms(NUMBER_SAMPLES_I) * VOLTAGE);
    digitalWrite(GREEN_LED_PIN, LOW);
   myGLCD.clrScr();
  myGLCD.setFont(SmallFont);
  myGLCD.print("Chargement", 3, 20);
  myGLCD.update();
  digitalWrite(GREEN_LED_PIN, HIGH);
    nrf.power = (int) (emon1.calcIrms(NUMBER_SAMPLES_I) * VOLTAGE);
    digitalWrite(GREEN_LED_PIN, LOW);
  myGLCD.clrScr();
  myGLCD.print("Chargement.", 3, 20);
  myGLCD.update();
  digitalWrite(GREEN_LED_PIN, HIGH);
    nrf.power = (int) (emon1.calcIrms(NUMBER_SAMPLES_I) * VOLTAGE);
    digitalWrite(GREEN_LED_PIN, LOW);
  myGLCD.clrScr();
  myGLCD.print("Chargement..", 3, 20);
  myGLCD.update();
  digitalWrite(GREEN_LED_PIN, HIGH);
    nrf.power = (int) (emon1.calcIrms(NUMBER_SAMPLES_I) * VOLTAGE);
    digitalWrite(GREEN_LED_PIN, LOW);
  myGLCD.clrScr();
  myGLCD.print("Chargement...", 3, 20);
  myGLCD.update();
  }
 
  delay(1500);
  myGLCD.clrScr();
  myGLCD.print("COMPLETE :)", CENTER, 20);
  myGLCD.update();
  delay(2000);

  if ( DEBUG )
    printf_begin();


  // Init Done, turn off LEDs.
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
      start=millis();
}

///////////////////////////////
//   Loop part of the code   //
///////////////////////////////

void loop(void)
{
 
  //
  // Read Current
  digitalWrite(BACKLIGHT_PIN, HIGH);
  digitalWrite(GREEN_LED_PIN, HIGH);
  nrf.power = (int) (emon1.calcIrms(NUMBER_SAMPLES_I) * VOLTAGE);
  digitalWrite(GREEN_LED_PIN, LOW);

  // Set voltage
  nrf.voltage = VOLTAGE;

  // Read Vcc
  nrf.battery = (int) emon1.readVcc();
  
  //
  // Calulation
  //
  int tot = 0;
  valeurs[35] = nrf.power;

  for (int i = 0; i < 35; i++) {
    valeurs[i] = valeurs[i + 1];
    tot = tot + valeurs[i];
  }
  //define the clock at start of mesures
    finished=millis();
  float h,m,s,ms;
  unsigned long over;
  elapsed=finished-start;
  h=int(elapsed/3600000);
  over=elapsed%3600000;
  m=int(over/60000);
  over=over%60000;
  s=int(over/1000);
  if (m == 30) {
    digitalWrite(RED_LED_PIN, HIGH);      // Funny things happen at 30min and 1 hour :)
  };
  if (h >= 1) {
    digitalWrite(BACKLIGHT_PIN, LOW);
    digitalWrite(RED_LED_PIN, LOW);
  };

  
  nb_ech++;
  if (nb_ech>=10){
  tot_ech+= nrf.power;
  Conso_cumu+=(float)nrf.power/3600;
  }
  //
  // Display
  //
  myGLCD.clrScr();
  myGLCD.drawRect(0, 0, 35, 20);
  
  for (int i = 0; i <= 35; i++)
  {
    myGLCD.drawLine(i , 20, i, 20 - map(valeurs[i], 0, 300, 0, 20));
  }
 
  //Display of the screen
  myGLCD.setFont(TinyFont);
  myGLCD.print(ConsoW, 0, 22);
  myGLCD.print(ConsoWh, 37, 0);
  myGLCD.print("Duree", 52, 22);
  myGLCD.printNumI((int)h, 52, 30);
  myGLCD.printNumI((int)m, 62, 30);
  myGLCD.printNumI((int)s, 72, 30);
  if (nrf.power >= 1000) {            //Transform Value of Instant conso into kiloWatt 
    nrf.power = nrf.power /1000;      //
    ConsoW = "Conso (kW)";            //Transform name of the value "W" into "kW"
  };
  if (Conso_cumu >= 1000) {           //
    Conso_cumu = Conso_cumu /1000;    // Same for the total consommation
    ConsoWh = "Conso (kWh)";          //
  };
    myGLCD.setFont(MediumNumbers);
  myGLCD.printNumI(nrf,.power /2, LEFT, 28);
  myGLCD.printNumF(Conso_cumu,2, 36,5 );
  myGLCD.setFont(TinyFont);
  myGLCD.print("mutualab.org", 37, 39); //Some advert for our association :)
    myGLCD.print(ConsoW, 0, 22);
  myGLCD.print(ConsoWh, 37, 0);
  myGLCD.update();

  if ( DEBUG )
  {
    Serial.print("|");
    Serial.print(nrf.power);
    Serial.print("\t");
    Serial.print("|");
    Serial.print("\t");
    Serial.print(nrf.voltage);
    Serial.print("\t");
    Serial.print("|");
    Serial.print("\t");
    Serial.print(nrf.battery);
    Serial.print("\t");
    Serial.println("|");

  }
  
  // Sleep the MCU.  The watchdog timer will awaken in a short while, and
  // continue execution here.
  delay(1000);

  // 100ms for the MCU to settle
  delay(100);

}

//////////////////////////
//   Sleep functions    //
//////////////////////////

// 0=16ms, 1=32ms,2=64ms,3=125ms,4=250ms,5=500ms
// 6=1 sec,7=2 sec, 8=4 sec, 9= 8sec

void setup_watchdog(uint8_t prescalar)
{
  prescalar = min(9, prescalar);
  uint8_t wdtcsr = prescalar & 7;
  if ( prescalar & 8 )
    wdtcsr |= _BV(WDP3);

  MCUSR &= ~_BV(WDRF);
  WDTCSR = _BV(WDCE) | _BV(WDE);
  WDTCSR = _BV(WDCE) | wdtcsr | _BV(WDIE);
}

ISR(WDT_vect)
{
}

void do_sleep(void)
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN); // sleep mode is set here
  sleep_enable();

  sleep_mode();                        // System sleeps here

  sleep_disable();                     // System continues execution here when watchdog timed out
}


//by Nino
