/* Temperature Controlled Decklighting
    Using 5050 RGB LEDs
    refactor from earlier code using Timelord library for sunrise/sunset
    and borrowing heavily from http://paulorenato.com/index.php/125
    Astronomical Garden Lights.

    Geoffrey Todd Lamke, MD

    Last modified  2017/04/18  :-)
*/

#include <DHT.h>         //include DHT library duplicate library error
#include <Wire.h>
#include <DS1307RTC.h>
#include <TimeLord.h>
#include <Time.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

/*DEBUGGING SETTINGS    *******************************************************************/
#define DEBUG 1  // Set to 1 to enable debug messages through serial port monitor
#define TIMEDRIFT 0 //Set to 1 enable systemtime and RTC sync in the loop

LiquidCrystal_I2C  lcd(0x3F, 2, 1, 0, 4, 5, 6, 7); // 0x3F is the I2C bus address For the Chinese Sensor
//2 x 16 display

// DHT11 Temperature and Humidity Sensors
#define DHTPIN 12        //define as DHTPIN the Pin 12 (13 activates LED)
#define DHTTYPE DHT11    //define the sensor used(DHT11)
#define REDPIN 5
#define GREENPIN 6
#define BLUEPIN 3
#define DELAY 5000     //Sets the global delay variable.

/* INSTANTIATE DHT OBJECT */
DHT dht(DHTPIN, DHTTYPE);

/*GLOBAL VARIABLES*/
// NONBLOCKING TIMER
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 5000;

byte sunTime[6]  = {}; //Byte Array for storing date to feed into Timelord
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

# if DEBUG == 1
  /* FIRE UP SERIAL */
  Serial.begin(9600);
  while (!Serial) { // wait for serial
    delay(200);
  }
#endif

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
  unsigned long currentMillis = millis();  //Nonblocking timer loop


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
      |___/|_||_| |_|
  */
  float h = dht.readHumidity();    // reading Humidity
  float t = (32 + (9 * (dht.readTemperature())) / 5); // read Temperature as Farenheight

  /*  Start the LCD printing temperatures
       _    ___ ___    _____     __  _  _
      | |  / __|   \  |_   _|   / / | || |
      | |_| (__| |) |   | |    / /  | __ |
      |____\___|___/    |_|   /_/   |_||_|
  */

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;
    Serial.println(currentMillis);
  }


  lcd.clear();
  lcd.home (); // set cursor to 0,0
  lcd.print("Temp: ");
  lcd.setCursor (6, 0);
  lcd.print(t, 1);            // One decimal place
  lcd.print((char)223);       //Prints the degree symbol

  lcd.setCursor (0, 1);       // go to start of 2nd line
  lcd.print("Hum: ");
  lcd.setCursor (6, 1);
  lcd.print(h, 1);            // One decimal place
  lcd.print((char)37);        // Prints the percent symbol




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

    /*Timelord functions
        _____ _           _            _
       |_   _(_)_ __  ___| |___ _ _ __| |
         | | | | '  \/ -_) / _ \ '_/ _` |
         |_| |_|_|_|_\___|_\___/_| \__,_|

    */
    // Set sunTime Array to today's DateUses the Time library to give Timelord the current date
    sunTime[3] = day();
    sunTime[4] = month();
    sunTime[5] = year();

    /* Sunrise: */
    tardis.SunRise(sunTime); // Computes Sun Rise.
    mSunrise = sunTime[2] * 60 + sunTime[1];  //Minutes after midnight that sunrise occurs
    String sunriseTime = (String)(sunTime[tl_hour]) + ":" + ((String) sunTime[tl_minute]);   //WOrking on how to make this double digits
    byte timeRise[] = {sunTime[tl_hour], sunTime[tl_minute]};

    //    Serial.println(timeSunrise[0]);
    //    Serial.println(timeSunrise[1]);

    /* Sunset: */
    tardis.SunSet(sunTime); // Computes Sun Set.
    mSunset = sunTime[2] * 60 + sunTime[1];
    String sunsetTime = ((String)sunTime[tl_hour]) + ":" + ((String) sunTime[tl_minute]);  //working on how to make double digits and get rid of it
    byte timeSet[] = {sunTime[tl_hour], sunTime[tl_minute]};

    //Current time array
    byte nowTime[] = {hour(), minute()};

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
       _    ___ ___    _____ ___ __  __ ___
      | |  / __|   \  |_   _|_ _|  \/  | __|
      | |_| (__| |) |   | |  | || |\/| | _|
      |____\___|___/    |_| |___|_|  |_|___|
    */

    delay(DELAY);
    lcd.clear();
    lcd.home ();                // set cursor to 0,0
    lcd.print("Rise:");
    printTimeLCD(timeRise);
    lcd.setCursor (0, 1);       // go to start of 2nd line
    lcd.print("Set: ");
    printTimeLCD(timeSet);
    lcd.setCursor (11, 0);
    lcd.print("Time: ");
    lcd.setCursor (11, 1);
    printTimeLCD(nowTime);

    //    printLCD2Digits(minute());




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
        ((dayMinNow >= (mSunrise - 10)) && (dayMinNow <= (mSunrise + 60)))) {
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
int print2digits(int number) {        //Serial Print 2 digits
  if (number >= 0 && number < 10) {
    Serial.print('0');
  }
  Serial.print(number);
}

void printLCD2Digits(int digits) {
  // utility function for digital clock display: prints leading 0
  if (digits < 10)
    lcd.print('0');
  lcd.print(digits);
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
void printTimeLCD(byte timearray[]) {
  if (timearray[0] < 10) {
    lcd.print("0");
  }
  lcd.print(timearray[0]);
  lcd.print(":");
  if (timearray[1] < 10) {
    lcd.print("0");
  }
  lcd.print(timearray[1]);
}


