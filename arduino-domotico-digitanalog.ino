#include <VirtualWire.h>
////////////////////////////////
// pins
////////////////////////////////
const int rele_pin     =  2;
const int led_pin_rx   = 13;
const int led_pin_tx   = 10;
const int receive_pin  = 11;
const int transmit_pin = 12;
const int lightPin     = A0;
const int temperPin    = A1;
////////////////////////////////
// indirizzi radio
////////////////////////////////
#define indirMAESTRO        1234 //rx
#define indirCANTINA        1235 //tx
////////////////////////////////
// comunicazione radio principale
////////////////////////////////
#define VELOCITAstd   500
#define INDIRIZZO       0
#define DATOa           1
#define DATOb           2
#define DATOc           3
#define BYTEStoTX       8
int     INTERIlocali[4]={0,0,0,0};
byte    BYTEradio[BYTEStoTX];
uint8_t buflen = BYTEStoTX; //for rx
////////////////////////////////
// comandi da decodificare
////////////////////////////////
#define RELE_ON     121
#define RELE_OFF    122
#define RELE_TOGGLE 123
#define READ_DATA   124
////////////////////////////////
// varie
////////////////////////////////
byte CIFR[]={223,205,228,240,43,146,241,//
	     87,213,48,235,131,6,81,26,//
	     70,34,74,224,27,111,150,22,//
	     138,239,200,179,222,231,212};
const unsigned long mask=0x0000FFFF;
#define SALITA  1
#define DISCESA 0
unsigned long tempo;
int  temperVALattuale;
int  temperVALprecedente;
bool temperSTAattuale; //1=RAISE 0=FALL
bool temperSTAprecedente; //1=RAISE 0=FALL
int  temperMINUTIstato;
//
int  luceVALattuale;
int  luceVALprecedente;
bool luceSTAattuale; //1=RAISE 0=FALL
bool luceSTAprecedente; //1=RAISE 0=FALL
int  luceMINUTIstato;
////////////////////////////////
// setup
////////////////////////////////
void setup() {
  pinMode(rele_pin, OUTPUT);
  pinMode(led_pin_rx,OUTPUT);
  pinMode(led_pin_tx,OUTPUT);  
  digitalWrite(led_pin_rx,LOW);
  digitalWrite(led_pin_tx,LOW);
  vw_set_tx_pin(transmit_pin);
  vw_set_rx_pin(receive_pin); 
  vw_setup(VELOCITAstd);              
  vw_rx_start();
  tempo=millis();
  Serial.begin(9600); // debug
}

//
////////////////////////////////
// loop
////////////////////////////////
void loop(){
  ////////////////////////////////
  // ogni minuto
  ////////////////////////////////
  if ((millis()-tempo)>5000){
    tempo=millis();
    chkTemperatura();
    Serial.println(temperVALattuale);
    Serial.println(temperSTAattuale);
    Serial.println(temperMINUTIstato);
  }
  ////////////////////////////////
  if (vw_get_message(BYTEradio, &buflen)){
    vw_rx_stop();
    // decifra
    decodeMessage();
    // controlla mittente
    switch (INTERIlocali[INDIRIZZO]){
    case indirMAESTRO:
      digitalWrite(led_pin_rx,HIGH);
      // esegue comando
      switch (INTERIlocali[DATOa]){
      case RELE_ON:digitalWrite(rele_pin,HIGH); txStato();break;
      case RELE_OFF:digitalWrite(rele_pin,LOW); txStato();break;
      case RELE_TOGGLE: digitalWrite(rele_pin,!digitalRead(rele_pin));txStato();break;
      case READ_DATA:txStato(); break;
      }
      delay(100);
      digitalWrite(led_pin_rx,LOW);	    
      break;
    }
    vw_rx_start();
  }
}

void chkTemperatura(){
  int sensorVal = analogRead(temperPin);
  float voltage = (sensorVal / 1024.0) * 5.0;
  float temperature = (voltage - .5) * 10000;
  temperVALattuale=temperature;
  int diff=abs(temperVALattuale-temperVALprecedente);
  Serial.println("-------------");
  Serial.println(temperVALattuale);
  Serial.println(temperVALprecedente);
  Serial.println(diff);
  
  
    // la differenza
    // col valore precedente e' consistente
    if (temperVALattuale > (temperVALprecedente+100)){
      temperSTAattuale=SALITA;
      temperVALprecedente=temperVALattuale;
    } else {
      if (temperVALattuale < (temperVALprecedente-100)){
	temperSTAattuale=DISCESA;	
  temperVALprecedente=temperVALattuale;
      } else {
	/// esce
      }
    }
    /////////////////
    
    if (temperSTAattuale==temperSTAprecedente){
      temperMINUTIstato++;
    } else {
      temperMINUTIstato=0;
      temperSTAprecedente=temperSTAattuale;
    }
    /////////////////
  
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
  INTERIlocali[DATOc]=digitalRead(rele_pin);
  // codifica in bytes
  encodeMessage();
  // tx
  //******************************
  // prima di trasmettere attende 
  // che l'AGC di rx-MAESTRO
  // abbia recuperato
  //******************************
  delay(1500); // IMPORTANTE 
  //******************************
  vw_rx_stop();
  digitalWrite(led_pin_tx,HIGH);  
  vw_send((uint8_t *)BYTEradio,BYTEStoTX); 
  vw_wait_tx();
  delay(100);
  digitalWrite(led_pin_tx,LOW);  
  vw_rx_start();
}

////////////////////////////////
// RADIO -> locale
////////////////////////////////
void decodeMessage(){
  byte m=0;
  cipher();
  for (int n=0; n<4;n++){
    INTERIlocali[n]=BYTEradio[m+1];
    INTERIlocali[n]=INTERIlocali[n] << 8;
    INTERIlocali[n]=INTERIlocali[n]+BYTEradio[m];
    m+=2;
  }
}

////////////////////////////////
// locale -> RADIO
////////////////////////////////
void encodeMessage(){
  byte m=0;
  for (int n=0; n<4;n++){
    BYTEradio[m]=INTERIlocali[n] & mask;
    INTERIlocali[n]=INTERIlocali[n] >> 8;
    BYTEradio[m+1]=INTERIlocali[n] & mask;
    m+=2;
  }
  cipher();
}

////////////////////////////////
// cifratura XOR del messaggio
////////////////////////////////
void cipher(){
  for (byte n=0;n<8;n++){
    BYTEradio[n]=BYTEradio[n]^CIFR[n];
  }
}
