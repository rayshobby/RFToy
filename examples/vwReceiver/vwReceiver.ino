/*
 VirtualWire Receiver Demo for RFToy
 
 This demo shows how to use RFToy to make a
 wireless temperature sensor. This is the
 receiver module which displays the received
 temperature value to OLED. The demo uses
 the VirtualWire library.
 
 Because the RF Receiver only works with 5V,
 you need to plug in a mini-USB cable.
 
 Written by Jonathan Goldin @ Rayshobby LLC
 Nov 2014
 For details, visit http://rayshobby.net/rftoy
*/

#include <SPI.h>
#include <Arduino.h>
#include <VirtualWire.h>
#include <U8glib.h>


U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI 

void setup()
{
    Serial.begin(9600);	// Debugging only

    vw_set_rx_pin(2);
    vw_setup(2000);	 // Bits per sec

    vw_rx_start();       // Start the receiver PLL running
    
    u8g.firstPage();
    do{
      u8g.setFont(u8g_font_10x20);
      u8g.setFontRefHeightText();
      u8g.setFontPosTop();
      u8g.drawStr(0,0,"No Reading.");
    } while(u8g.nextPage());
    
}

void loop()
{
    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;

    if (vw_get_message(buf, &buflen)) // Non-blocking
    {
      String s = String(atof((char*)buf)/10);
      s+=(char)(176);
      s+="C";
      u8g.firstPage();
      do{
        u8g.setFont(u8g_font_10x20);
        u8g.setFontRefHeightText();
        u8g.setFontPosTop();
        u8g.drawStr(0,0,s.c_str());
         
      } while(u8g.nextPage());
    }
}
