/*
 nRF24Sender Demo for RFToy
 
 This demo shows how to use RFToy to make a
 wireless temperature sensor. This is the
 sender module which transmits the current
 temperature value to a receiver module. The
 demo uses the Mirf library.
 
 This demo uses a 100K resistor and 100K
 thermistor to form a simple temperature 
 sensor. Pin A1 is used to read the value. 
 The connection is:
 VCC->100K->A1->thermistor->GND
  
 Written by Jonathan Goldin @ Rayshobby LLC
 Nov 2014
 For details, visit http://rayshobby.net/rftoy
*/

#include <SPI.h>
#include <Mirf.h>
#include <nRF24L01.h>
#include <MirfHardwareSpiDriver.h>
#include <U8glib.h>

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);	// I2C / TWI 

static const float temps[] = {
// according to TDR datasheet: http://www.360yuanjian.com/product/downloadFile-14073-proudct_pdf_doc-pdf.html
// this table defines the reference resistance of the thermistor under each temperature value from -20 to 40 Celcius
940.885,  // -20 Celcius
888.148,  // -19 Celcius
838.764,
792.494,
749.115,
708.422,
670.228,
634.359,
600.657,
568.973,
539.171,
511.127,
484.723,
459.851,
436.413,
414.316,
393.473,
373.806,
355.239,
337.705,
321.140,
305.482,
290.679,
276.676,
263.427,
250.886,
239.012,
227.005,
217.106,
207.005,
198.530,
188.343,
179.724,
171.545,
163.780,
156.407,
149.403,
142.748,
136.423,
130.410,
124.692,
119.253,
114.078,
109.152,
104.464,
100.000,
95.747,
91.697,
87.837,
84.157,
80.650,
77.305,
74.115,
71.072,
68.167,
65.395,
62.749,
60.222,
57.809,
55.503,
53.300    // +40 Celcius
};

void setup(){
  Serial.begin(9600);
  Serial.println("begin.");
   
  /*
   Set ce and csn pins
   */
  
  Mirf.cePin = 17;
  Mirf.csnPin = 16;
  
  Mirf.spi = &MirfHardwareSpi;
  Mirf.init();
  
  /*
   * Configure reciving address.
   */
   
  Mirf.setRADDR((byte *)"clie1");
  
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
   
  /*
   * To change channel:
   * 
   * Mirf.channel = 10;
   *
   * NB: Make sure channel is legal in your area.
   */
   
  Mirf.config();
  
}

void loop(){
  long temp = analogRead(A1);
  Serial.println(temp);
  float resistance = (100.0*temp)/(1024.0-temp);
  temp = getTemp(resistance);
    
  Mirf.setTADDR((byte *)"serv1");
  
  Mirf.send((byte *)&temp);
  
  // OLED
  u8g.firstPage();
  do{
    uint8_t h;
    u8g.setFont(u8g_font_10x20);
    u8g.setFontRefHeightText();
    u8g.setFontPosTop();
    h = u8g.getFontAscent()-u8g.getFontDescent();
    u8g.drawStr(29,(u8g.getHeight()-h)/2,"SENDING");
  } 
  while(u8g.nextPage());

  while(Mirf.isSending()){
  }

  Serial.println("Finished sending");
  delay(10);
  unsigned long time = millis();
  while(!Mirf.dataReady()){
    //Serial.println("Waiting");
    if ( ( millis() - time ) > 1000 ) {
      Serial.println("Timeout on response from server!");
      return;
    }
  }
  int temp2;
  Mirf.getData((byte *) &temp2);
  
  if(temp == temp2){
    Serial.println("response matches");
  } else {
    Serial.println("response doesn't match");
  }
  
  delay(1000);  // keep the 'sending' message displayed on OLED for 1 sec
  u8g.firstPage();
  do{
  } while(u8g.nextPage());

  delay(2000);  // wait for 2 seconds till next transmission
} 

// search in TDR table for the closest resistance
// then do linear interpolation
long getTemp(float resistance){
  for(int i = 1; i < (sizeof(temps)/sizeof(long)); i++){
    if(resistance >= temps[i]){
      float temp = 10.0*(i-20);
      Serial.println(i);
      // keep in mind the thermistor is negatively correlated with temperature
      temp -= 10.0*(resistance-temps[i])/(temps[i-1]-temps[i]);
      Serial.println(temp);
      return (long)temp;
    }
  }
  return -40;
}


