/* Demo with 128x64 OLED display and multiple I2C encoders wired up. The sketch will auto-
 * detect up to 4 encoder on the first 4 addresses. Twisting will display text on OLED
 * and change neopixel color.
 * set USE_OLED to true t
 */

#include <Arduino.h>
#include <iostream>
#include <vector>
#include <SPI.h>
#include <Wire.h>
#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <config.h>
#include <DallasTemperature.h>
#include <OneWire.h>

#ifdef USE_OLED
#include <Adafruit_SH110X.h>
#include <Fonts/FreeSans9pt7b.h>
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#endif

// MANAGE BUTTONS ON OLED
uint8_t buttonState = 3;
bool btnPressed = false;
bool encBtnPressed = false;

// MANAGE OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
byte arrowX = 42;
byte arrowH = 6;
byte arrowW = 5;
byte arrowY1 = 0;
byte arrowY2 = 11;
byte arrowY3 = 22;
std::vector<std::vector<int>>
    triangleCoords{
        {arrowX, arrowH / 2, arrowX + arrowW, arrowY1, arrowX + arrowW, arrowY1 + arrowH, SH110X_WHITE},
        {arrowX, arrowY2 + (arrowH / 2), arrowX + arrowW, arrowY2, arrowX + arrowW, arrowY2 + arrowH, SH110X_WHITE},
        {arrowX, arrowY3 + (arrowH / 2), arrowX + arrowW, arrowY3, arrowX + arrowW, arrowY3 + arrowH, SH110X_WHITE},
        {(arrowX + arrowW) + arrowW, arrowY2 + (arrowH / 2), (arrowX) + arrowW, arrowY2, (arrowX) + arrowW, arrowY2 + arrowH, SH110X_WHITE}};

// PID PARAMS & FRIENDS
String currentTune = "P";
int8_t pValue = 10;
int8_t iValue = 10;
int8_t dValue = 10;

// THERMISTOR READINGS USING VOLDATE DIVIDING
double adcMax = 4096;
double Vs = 5;
double R1 = 100000.0; // voltage divider resistor value
double Beta = 3950.0; // Beta value
double To = 298.15;   // Temperature in Kelvin for 25 degree Celsius
double Ro = 100000.0; // Resistance of Thermistor at 25 degree Celsius
double Vout, Rt = 0;
float T, Tc, Tf = 0;
unsigned long bed_temp_check_previous = 0;

// DS18B20 CONTROLS
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress(heaterSensor);

// BED CONTROLS
double bedTargetTemp;
bool bedHeating = false;

// START UP THE ADAFRUIT ROTARY ENCODERS IF USING THEM
Adafruit_seesaw encoders[4];
seesaw_NeoPixel encoder_pixels[4] = {
    seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800),
    seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800),
    seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800),
    seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800)};

int32_t encoder_positions[] = {0, 0, 0, 0};
bool found_encoders[] = {false, false, false, false};

// PRETTY COLORS FOR NEOPIXEL ON ROTARY ENCOODER
uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

// SWITCH PID PARAMS FOR TUNING USING PUSH BUTTONS ON ADAFRUIT OLED FEATHERWING
void setButtonState(uint8_t btnInput)
{
  buttonState = btnInput;
  switch (btnInput)
  {
  case 0:
    currentTune = "P";
    encoders[0].setEncoderPosition(-pValue);
    break;

  case 1:
    currentTune = "I";
    encoders[0].setEncoderPosition(-iValue);
    break;

  case 2:
    currentTune = "D";
    encoders[0].setEncoderPosition(-dValue);
    break;

  case 3:
    currentTune = "Bed Temp";
    encoders[0].setEncoderPosition(-bedTargetTemp);
    break;

  default:
    break;
  }
  // display.setCursor(100, 0);
}

void setBedTempTarget()
{
#ifdef USE_OLED
  display.setCursor(70, 11);
  display.print("T");
  display.print(bedTargetTemp);
#endif
}

void activateHeadBed()
{
  bedHeating = true;
  digitalWrite(RELAY_PIN, HIGH);
}

void deActivateHeadBed()
{
  bedHeating = false;
  digitalWrite(RELAY_PIN, LOW);
}

float checkBedTemperature()
{
  // unsigned long currentMillis = millis();
  // if ((currentMillis - bed_temp_check_previous) >= BED_TEMP_CHECK)
  // {
  // unsigned long start = millis();
  // sensors.requestTemperatures();
  // unsigned long stop = millis();
  // start = millis();
  // sensors.setWaitForConversion(false); // makes it async
  // sensors.requestTemperatures();
  // sensors.setWaitForConversion(true);
  // stop = millis();
  // int resolution = 9;
  // delay(750 / (1 << (12 - resolution)));
  // float tempC = sensors.getTempCByIndex(0);
  // Tf = tempC * 9 / 5 + 32;

  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  //  Tc = sensors.getTempCByIndex(0);
  //  Tf = Tc * 9 / 5 + 32;
  //       Vout = analogRead(THERMISTOR_PIN) * Vs / adcMax;
  //       Rt = R1 * Vout / (Vs - Vout);
  //       T = 1 / (1 / To + log(Rt / Ro) / Beta); // Temperature in Kelvin
  //       Tc = T - 273.15;                        // Celsius
  //       Tf = Tc * 9 / 5 + 32;                   // Fahrenheit

#ifdef DEBUG_THERMISTOR
  Serial.println(Tf);
#endif

  if (bedTargetTemp > (Tf))
  {
    activateHeadBed();
  }
  else if (bedTargetTemp < (Tf))
  {
    deActivateHeadBed();
  }

  // bed_temp_check_previous = currentMillis;
  // }

  return Tf;
}

void setup()
{
  Serial.begin(115200);

  // wait for serial port to open
  while (!Serial)
    delay(10);

  // SET OUR RELAY TO SWITCH WHEN WE PULL THE PIN HIGH
  pinMode(RELAY_PIN, OUTPUT);

  // SET UP DS18B20_POWER_PIN
  pinMode(DS18B20_PIN, INPUT_PULLUP);
  sensors.begin();

#ifdef USE_OLED
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  // display.begin(0x3C, true); // Address 0x3C default
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }
  display.display();
  delay(500); // Pause for half second
  display.setRotation(2);
  // display.setFont(&FreeSans9pt7b);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
#endif

  for (uint8_t enc = 0; enc < sizeof(found_encoders); enc++)
  {
    // See if we can find encoders on this address
    if (!encoders[enc].begin(SEESAW_BASE_ADDR + enc) ||
        !encoder_pixels[enc].begin(SEESAW_BASE_ADDR + enc))
    {
#ifdef DEBUG_ENCODERS
      Serial.print("Couldn't find encoder #");
      Serial.println(enc);
#endif
    }
    else
    {
#ifdef DEBUG_ENCODERS
      Serial.print("Found encoder + pixel #");
      Serial.println(enc);
#endif

#ifdef DEBUG_ENCODERS
      uint32_t version = ((encoders[enc].getVersion() >> 16) & 0xFFFF);
      if (version != 4991)
      {
        Serial.print("Wrong firmware loaded? ");
        Serial.println(version);
        while (1)
          delay(10);
      }
      Serial.println("Found Product 4991");
#endif

      // use a pin for the built in encoder switch
      encoders[enc].pinMode(SS_SWITCH, INPUT_PULLUP);
      // get starting position
      encoder_positions[enc] = encoders[enc].getEncoderPosition();
#ifdef DEBUG_ENCODERS
      Serial.println("Turning on interrupts");
#endif
      delay(10);
      encoders[enc].setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
      encoders[enc].enableEncoderInterrupt();

#ifdef USE_ENCODER_NEOPIXEL
      encoder_pixels[enc].setBrightness(5);
      encoder_pixels[enc].show();
#endif

      found_encoders[enc] = true;
    }

    // SET THE INITIAL BUTTON STATE
    setButtonState(0);
  }

#ifdef DEBUG_ENCODERS
  Serial.println("Encoders started");
#endif
}

void loop()
{
#ifdef USE_OLED
  display.clearDisplay();
  uint16_t display_line = 1;
#endif

  for (uint8_t enc = 0; enc < sizeof(found_encoders); enc++) // LOOPS THROUGH HOWEVER MANY ENCODERS THERE ARE. NOT REALLY NEEDED IF YOU'RE ONLY USING ONE
  {
    if (found_encoders[enc] == false)
      continue;

    int8_t new_position = -encoders[enc].getEncoderPosition();

    //  did we move around?
    if (encoder_positions[enc] != new_position)
    {

      encoder_positions[enc] = new_position;

      if (buttonState == 0)
      {
        pValue = new_position;
      }
      else if (buttonState == 1)
      {
        iValue = new_position;
      }
      else if (buttonState == 2)
      {
        dValue = new_position;
      }
      else if (buttonState == 3)
      {
        bedTargetTemp = new_position;
      }

#ifdef USE_ENCODER_NEOPIXEL
      // change the neopixel color, mulitply the new positiion by 4 to speed it up
      encoder_pixels[enc].setPixelColor(0, Wheel((new_position * 4) & 0xFF));
      encoder_pixels[enc].show();
#endif
    }

#ifdef USE_OLED
    // PID VALUES
    display.setCursor(0, 0);
    display.print("P: ");
    display.println(pValue);
    display.setCursor(0, 11);
    display.print("I: ");
    display.println(iValue);
    display.setCursor(0, 22);
    display.print("D: ");
    display.println(dValue);

    // BED TEMP VALUES
    display.setCursor(70, 0);
    display.print("F ");
    display.print(Tf);
    display.setCursor(70, 11);
    display.print("T ");
    display.print(bedTargetTemp);

    // BED HEATING STATE
    if (bedHeating)
    {
      display.setCursor(70, 22);
      // display.setTextColor(SH110X_BLACK, SH110X_WHITE);
      display.print("HEATING");
    }
#endif

    // CHECK IF THE ROTARY ENCODER HAS BEEN PRESSED
    if (!encoders[enc].digitalRead(SS_SWITCH))
    {
      if (!encBtnPressed)
      {
        encBtnPressed = true;
        if (!bedHeating)
        {
          activateHeadBed();
        }
        else if (bedHeating)
        {
          deActivateHeadBed();
        }
      }

#ifdef DEBUG_ENCODERS
      Serial.print("Encoder #");
      Serial.print(enc);
      Serial.println(" pressed");
#endif
#ifdef USE_OLED
#ifdef DEBUG_ENCODERS
      display.print(" P");
#endif
#endif
    }
    else if (encoders[enc].digitalRead(SS_SWITCH))
    {
      encBtnPressed = false;
    }
  }

  // CHECK IF THE BUTTONS ON THE OLED HAVE BEEN PRESSED
#ifdef USE_OLED
  if (!digitalRead(BUTTON_C))
  {
    if (!btnPressed)
    {
      setButtonState(0);
    }
  }
  else if (!digitalRead(BUTTON_B))
  {
    if (!btnPressed)
    {
      setButtonState(1);
    }
  }
  else if (!digitalRead(BUTTON_A))
  {
    if (!btnPressed)
    {
      btnPressed = true;
      setButtonState(2);
    }
  }
  if (!digitalRead(BUTTON_C) && !digitalRead(BUTTON_A))
  {
    if (!btnPressed)
    {
      setButtonState(3);
      btnPressed = true;
    }
  }
  btnPressed = false;
#endif

  // CHECK BED TEMP
  // sensors.requestTemperatures();
  checkBedTemperature();
  float tempC = sensors.getTempCByIndex(0);
  Tf = tempC * 9 / 5 + 32;

#ifdef USE_OLED
  display.fillTriangle(triangleCoords[buttonState][0], triangleCoords[buttonState][1], triangleCoords[buttonState][2], triangleCoords[buttonState][3], triangleCoords[buttonState][4], triangleCoords[buttonState][5], triangleCoords[buttonState][6]);
  display.display();
#endif

  // don't overwhelm serial port
  // yield();
  delay(20);
}
