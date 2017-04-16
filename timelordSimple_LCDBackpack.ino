/* Temperature Controlled Decklighting
    Using 5050 RGB LEDs
    refactor from earlier code using Timelord library for sunrise/sunset
    and borrowing heavily from http://paulorenato.com/index.php/125
    Astronomical Garden Lights.

    Geoffrey Todd Lamke, MD

    Last modified  2017/04/16  :-)
*/

#include "DHT.h"         //include DHT library
#include <Wire.h>
#include <DS1307RTC.h>
#include <TimeLord.h>
#include <Time.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C  lcd(0x3F, 2, 1, 0, 4, 5, 6, 7); // 0x3F is the I2C bus address For the Chinese Sensor
//2 x 16 display



// DHT11 Temperature and Humidity Sensors
#define DHTPIN 13         //define as DHTPIN the Pin 13 used to connect the Sensor
#define DHTTYPE DHT11    //define the sensor used(DHT11)
#define REDPIN 5
#define GREENPIN 6
#define BLUEPIN 3
#define DELAY 15000     //Sets the global delay variable.

/* INSTANTIATE DHT OBJECT */
DHT dht(DHTPIN, DHTTYPE);

/*DEBUGGING SETTINGS    *******************************************************************/
#define DEBUG 1  // Set to 1 to enable debug messages through serial port monitor
#define TIMEDRIFT 0 //Set to 1 enable systemtime and RTC sync in the loop

/*GLOBAL VARIABLES*/
byte sunTime[]  = {0, 0, 0, 1, 1, 13}; //Byte Array for storing date to feed into Timelord
int dayMinNow, dayMinLast = -1, hourNow, hourLast = -1, minOfDay; //time parts to trigger various actions.
// -1 init so hour/min last inequality is triggered the first time around
int mSunrise, mSunset; //sunrise and sunset expressed as minute of day (0-1439)

//  TIMELORD VAR
TimeLord tardis;  //INSTANTIATE TIMELORD OBJECT
// Greensboro, NC  Latitude: 36.145833 | Longitude: -79.801728
float const LONGITUDE = -79.80;
float const LATITUDE = 36.14;
int const TIMEZONE = -4;

void setup() {
  pinMode(A2, OUTPUT);      //Setting the A2-A4 pins to output
  digitalWrite(A2, LOW);    //digital mode to power the RTC
  pinMode(A3, OUTPUT);      //without wires!
  digitalWrite(A3, HIGH);
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);

  // activate LCD module
  lcd.begin (16, 2); // for 16 x 2 LCD module
  lcd.setBacklightPin(3, POSITIVE);
  lcd.setBacklight(HIGH);


  /* FIRE UP SERIAL */
  Serial.begin(9600);
  while (!Serial) ; // wait for serial
  delay(200);

  /* FIRE UP DHT SENSOR */
  dht.begin();

  /* TimeLord Object Initialization */
  tardis.TimeZone(TIMEZONE * 60);
  tardis.Position(LATITUDE, LONGITUDE);

  //RTC Synchronization Setup
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if (timeStatus() != timeSet) {
    Serial.println("Unable to sync with the RTC");
  }
  else {
    Serial.println("RTC has set the system time");
  }
  blinkWhite();


}

void loop() {
  //Set the system time from the RTC if there is suspicion of drift
  //Empiric testing indicates that the time library is routinely syncing RTC to SYSTEM
# if TIMEDRIFT == 1
  Serial.print("SYS TIME = ");
  print2digits(hour());
  Serial.print(':');
  print2digits(minute());
  Serial.print(':');
  print2digits(second());
  Serial.println();
# endif

  tmElements_t tm; //Instantiate time object

  /* CHECK TIME AND SENSORS
    ___  _  _ _____
    |   \| || |_   _|
    | |) | __ | | |
    |___/|_||_| |_|  */
  float h = dht.readHumidity();    // reading Humidity
  float t = (32 + (9 * (dht.readTemperature())) / 5); // read Temperature as Farenheight

  /*  Start the LCD printing temperatures
     _    ___ ___
    | |  / __|   \
    | |_| (__| |) |
    |____\___|___/  */

  lcd.clear();
  lcd.home (); // set cursor to 0,0
  lcd.print("temp: ");
  lcd.setCursor (6, 0);
  lcd.print(t);

  lcd.setCursor (0, 1);       // go to start of 2nd line
  lcd.print("hum: ");
  lcd.setCursor (6, 1);
  lcd.print(h);

  // check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    Serial.println("Using Bogus Values");
    h = 100;
    t = 100;
  }
  if (timeStatus() == timeNeedsSync) {
    Serial.println("Hey, Time is out of Sync");
  };

  if (RTC.read(tm)) {
# if DEBUG == 1
    Serial.print("RTC TIME = ");
    print2digits(tm.Hour);
    Serial.print(':');
    print2digits(tm.Minute);
    Serial.print(':');
    print2digits(tm.Second);
    Serial.print(" Date = ");
    Serial.print(tmYearToCalendar(tm.Year));
    Serial.print('/');
    print2digits(tm.Month);
    Serial.print('/');
    print2digits(tm.Day);
    Serial.println();
#endif
    // Lets figure out where we are in the day in terms of minutes
    dayMinNow = (minute() + (hour() * 60));
#if DEBUG ==1
    Serial.print("Minutes into the current Day: ");
    Serial.println(dayMinNow);
#endif
    // Set sunTime Array to today's DateUses the Time library to give Timelord the current date
    sunTime[3] = day();
    sunTime[4] = month();
    sunTime[5] = year();

    /* Sunrise: */
    tardis.SunRise(sunTime); // Computes Sun Rise.
    mSunrise = sunTime[2] * 60 + sunTime[1];  //Minutes after midnight that sunrise occurs
    
    String sunriseTime = ((String)sunTime[tl_hour]) + ":" + ((String) sunTime[tl_minute]);


    /* Sunset: */
    tardis.SunSet(sunTime); // Computes Sun Set.
    mSunset = sunTime[2] * 60 + sunTime[1];
    String sunsetTime = ((String)sunTime[tl_hour]) + ":" + ((String) sunTime[tl_minute]);
//    int hrs = print2digits(int (sunTime[tl_hour]));   Trying to get this to print double digit sunrise and sunset time.  
//    Serial.println(hrs);                              Not sure why what object it's returning that's getting converted.


#if DEBUG ==1
    Serial.print("Sunrise: ");
    Serial.println(sunriseTime);
    Serial.print("Sunset: ");
    Serial.println(sunsetTime);
    Serial.print("SunriseMin: ");
    Serial.println(mSunrise);           // Print minutes after midnight that sunrise occurs
    Serial.print("SunsetMin: ");
    Serial.println(mSunset);            // Print minutes after midnight that sunrise occurs
    Serial.print("Min of daylight today: ");
    int mDaylight = (mSunset - mSunrise);
    Serial.println(mDaylight);          // Print minutes of daylight today
    Serial.print("Min until Sunrise: ");
    int mLeftTilDawn = (mSunrise - dayMinNow);
    Serial.println(mLeftTilDawn);              // Print Minutes Until
    Serial.print("Min until Sunset: ");
    int mLeftInDay = (mSunset - dayMinNow);
    Serial.println(mLeftInDay);              // Print Minutes Until Sunset
    int pctDayLeft = ((float(mLeftInDay) / float(mDaylight)) * 100);
    if (pctDayLeft > 100) {
      pctDayLeft = 100;
    };
    Serial.print("% of daylight left: ");
    Serial.println (pctDayLeft);
    Serial.print("Temp / Hum: ");    //print current environmental conditions
    Serial.print(t, 2);    //print the temperature
    Serial.print(" / ");
    Serial.println(h, 2);  //print the humidity
    Serial.println("**************************************");
#endif

    /*  Start the LCD printing temperatures
       _    ___ ___
      | |  / __|   \
      | |_| (__| |) |
      |____\___|___/  */

    delay(DELAY);
    lcd.clear();
    lcd.home ();                // set cursor to 0,0
    lcd.print("Sunrise: ");
    lcd.setCursor (10, 0);
    lcd.print(sunriseTime);
    lcd.setCursor (0, 1);       // go to start of 2nd line
    lcd.print("Sunset: ");
    lcd.setCursor (10, 1);
    lcd.print(sunsetTime);
 
    
//    lcd.print("% dayleft: ");
//    lcd.setCursor (12, 0);
//    lcd.print(pctDayLeft);
//
//    lcd.setCursor (0, 1);       // go to start of 2nd line
//    lcd.print("Sunset: ");
//    lcd.setCursor (12, 1);
//    lcd.print(mLeftInDay);

    /* SET THE COLOR SCHEME DEPENDING ON TEMPERATURE
        Currently set for summer and checking every 3 seconds which is excessive.
        this will lead to color bouncing at border temperatures and probably need debouncing.
        I can probably change this to switch or "functionalize" the color change
        and further I can have it automatically change colors based on summer or winter
        to save hassle and reuploading of code.
        Can base summer and winter on min of daylight to get solstice.
    */

    if
    (((dayMinNow >= (mSunset - 10)) && (dayMinNow <= (mSunset + 90)))
        ||
        ((dayMinNow >= (mSunrise - 30)) && (dayMinNow <= (mSunrise + 120)))) {
      if (t > 85) {
        analogWrite(REDPIN, 255);
        analogWrite(BLUEPIN, 0);
        analogWrite(GREENPIN, 0);
      }
      else if (t <= 85 && t >= 80) {
        analogWrite(GREENPIN, 255);
        analogWrite(BLUEPIN, 0);
        analogWrite(REDPIN, 255);
      }
      else if (t <= 80 && t >= 70) {
        analogWrite(GREENPIN, 150);
        analogWrite(BLUEPIN, 150);
        analogWrite(REDPIN, 0);
      }
      else if (t <= 70 && t >= 60) {
        analogWrite(GREENPIN, 255);
        analogWrite(BLUEPIN, 0);
        analogWrite(REDPIN, 0);
      }
      else if (t <= 60 && t >= 50) {
        analogWrite(GREENPIN, 0);
        analogWrite(BLUEPIN, 150);
        analogWrite(REDPIN, 150);
      }
      else {
        analogWrite(BLUEPIN, 255);
        analogWrite(REDPIN, 255);
        analogWrite(GREENPIN, 255);
      }
    }
    else {
      analogWrite(REDPIN, 0);
      analogWrite(BLUEPIN, 0);
      analogWrite(GREENPIN, 0);
    }
    delay(DELAY);
  }
}
int print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.print('0');
  }
  Serial.print(number);
}
void blinkWhite() {
  //BLINK WHITE LIGHTS STARTUP TO CONFIRM ACTIVITY
  analogWrite(REDPIN, 255);
  analogWrite(BLUEPIN, 255);
  analogWrite(GREENPIN, 255);
  delay(5000);
  analogWrite(REDPIN, 0);
  analogWrite(BLUEPIN, 0);
  analogWrite(GREENPIN, 0);
}


