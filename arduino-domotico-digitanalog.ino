#include <VirtualWire.h>
//
#define INDIRIZZO  0
#define DATOa      1
#define DATOb      2
#define DATOc      3
//
#define indirMAESTRO   1234
#define indirCANTINA 1235
//
#define BYTEStoTX  8
#define RELE_ON     121
#define RELE_OFF    122
#define RELE_TOGGLE 123
#define READ_DATA   124
//
#define VELOCITAstd 500
//
int  INTERIlocali[4]={0,0,0,0};
byte BYTEradio[BYTEStoTX];
byte CIFR[]={223,205,228,240,43,146,241,//
	     87,213,48,235,131,6,81,26,//
	     70,34,74,224,27,111,150,22,//
	     138,239,200,179,222,231,212};

const unsigned long mask=0x0000FFFF;
//
const int rele_pin = 2;
const int led_pin = 13;      // led pin
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
  pinMode(rele_pin, OUTPUT);   
  pinMode(led_pin,OUTPUT);     
  digitalWrite(led_pin,LOW);  
  vw_set_tx_pin(transmit_pin);
  vw_set_rx_pin(receive_pin); 
  vw_setup(VELOCITAstd);              
  vw_rx_start();              
  Serial.begin(9600);
}
////////////////////////////////
// loop
////////////////////////////////
void loop(){
  if (vw_get_message(BYTEradio, &buflen)){
    // decifra
    decodeMessage();
    digitalWrite(led_pin,HIGH);
    // controlal mittente
    if (INTERIlocali[INDIRIZZO]==indirMAESTRO){
      // esegue comando
      switch (INTERIlocali[DATOa]){
      case RELE_ON:digitalWrite(rele_pin,HIGH);	txStato();break;
      case RELE_OFF:digitalWrite(rele_pin,LOW);	txStato();break;
      case RELE_TOGGLE:	digitalWrite(rele_pin,!digitalRead(rele_pin));txStato();break;
      case READ_DATA:txStato();	break;
      } 
    }
    delay(100);
    digitalWrite(led_pin,LOW);
  }
}

int arrotondaTemp(int valore){
  // valore: per 21,76 deve inserirsi il valore 2176
  // quindi moltiplicare per cento il valore restituito
  // come float
//  char c[];
//  itoa(valore,c,10);
  // gli ultimi due numero sono i decimali
//  byte numCar=sizeof(c);
  
}

void txStato(){
  // imposta il proprio nome come mittente
  INTERIlocali[INDIRIZZO]=indirCANTINA;
  // recupera valori
  int sensorVal = analogRead(temperPin);
  float voltage = (sensorVal / 1024.0) * 5.0;
  float temperature = (voltage - .5) * 10000;
  int temper=temperature;
  // valori in memoria
  INTERIlocali[DATOa]=analogRead(lightPin);
  INTERIlocali[DATOb]=temper;
  INTERIlocali[DATOc]=digitalRead(rele_pin) & 0xFF;
  // codifica in bytes
  encodeMessage();
  // tx
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
  delay(1000);
}

void decodeMessage(){
  // from byte to struct
  byte m=0;
  cipher(); // cifratura
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
  cipher(); // cifratura
}

void cipher(){
  for (byte n=0;n<8;n++){
    BYTEradio[n]=BYTEradio[n]^CIFR[n];
  }
}
