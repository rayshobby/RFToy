/*
 nRF24Receiver Demo for RFToy
 
 This demo shows how to use RFToy to make a
 wireless temperature sensor. This is the
 receiver module which displays the received
 temperature value to OLED. The demo uses
 the Mirf library.
 
 Written by Jonathan Goldin @ Rayshobby LLC
 Nov 2014
 For details, visit http://rayshobby.net/rftoy
*/

#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include "U8glib.h"


U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI 

void setup(){
  Serial.begin(9600);
  Serial.println("begin.");

  Mirf.cePin = 17;
  Mirf.csnPin = 16;
  /*
   * Set the SPI Driver.
   */

  Mirf.spi = &MirfHardwareSpi;

  /*
   * Setup pins / SPI.
   */

  Mirf.init();

  /*
   * Configure reciving address.
   */

  Mirf.setRADDR((byte *)"serv1");

  /*
   * Set the payload length to sizeof(unsigned long) the
   * return type of millis().
   *
   * NB: payload on client and server must be the same.
   */

  Mirf.payload = sizeof(long);

  /*
   * Write channel and payload config then power up reciver.
   */

  Mirf.config();

  u8g.firstPage();
  do{
    uint8_t h;
    u8g.setFont(u8g_font_10x20);
    u8g.setFontRefHeightText();
    u8g.setFontPosTop();
    h = u8g.getFontAscent()-u8g.getFontDescent();
    u8g.drawStr(19,(u8g.getHeight()-h)/2,"LISTENING");
  } 
  while(u8g.nextPage());
}
String c = "New Signal.";
String f = "Decoding.";
void loop(){
  /*
   * A buffer to store the data.
   */

  byte data[Mirf.payload];

  /*
   * If a packet has been recived.
   *
   * isSending also restores listening mode when it 
   * transitions from true to false.
   */

  if(!Mirf.isSending() && Mirf.dataReady()){
    Serial.print("Got packet: ");

    /*
     * Get load the packet into the buffer.
     */

    Mirf.getData(data);

    long temp = ((int)data[3] << 24) + ((int)data[2] << 16) + ((int)data[1] << 8) + data[0];
    Serial.println(temp);
    double tmp = (double)temp/10;
    tmp = 10*(tmp*9/5+32);

    c = String(temp/10);
    c += ".";
    c += String(temp%10);
    f = String((long)tmp/10);
    f += ".";
    f += String((long)tmp%10);
    c += (char)(176);
    c += "C";
    f += (char)(176);
    f += "F";
    
   u8g.firstPage();
    do{
      u8g.setFont(u8g_font_10x20);
      u8g.setFontRefHeightText();
      u8g.setFontPosTop();
      u8g.drawStr(0,0,c.c_str());
      u8g.drawStr(0,25,f.c_str());
    } while(u8g.nextPage());
    /*
     * Set the send address.
     */


    Mirf.setTADDR((byte *)"clie1");

    /*
     * Send the data back to the client.
     */

    Mirf.send(data);

    /*
     * Wait untill sending has finished
     *
     * NB: isSending returns the chip to receving after returning true.
     */

    Serial.println("Reply sent.");
  }
}

