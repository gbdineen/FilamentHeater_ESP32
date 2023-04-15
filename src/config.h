#ifndef CONFIG_H
#define CONFIG_H

// OLED STUFF IF YOU'RE USING ONE
#define USE_OLED
#ifdef USE_OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET -1    // Reset pin # (or -1 if sharing Arduino reset pin)
#define BUTTON_A 26
#define BUTTON_B 18
#define BUTTON_C 19
#endif
#define WIRE Wire

// ADAFRUIT ROTARY ENCODER STUFF IF YOU'RE USING THEM
#define USE_AF_ROTARY_ENCODERS
#ifdef USE_AF_ROTARY_ENCODERS
// #define DEBUG_ENCODERS
// #define USE_ENCODER_NEOPIXEL
#define SS_SWITCH 24          // this is the pin on the encoder connected to switch
#define SS_NEOPIX 6           // this is the pin on the encoder connected to neopixel
#define SEESAW_BASE_ADDR 0x36 // I2C address, starts with 0x36
#endif

// THERMISTOR & BED STUFF
// #define THERMISTOR_PIN 36
#define RELAY_PIN 17
#define BED_TEMP_CHECK 1000
// #define DEBUG_THERMISTOR
#define DS18B20_PIN 16 // OneWire data pin for DS18B20_PIN temp sensor

#endif