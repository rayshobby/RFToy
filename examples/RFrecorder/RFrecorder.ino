/*
 RFrecorder Demo for RFToy
 
 This demo shows how to use RFToy to decode
 RF signals from the remote control of a common
 wireless power socket, and play back the
 codes to simulate the remote control. The codes
 are stored in EEPROM. The demo supports up to
 7 codes, each consisting of a on signal, off
 signal, and timing.
 
 User Interface
 --------------
 At menu page:
 - Click S1: up
 - Click S2: select
 - Click S3: down
 - Hold S1:  play back 'off' code
 - Hold S2:  delete code
 - Hold S3:  play back 'on' code
 
 At code page:
 - Click S1: play back 'off' code
 - Click S2: go back to menu page
 - Click S3: play back 'on' code
 - Hold S1:  record 'off' code
 - Hold S2:  delete code
 - Hold S3:  record 'on' code
 
 Recording Code
 --------------
 When recording a code, make sure you plug in
 mini-USB cable, as the receiver required +5V.
 Also make sure to use a RF receiver matching
 the remote control's frequency (433MHz and
 315MHz are both common).
 
 When playing back, it's OK to use either USB
 power or 3V battery.
 
 Written by Jonathan Goldin @ Rayshobby LLC
 Nov 2014
 For details, visit http://rayshobby.net/rftoy
*/

#include <SPI.h>
#include <RCSwitch.h>
#include "U8glib.h"
#include <avr/eeprom.h>

// pin defines
#define RECEIVE_POWER_PIN 9
#define TRANSMIT_POWER_PIN 8
#define S1 4
#define S2 5
#define S3 6
#define TRANSMIT_PIN 7
#define RECEIVE_PIN 2

// for simplicity, assume 24-bit code, protocol 1
// the RCSwitch library supports more general code 
#define CODE_LENGTH 24
#define PROTOCOL 1

// UI states
#define MENU 0
#define CODE 1
#define RECORD_ON 2
#define RECORD_OFF 3
#define TRANSMIT_ON 4
#define TRANSMIT_OFF 5
#define DELETE 6

#define STATION_COUNT 7
#define BUTTON_COUNT 3

#define PAUSE 500

// eeprom defines
#define ON_ADDR 0
#define OFF_ADDR 4
#define DELAY_ADDR 8
#define S_SIZE 10

int station_selected = 0;
int mode = MENU;
int old_mode = CODE;
boolean redraw = false;

unsigned int signal_delay[STATION_COUNT];
unsigned long ons[STATION_COUNT];
unsigned long offs[STATION_COUNT];
String station[STATION_COUNT];

int button[]  = {S1,S2,S3};
byte buttonStatus[] = {HIGH,HIGH,HIGH};
boolean is_pressed[] = {false,false,false};
boolean is_long_press[] = {false,false,false};
unsigned long temp_t[] = {0,0,0};

// define I2D OLED object
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);
// define RCSwitch object
RCSwitch mySwitch = RCSwitch();

void clearEEPROM(){
  for(int i = 0; i < STATION_COUNT; i++){
    signal_delay[i] = 0;
    ons[i] = 0;
    offs[i] = 0;
    boolean erase = true;
    for(int j = 0; j < S_SIZE; j++){
      if(eeprom_read_byte((uint8_t*)(S_SIZE*i+j)) != 255){
        erase = false;
      }
    }
    if(erase){
      writeSignalEEPROM(i);
    }
  }
}
void writeSignalEEPROM(int i){
  eeprom_write_dword((uint32_t*)(S_SIZE*i+ON_ADDR),ons[i]);
  eeprom_write_dword((uint32_t*)(S_SIZE*i+OFF_ADDR),offs[i]);
  eeprom_write_word((uint16_t*)(S_SIZE*i+DELAY_ADDR),signal_delay[i]);
}

void readEEPROM(){
  for(int i = 0; i < STATION_COUNT; i++){
    ons[i] = eeprom_read_dword((uint32_t*)(S_SIZE*i+ON_ADDR));
    offs[i] = eeprom_read_dword((uint32_t*)(S_SIZE*i+OFF_ADDR));
    signal_delay[i] = eeprom_read_word((uint16_t*)(S_SIZE*i+DELAY_ADDR));
    station[i] = getStationName(ons[i],offs[i],signal_delay[i]);
  }
}

void setupButtons(){
  for(int i = 0; i < BUTTON_COUNT; i++){
    pinMode(button[i],INPUT);
    digitalWrite(button[i],buttonStatus[i]);
  }
}

void reset_buttons(){
    for(int i = 0; i < BUTTON_COUNT; i++){
      if(is_pressed[i]){buttonStatus[i] = HIGH;}
      is_pressed[i] = false;
      is_long_press[i] = false;
    }
}

void detect_buttons(){
  for(int i = 0; i < BUTTON_COUNT; i++){
    if(digitalRead(button[i]) == LOW){
      if(buttonStatus[i] == HIGH){
        temp_t[i] = millis();
        buttonStatus[i] = LOW;
      } else {
        if(millis() - temp_t[i] > 1000){
          is_long_press[i] = true;
          is_pressed[i] = true;
          temp_t[i] = millis();
        }
      }
    } else {
      if(buttonStatus[i] == LOW){
        buttonStatus[i] = HIGH;
        if(!is_long_press[i]){
          is_pressed[i] = true;
        }
        temp_t[i] = millis();
      }
    }
  }
}

void process_buttons(){
  detect_buttons();
  switch(mode){
    case MENU:
      if(is_pressed[0]){
        if(is_long_press[0]){
          setMode(TRANSMIT_OFF);
        } else {
          station_selected = (station_selected-1+STATION_COUNT)%STATION_COUNT;
          setMode(MENU);
        }
      }
      if(is_pressed[1]){
        if(is_long_press[1]){
          setMode(DELETE);
        } else {
          setMode(CODE);
        }
      }
      if(is_pressed[2]){
        if(is_long_press[2]){
          setMode(TRANSMIT_ON);
        } else {
          station_selected = (station_selected+1)%STATION_COUNT;
          setMode(MENU);
        }
      }
      break;
    case CODE:
      if(is_pressed[0]){
        if(is_long_press[0]){
          setMode(RECORD_OFF);
        } else {
          setMode(TRANSMIT_OFF);
        }
      }
      if(is_pressed[1]){
        if(is_long_press[1]){
          setMode(DELETE);
        } else {
          setMode(MENU);
        }
      }
      if(is_pressed[2]){
        if(is_long_press[2]){
          setMode(RECORD_ON);
        } else {
          setMode(TRANSMIT_ON);
        }
      }
      break;
    case DELETE:
      if(is_pressed[0]){
        setMode(old_mode);
      }
      if(is_pressed[2]){
        deleteCurrentStation();
        setMode(old_mode);
      }
      break;
  }
  reset_buttons();
}

String getStationName(unsigned long on,unsigned long off, unsigned int pLength){
  String s = "";
  s += getHex(on,6);
  s += getHex(off,6);
  s += getHex(pLength,4);
  s.toUpperCase();
  return s;
}

String getHex(unsigned long num,int chars){
  String s = "";
  for(int i = chars-1; i >= 0; i--){
    if(num == 0){
      s += "-";
    } else {
      s += String(((num >> i*4) & 0xF),HEX);
    }
  }
  return s;
}

void splash(){
    u8g.firstPage();
    do{
      uint8_t h;
      u8g.setFont(u8g_font_8x13);
      u8g.setFontRefHeightText();
      u8g.setFontPosTop();
      h = u8g.getFontAscent()-u8g.getFontDescent();
      u8g.drawStr(20, 0, F("RF Recorder"));
      u8g.setFont(u8g_font_6x12);
      u8g.drawStr(4, 18, F("http://rayshobby.net"));
      u8g.drawStr(10, 32, F("(click / hold)"));
      u8g.drawStr(10, 42, F("S1: <- / play off"));
      u8g.drawStr(10, 52, F("S2: OK / delete"));
      u8g.drawStr(10, 62, F("S3: -> / play on"));            
    } while (u8g.nextPage());
    // wait for any button press
    while(1) {
      if (!digitalRead(S1)) break;
      if (!digitalRead(S2)) break;
      if (!digitalRead(S3)) break;
    }
}
void menu(){
  uint8_t h;

  u8g.setFont(u8g_font_6x12);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  h = u8g.getFontAscent()-u8g.getFontDescent();
  for(int i = 0; i < STATION_COUNT; i++){
    if(i == station_selected){
      u8g.drawStr(0,i*h,">");
    }
    u8g.drawStr(u8g.getFontLineSpacing(),i*h,String(i+1).c_str());
    u8g.drawStr(u8g.getFontLineSpacing()*1.6,i*h,":");
    if(ons[i]==0 && offs[i]==0){
      u8g.drawStr(u8g.getFontLineSpacing()*2.5,i*h, "-");
    } else {
      u8g.drawStr(u8g.getFontLineSpacing()*2.5,i*h,station[i].c_str());
    }
  }
}
void code(){
  uint8_t h;
  u8g.setFont(u8g_font_6x12);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  h = u8g.getFontAscent()-u8g.getFontDescent();
  u8g.drawStr(0,0,  F("Click S1/S3: play"));
  u8g.drawStr(0,10, F("Hold  S1/S3: record"));
  u8g.drawStr(0,u8g.getHeight()-h, F("off    back/del    on"));
  u8g.drawStr(0,22, F("Code:"));
  u8g.setFont(u8g_font_8x13);
  if(ons[station_selected]==0 && offs[station_selected]==0){
    u8g.drawStr(0,44, "-");
  } else {
    u8g.drawStr(0,44,station[station_selected].c_str());
  }
}

void record(boolean on){
  digitalWrite(RECEIVE_POWER_PIN,HIGH);
  delay(PAUSE);
  mySwitch.enableReceive(0);  // Receiver on inerrupt 0 => that is pin #2

  u8g.firstPage();
  do{
    uint8_t h;
    u8g.setFont(u8g_font_8x13);
    u8g.setFontRefHeightText();
    u8g.setFontPosTop();
    h = u8g.getFontAscent()-u8g.getFontDescent();
    u8g.drawStr(28, 20, F("Recording"));
    if (on)
      u8g.drawStr(28, 32, F("On code"));
    else
      u8g.drawStr(28, 32, F("Off code"));
  } while (u8g.nextPage());
  unsigned long t = millis();
  while(millis() - t < 10000){
    if(mySwitch.available()){
      if(mySwitch.getReceivedValue() != 0){
        if(on){
          ons[station_selected] = mySwitch.getReceivedValue();
          signal_delay[station_selected] = mySwitch.getReceivedDelay();
        } else {
          offs[station_selected] = mySwitch.getReceivedValue();
          signal_delay[station_selected] = mySwitch.getReceivedDelay();
        }
      }
      mySwitch.resetAvailable();
      break;
    }
  }
  mySwitch.disableReceive();
  digitalWrite(RECEIVE_POWER_PIN,LOW);
//  pinMode(RECEIVE_PIN, OUTPUT);
//  digitalWrite(RECEIVE_PIN, LOW);
  setMode(CODE);
}
void transmit(boolean on){
  if(signal_delay[station_selected] != 0){
    digitalWrite(TRANSMIT_POWER_PIN,HIGH);
    delay(PAUSE);
    mySwitch.enableTransmit(TRANSMIT_PIN);
    mySwitch.setProtocol(PROTOCOL);
    mySwitch.setPulseLength(signal_delay[station_selected]*.98);
    if(on){
      if(ons[station_selected] != 0){
        mySwitch.send(ons[station_selected],CODE_LENGTH);
      }
    } else {
      if(offs[station_selected] != 0){
        mySwitch.send(offs[station_selected],CODE_LENGTH);
      }
    }
    mySwitch.disableTransmit();
    digitalWrite(TRANSMIT_POWER_PIN,LOW);
  }
  delay(PAUSE);
  setMode(old_mode);
}
void deleteCurrentStation(){
    signal_delay[station_selected] = 0;
    ons[station_selected] = 0;
    offs[station_selected] = 0;
    writeSignalEEPROM(station_selected);
    readEEPROM();
}
void deleteMenu(){
  uint8_t h;
  u8g.setFont(u8g_font_6x12);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  h = u8g.getFontAscent()-u8g.getFontDescent();
  u8g.drawStr(0, 6,  F("Confirm deletion of"));
  u8g.drawStr(0, u8g.getHeight()-h, F("no                yes"));
  u8g.drawStr(0, 16, F("code?"));
  u8g.setFont(u8g_font_8x13);
  if(ons[station_selected]==0 && offs[station_selected]==0){
    u8g.drawStr(0,36,"-");
  } else {
    u8g.drawStr(0,36,station[station_selected].c_str());
  }
}
void setup(){
  setupButtons();
  splash();
  Serial.begin(9600);
  pinMode(TRANSMIT_POWER_PIN,OUTPUT);
  pinMode(RECEIVE_POWER_PIN,OUTPUT);
  digitalWrite(TRANSMIT_POWER_PIN,LOW);
  digitalWrite(RECEIVE_POWER_PIN,LOW);

  redraw = true;
  clearEEPROM(); //only clears signals which each byte is 255
  readEEPROM();
}
void loop(){
  switch(mode){
    case MENU:
      u8g.firstPage();
      if(redraw){
        do{
          menu();
        } while(u8g.nextPage());
        redraw = false;
      }
      break;
    case CODE:
      u8g.firstPage();
      if(redraw){
        do{
          code();
        } while(u8g.nextPage());
        redraw = false;
      }
      break;
    case RECORD_ON:
      record(true);
      writeSignalEEPROM(station_selected);
      station[station_selected] = getStationName(ons[station_selected],offs[station_selected],signal_delay[station_selected]);
      break;
    case RECORD_OFF:
      record(false);
      writeSignalEEPROM(station_selected);
      station[station_selected] = getStationName(ons[station_selected],offs[station_selected],signal_delay[station_selected]);
      break;
    case TRANSMIT_ON:
      transmit(true);
      break;
    case TRANSMIT_OFF:
      transmit(false);
      break;
    case DELETE:
      u8g.firstPage();
      if(redraw){
        do{
          deleteMenu();
        } while(u8g.nextPage());
        redraw = false;
      }
      break;
  }
  process_buttons();
}
void setMode(int m){
  old_mode = mode;
  mode = m;
  redraw = true;
}
