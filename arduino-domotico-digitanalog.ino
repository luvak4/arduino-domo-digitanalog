#include <VirtualWire.h>
//
#define INDIRIZZO  0
#define DATOa      1
#define DATOb      2
#define DATOc      3
//
#define PONTEsuGIU   1234
#define CIRC_CANTINA 1235
//
#define BYTEStoTX  8
#define RELE_ON     121
#define RELE_OFF    122
#define RELE_TOGGLE 123
#define READ_DATA   124

int  INTERIlocali[4]={0,0,0,0};
byte BYTEradio[BYTEStoTX];
byte CIFR[BYTEStoTX]={156,33,183,95,230,63,250,215};
const unsigned long mask=0x0000FFFF;

const int led_pin = 2;
const int receive_pin = 11;
const int transmit_pin = 12;
uint8_t buflen = BYTEStoTX;    //for rx
const int lightPin = A0;
const int temperPin = A1;
unsigned long millPrec=0;
////////////////////////////////
// setup
////////////////////////////////
void setup() {
  pinMode(led_pin, OUTPUT);    // led set pin
  vw_set_tx_pin(transmit_pin); // radio set tx pin
  vw_set_rx_pin(receive_pin);  // radio set rx pin
  vw_setup(1000);              // radio speed
  vw_rx_start();               // radio rx ON
  pinMode(13,OUTPUT);          // out pin 13
  digitalWrite(13,LOW);        // ... set low
  //Serial.begin(9600);
}

////////////////////////////////
// loop
////////////////////////////////
void loop(){    
  if (vw_get_message(BYTEradio, &buflen)){
    decodeMessage();
    if (INTERIlocali[INDIRIZZO]==PONTEsuGIU){
      switch (INTERIlocali[DATOa]){
      case RELE_ON:
        digitalWrite(led_pin,HIGH);
        txStato();
        break;
      case RELE_OFF:
        digitalWrite(led_pin,LOW);
        txStato();
        break;
      case RELE_TOGGLE:
        digitalWrite(led_pin,!digitalRead(led_pin));
        txStato();
        break;
      case READ_DATA:
        txStato();
        break;
      }
    } 
  }
}

void txStato(){
  INTERIlocali[INDIRIZZO]=CIRC_CANTINA;
  int sensorVal = analogRead(temperPin);
  float voltage = (sensorVal / 1024.0) * 5.0;
  float temperature = (voltage - .5) * 10000;
  int temper=temperature;
  INTERIlocali[DATOa]=analogRead(lightPin);
  INTERIlocali[DATOb]=temper;
  INTERIlocali[DATOc]=digitalRead(led_pin);
  encodeMessage();
  vw_rx_stop();
  delayForRadioRxAdj();
  vw_send((uint8_t *)BYTEradio,BYTEStoTX); // send to tx-radio
  vw_wait_tx();                    // wait until message is gone
  vw_rx_start();
}

void delayForRadioRxAdj(){
  // this delay is necessary because TX-KEYBOARD and
  // RX-DISPLAY are near and TX blind RX.
  // allows RX to listen regulary
  delay(500);
}

void decodeMessage(){
  // de-cifratura
  byte m=0;
  cipher();
  for (int n=0; n<4;n++){
    INTERIlocali[n]=BYTEradio[m+1];
    INTERIlocali[n]=INTERIlocali[n] << 8;
    INTERIlocali[n]=INTERIlocali[n]+BYTEradio[m];
    m+=2;
  }
}

void encodeMessage(){
  // from struct to byte
  byte m=0;
  for (int n=0; n<4;n++){
    BYTEradio[m]=INTERIlocali[n] & mask;
    INTERIlocali[n]=INTERIlocali[n] >> 8;
    BYTEradio[m+1]=INTERIlocali[n] & mask;
    m+=2;
  }
  cipher();
}

void cipher(){
  for (byte n=0;n<8;n++){
    BYTEradio[n]=BYTEradio[n]^CIFR[n];
  }
}
