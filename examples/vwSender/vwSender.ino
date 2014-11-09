/*
 VirtualWire Sender Demo for RFToy
 
 This demo shows how to use RFToy to make a
 wireless temperature sensor. This is the
 sender module which transmits the current
 temperature value to a receiver module. The
 demo uses the VirtualWire library.
 
 This demo also uses power down sleep and 
 watchdog timer to save battery life, for
 long-term monitoring.
 
 Written by Jonathan Goldin @ Rayshobby LLC
 Nov 2014
 For details, visit http://rayshobby.net/rftoy
*/

#include <SPI.h>
#include <Arduino.h>
#include <VirtualWire.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define pinReceiverPower    9
#define pinReceiver         2
#define pinTransmitterPower 8
#define pinTransmitter      7
#define pinTemp             A1
#define pinLED              13

volatile int wdt_counter = 0;

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


ISR(WDT_vect)
{
  wdt_counter ++;
}

void enterSleep(void)
{
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
  sleep_enable();
  /* Now enter sleep mode. */
  power_all_disable();
  sleep_mode();  
  /* The program will continue from here after the WDT timeout*/
  sleep_disable(); /* First thing to do is disable sleep. */ 
  /* Re-enable the peripherals. */
  power_all_enable();
}


void setup()
{
  // set serial pins (0 and 1) to low-power state
  pinMode(0, INPUT);
  digitalWrite(0, LOW);
  pinMode(1, INPUT);
  digitalWrite(1, LOW);

  pinMode(pinTransmitterPower, OUTPUT);
  digitalWrite(pinTransmitterPower, LOW);
  pinMode(pinReceiverPower, OUTPUT);
  digitalWrite(pinReceiverPower, LOW);
  
  pinMode(pinTransmitter, OUTPUT);
  digitalWrite(pinTransmitter, LOW);
  pinMode(pinReceiver, OUTPUT);
  digitalWrite(pinReceiver, LOW);
  pinMode(pinLED, OUTPUT);
  digitalWrite(pinLED, LOW);

  // Initialise the IO and ISR
  vw_set_tx_pin(pinTransmitter);
  vw_setup(2000);	 // Bits per sec
    /* Clear the reset flag. */
  MCUSR &= ~(1<<WDRF);
  /* In order to change WDE or the prescaler, we need to
   * set WDCE (This will allow updates for 4 clock cycles).
   */
  WDTCSR |= (1<<WDCE) | (1<<WDE);
  /* set new watchdog timeout prescaler value */
  //WDTCSR = 1<<WDP0 | 1<<WDP1 | 1<<WDP2;  // 2.0 seconds
  /* other options (see page 57 of ATmega328 datasheet) */
  //WDTCSR = 1<<WDP2;                      // 0.25 seconds
  //WDTCSR = 1<<WDP0 | 1<<WDP2;            // 0.5 seconds
  //WDTCSR = 1<<WDP1 | 1<<WDP2;            // 1.0 seconds
  //WDTCSR = 1<<WDP0 | 1<<WDP1 | 1<<WDP2;  // 2.0 seconds
  WDTCSR = 1<<WDP3;                      // 4.0 seconds
  //WDTCSR = 1<<WDP3 | 1<<WDP0;            // 8.0 seconds

  /* Enable the WD interrupt (note no reset). */
  WDTCSR |= _BV(WDIE);
  ADCSRA &= ~(1<<ADEN); //Disable ADC
  ACSR = (1<<ACD); //Disable the analog comparator
  DIDR0 = 0x3F; //Disable digital input buffers on all AD
  power_all_disable(); 
}

void loop()
{
  if(wdt_counter== 1){
  
    power_all_enable();
    ADCSRA |= (1<<ADEN); //Enable ADC
    long temp = analogRead(pinTemp);  // read temperature
    ADCSRA &= ~(1<<ADEN); //Disable ADC    
    float resistance = (100.0*temp)/(1024.0-temp);
    temp = getTemp(resistance);
    
    String s = String(temp);
    const char *msg = s.c_str();
    
    digitalWrite(pinTransmitterPower, HIGH);
    digitalWrite(pinLED, HIGH);
    delay(500);  // delay a bit for transmitter to power up
    vw_setup(2000);
    vw_send((uint8_t *)msg, strlen(msg));
    vw_wait_tx(); // Wait until the whole message is gone
    digitalWrite(pinLED, LOW);
    digitalWrite(pinTransmitterPower, LOW); // power down
    digitalWrite(pinTransmitter, LOW);
    wdt_counter = 0;
  }
  enterSleep();
}

// search in TDR table for the closest resistance
// then do linear interpolation
long getTemp(float resistance){
  for(int i = 1; i < (sizeof(temps)/sizeof(long)); i++){
    if(resistance >= temps[i]){
      float temp = 10.0*(i-20);
      // keep in mind the thermistor is negatively correlated with temperature
      temp -= 10.0*(resistance-temps[i])/(temps[i-1]-temps[i]);
      return (long)temp;
    }
  }
  return -40;
}

