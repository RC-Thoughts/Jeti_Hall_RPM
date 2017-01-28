/*
   --------------------------------------------------------
        Jeti Arduino RPM Sensor v 1.0
   --------------------------------------------------------

    Tero Salminen RC-Thoughts.com 2017 www.rc-thoughts.com

    Ability to set amount magnets (2 is recommended for balance)
    If for example headspeed is wanted you can set gear-ratio.

    Features:
    - Arduino Pro Mini 3.3V 8Mhz
    - Omnipolar Hall Sensor (SS451A Honeywell used in development)

   --------------------------------------------------------
    ALWAYS test functions thoroughly before use!
   --------------------------------------------------------
    Shared under MIT-license by Tero Salminen 2017
   --------------------------------------------------------
*/
 
String sensVersion = "v.1.0";

// Settings according models hardware
int magnets = 1; // How many magnets on your setup?
float gearRatio = 1; // Gear ratio, for example 172 main gear and 17 teeth pinion = 172/17 = 10.11765

// Do not touch below this
#include <SoftwareSerialJeti.h>
#include <JETI_EX_SENSOR.h>
unsigned int rpm;
int hall = 2;
int rps;
long interval;
int header = 0;
int lastbtn = 240;
int current_screen = 1;
int current_config = 0;

char temp[LCDMaxPos / 2];
char msg_line1[LCDMaxPos / 2];
char msg_line2[LCDMaxPos / 2];

#define prog_char char PROGMEM
#define GETCHAR_TIMEOUT_ms 20

#ifndef JETI_RX
#define JETI_RX 3
#endif

#ifndef JETI_TX
#define JETI_TX 4
#endif

#define JETI_SENSOR_ID1 0x11 // 0x11 for first, 0x12 for second, 0x13 for third

#define ITEMNAME_1 F("RPM")
#define ITEMTYPE_1 F("rpm")
#define ITEMVAL_1 (unsigned int*)&rpm 

#define ABOUT_1 F(" RCT Jeti Tools")
#define ABOUT_2 F("   RPM-Sensor")

SoftwareSerial JetiSerial(JETI_RX, JETI_TX);

void JetiUartInit()
{
  JetiSerial.begin(9700);
}

void JetiTransmitByte(unsigned char data, boolean setBit9)
{
  JetiSerial.set9bit = setBit9;
  JetiSerial.write(data);
  JetiSerial.set9bit = 0;
}

unsigned char JetiGetChar(void)
{
  unsigned long time = millis();
  while ( JetiSerial.available()  == 0 )
  {
    if (millis() - time >  GETCHAR_TIMEOUT_ms)
      return 0;
  }
  int read = -1;
  if (JetiSerial.available() > 0 )
  { read = JetiSerial.read();
  }
  long wait = (millis() - time) - GETCHAR_TIMEOUT_ms;
  if (wait > 0)
    delay(wait);
  return read;
}

char * floatToString(char * outstr, float value, int places, int minwidth = 0) {
  int digit;
  float tens = 0.1;
  int tenscount = 0;
  int i;
  float tempfloat = value;
  int c = 0;
  int charcount = 1;
  int extra = 0;
  float d = 0.5;
  if (value < 0)
    d *= -1.0;
  for (i = 0; i < places; i++)
    d /= 10.0;
  tempfloat +=  d;
  if (value < 0)
    tempfloat *= -1.0;
  while ((tens * 10.0) <= tempfloat) {
    tens *= 10.0;
    tenscount += 1;
  }
  if (tenscount > 0)
    charcount += tenscount;
  else
    charcount += 1;
  if (value < 0)
    charcount += 1;
  charcount += 1 + places;
  minwidth += 1;
  if (minwidth > charcount) {
    extra = minwidth - charcount;
    charcount = minwidth;
  }
  if (value < 0)
    outstr[c++] = '-';
  if (tenscount == 0)
    outstr[c++] = '0';
  for (i = 0; i < tenscount; i++) {
    digit = (int) (tempfloat / tens);
    itoa(digit, &outstr[c++], 10);
    tempfloat = tempfloat - ((float)digit * tens);
    tens /= 10.0;
  }
  if (places > 0)
    outstr[c++] = '.';
  for (i = 0; i < places; i++) {
    tempfloat *= 10.0;
    digit = (int) tempfloat;
    itoa(digit, &outstr[c++], 10);
    tempfloat = tempfloat - (float) digit;
  }
  if (extra > 0 ) {
    for (int i = 0; i < extra; i++) {
      outstr[c++] = ' ';
    }
  }
  outstr[c++] = '\0';
  return outstr;
}

JETI_Box_class JB;

unsigned char SendFrame()
{
  boolean bit9 = false;
  for (int i = 0 ; i < JB.frameSize ; i++ )
  {
    if (i == 0)
      bit9 = false;
    else if (i == JB.frameSize - 1)
      bit9 = false;
    else if (i == JB.middle_bit9)
      bit9 = false;
    else
      bit9 = true;
    JetiTransmitByte(JB.frame[i], bit9);
  }
}

#define MAX_SCREEN 1
#define MAX_CONFIG 1
#define COND_LES_EQUAL 1
#define COND_MORE_EQUAL 2

// setup functions
void setup()
{
  Serial.begin(9600);
  pinMode(hall, INPUT_PULLUP);
  Serial.println("");
  Serial.print("RC-Thoughts Hall RPM-Sensor "); Serial.println(sensVersion);
  Serial.println("design by Tero Salminen @ RC-Thoughts 2017");
  Serial.println("Free and open-source - Released under MIT-license");
  Serial.println("");
  Serial.println("Ready!");
  Serial.println("");
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  pinMode(JETI_RX, OUTPUT);
  JetiUartInit();
  JB.JetiBox(ABOUT_1, ABOUT_2);
  JB.Init(F("RCT"));
  JB.addData(ITEMNAME_1, ITEMTYPE_1);
  JB.setValueBig(1, ITEMVAL_1);
  do {
    JB.createFrame(1);
    SendFrame();
    delay(GETCHAR_TIMEOUT_ms);
  }
  while (sensorFrameName != 0);
  digitalWrite(13, LOW);
}

// Working loop
void loop()
{ 
  // Jeti Stuff
  unsigned long time = millis();
  SendFrame();
  time = millis();
  int read = 0;
  pinMode(JETI_RX, INPUT);
  pinMode(JETI_TX, INPUT_PULLUP);

  JetiSerial.listen();
  JetiSerial.flush();

  while ( JetiSerial.available()  == 0 )
  {
    if (millis() - time >  5)
      break;
  }

  if (JetiSerial.available() > 0 )
  { read = JetiSerial.read();
    if (lastbtn != read)
    {
      lastbtn = read;
      switch (read)
      {
        case 224 :
          break;
        case 112 :
          break;
        case 208 :
          break;
        case 176 :
          break;
      }
    }
  }

  if (current_screen != MAX_SCREEN)
    current_config = 0;
  header++;
  if (header >= 5)
  {
    JB.createFrame(1);
    header = 0;
  }
  else
  {
    JB.createFrame(0);
  }

  long wait = GETCHAR_TIMEOUT_ms;
  long milli = millis() - time; 
  if (milli > wait)
    wait = 0;
  else
    wait = wait - milli;
  pinMode(JETI_TX, OUTPUT);
  
  // RPM-Stuff
  interval = pulseIn(hall, HIGH) + pulseIn(hall, LOW);
  rps = 1000000UL / interval;
  if (rps == -1) {
    rpm = 0;
  }
  else {
    rpm = ((rps*60)/magnets)/gearRatio;
    if (rpm < 0) {
      rpm = 0;
    }
  }
  Serial.print(" RPM ");Serial.println(rpm);
}
