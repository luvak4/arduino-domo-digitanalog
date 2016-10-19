#include <VirtualWire.h>
////////////////////////////////
// pins
////////////////////////////////
const int rele_pin     =  2;
const int receive_pin  = 11;
const int transmit_pin = 12;
const int lightPin     = A0;
const int temperPin    = A1;
////////////////////////////////
// soglie
////////////////////////////////
#define temperMAXSOGLIA 1000
#define temperMINSOGLIA 10
#define luceMAXSOGLIAa 50
#define luceMINSOGLIAa 1
#define luceMAXSOGLIAb 1000
#define luceMINSOGLIAb 300
#define agcMIN 300;
#define agcMAX 1500;
int temperSOGLIA =100;
int luceSOGLIAa =15;
int luceSOGLIAb =500;
int AGCdelay = 1000;
////////////////////////////////
// indirizzi radio RX
////////////////////////////////
#define MASTRa        101 // get value luce/temp/rele
#define MASTRb        102 // set temp (soglia) up   +10
#define MASTRc        103 // set temp (soglia) down -10
#define MASTRd        104 // set luce (soglia a) up +5
#define MASTRe        105 // set luce (soglia a) dn -5
#define MASTRf        106 // set luce (soglia b) up +50
#define MASTRg        107 // set luce (soglia b) dn -50
#define MASTRh        108 // get soglie
#define MASTRi        109 // set AGC delay up +100
#define MASTRj        110 // set AGC delay dn -100
#define MASTRk        111 // get AGC delay
////////////////////////////////
// indirizzi radio TX
////////////////////////////////
#define CANTIa       1000 // get value luce/temp/rele
#define CANTIb       1001 // get soglie luce/temp
#define CANTIc       1002 // get AGC delay
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
#define PORTACHIUSA 0
#define PORTAAPERTA 1
#define LUCECORRIDOIOACCESA 2

unsigned long tempo;
byte decimi;
byte secondi;
byte minuti;
int  temperVALattuale;
int  temperVALprecedente;
bool temperSTAattuale; //1=RAISE 0=FALL
bool temperSTAprecedente; //1=RAISE 0=FALL
unsigned int  temperMINUTIstato;
//
int  luceVALattuale;
int  luceVALprecedente;
bool luceSTAattuale; //1=RAISE 0=FALL
bool luceSTAprecedente; //1=RAISE 0=FALL
unsigned int  luceMINUTIstato;
// 
////////////////////////////////
// setup
////////////////////////////////
void setup() {
  pinMode(rele_pin, OUTPUT);
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
  // tieni il tempo
  ////////////////////////////////
  if ((abs(millis()-tempo))>100){
    tempo=millis();
    decimi++;
    ////begin ogni decimo///////////
    ////end   ogni decimo///////////    
    if (decimi>9){
      ////begin ogni secondo//////////
      ////end   ogni secondo//////////          
      decimi=0;
      secondi++;
      if (secondi>59){
	////begin ogni minuto //////////
	chkTemperatura();
	chkLuce();
	////end   ogni minuto //////////          	
	secondi=0;
	minuti++;
	if (minuti>250){
	  minuti=0;
	}
      }
    }
    ////////////////////////////////
    // controlla messaggi radio rx
    ////////////////////////////////
    if (vw_get_message(BYTEradio, &buflen)){
      vw_rx_stop();
      // a quanti minuti ha ricevuto l'ultimo messaggio;
      lastCommandAtMinute=minuti;
      // decifra
      decodeMessage();
      ///////primo switch/////////////
      switch (INTERIlocali[INDIRIZZO]){
	// lettura valori luce/temperatura/stato rele
      case MASTRa: ROUTINEa(); break;
	// impostazione soglie variabili di luce/temperatura
      case MASTRb:fxSOGLIE(temperSOGLIA, 10,temperMAXSOGLIA,temperMINSOGLIA);break;
      case MASTRc:fxSOGLIE(temperSOGLIA,-10,temperMAXSOGLIA,temperMINSOGLIA);break;
      case MASTRd:fxSOGLIE(luceSOGLIAa, 5,luceMAXSOGLIAa,luceMINSOGLIAa);break;
      case MASTRe:fxSOGLIE(luceSOGLIAa,-5,luceMAXSOGLIAa,luceMINSOGLIAa);break;
      case MASTRf:fxSOGLIE(luceSOGLIAb, 50,luceMAXSOGLIAb,luceMINSOGLIAb);break;
      case MASTRg:fxSOGLIE(luceSOGLIAb,-50,luceMAXSOGLIAb,luceMINSOGLIAb);break;
	// AGC
      case MASTRi:fxSOGLIE(AGCdelay, 300,agcMAX,agcMIN);break;
      case MASTRj:fxSOGLIE(AGCdelay,-300,agcMAX,agcMIN);break;	
	// lettura soglie
      case MASTRh: ROUTINEb(); break;
	// lettura agc
      case MASTRh: ROUTINEc(); break;      	
      }
      //////secondo switch////////////
      switch (INTERIlocali[INDIRIZZO]){
      case MASTRb: MASTRc: MASTRd: MASTRe: MASTRf: MASTRg:
	ROUTINEb();
	break;
      case MASTRi: MASTRj:
	ROUTINEc();
	break;	
      }
      vw_rx_start();
    }
  }
}

////////////////////////////////
// trasmissione valore AGC
////////////////////////////////
void ROUTINEc(){
  // imposta l'indirizzo
  INTERIlocali[INDIRIZZO]=CANTIc;
  // valori in memoria
  INTERIlocali[DATOa]=AGCdelay;
  INTERIlocali[DATOb]=0;
  INTERIlocali[DATOc]=0;
  //
  tx();
}

////////////////////////////////
// trasmissione soglie
////////////////////////////////
void ROUTINEb(){
  // imposta l'indirizzo
  INTERIlocali[INDIRIZZO]=CANTIb;
  // valori in memoria
  INTERIlocali[DATOa]=temperSOGLIA;
  INTERIlocali[DATOb]=luceSOGLIAa;
  INTERIlocali[DATOc]=luceSOGLIAb;
  //
  tx();
}

////////////////////////////////
// trasmissione valori sensori
////////////////////////////////
void ROUTINEa(){
  // esegue comando
  switch (INTERIlocali[DATOa]){
  case RELE_ON:digitalWrite(rele_pin,HIGH); break;
  case RELE_OFF:digitalWrite(rele_pin,LOW); break;
  case RELE_TOGGLE: digitalWrite(rele_pin,!digitalRead(rele_pin));break;
  case READ_DATA: break;
  }
  // imposta l'indirizzo
  INTERIlocali[INDIRIZZO]=CANTIa;
  // recupera valori
  int sensorVal = analogRead(temperPin);
  float voltage = (sensorVal / 1024.0) * 5.0;
  float temperature = (voltage - .5) * 10000;
  int temper=temperature;
  // valori in memoria
  INTERIlocali[DATOa]=analogRead(lightPin);
  INTERIlocali[DATOb]=temper;
  INTERIlocali[DATOc]=digitalRead(rele_pin);
  // tx
  tx();
}
////////////////////////////////
// leggendo ogni minuto i valori di
// temperatura, determina se la
// temperatura sta salendo o diminuendo.
// Quando cambia di stato (salita-diminuzione
// o viceversa) inizia un conteggio
// (quanti minuti è in salita, quanti in discesa)
// Viene usata una soglia di temperatura
// per evitare oscillazioni di su e giu
// Prende nota dei minuti che la temperatura
// sta nello stato corrente
////////////////////////////////
void chkTemperatura(){
  int sensorVal = analogRead(temperPin);
  float voltage = (sensorVal / 1024.0) * 5.0;
  float temperature = (voltage - .5) * 10000;
  temperVALattuale=temperature;
  int diff=abs(temperVALattuale-temperVALprecedente);
  // la differenza
  // col valore precedente e' consistente
  if (temperVALattuale > (temperVALprecedente+temperSOGLIA)){
    temperSTAattuale=SALITA;
    temperVALprecedente=temperVALattuale;
  } else {
    if (temperVALattuale < (temperVALprecedente-temperSOGLIA)){
      temperSTAattuale=DISCESA;	
      temperVALprecedente=temperVALattuale;
    }
  }  
  if (temperSTAattuale==temperSTAprecedente){
    // lo stato e' lo stesso di prima
    temperMINUTIstato++;
  } else {
    // cambio di stato:
    temperMINUTIstato=0;
    temperSTAprecedente=temperSTAattuale;
  }
}

////////////////////////////////
// controllando le soglie identifica
// lo stato della porta della cantina
// Prende nota dei minuti che la luce
// sta nello stato corrente
////////////////////////////////
void chkLuce(){
  int sensorVal = analogRead(lucePin);
  // soglia
  if ((sensorVal>0) & (sensorVal<=luceSOGLIAa)) {
    luceSTAattuale=PORTACHIUSA;
  } else if ((sensorVal>luceSOGLIAa) & (sensorVal<=luceSOGLIAb)){
    luceSTAattuale=PORTAAPERTA;
  } else {
    luceSTAattuale=LUCECORRIDOIOACCESA;
  }
  // cambio stato?
  if (luceSTAattuale==luceSTAprecedente){
    luceMINUTIstato++;
  } else {
    luceMINUTIstato=0;
    luceSTAprecedente=luceSTAattuale;
  }
}


////////////////////////////////
// aumenta o decrementa una variabile
// passata byRef
////////////////////////////////
void fxSOGLIE(int& x, int INCDECx int MINx, int MAXx){
  x+=INCDECx;
  if (x>MAXx){x=MAXx;}
  if (x<MINx){x=MINx;}
}

void tx(){
  // codifica in bytes
  encodeMessage();  
  //******************************
  // prima di trasmettere attende 
  // che l'AGC di rx-MAESTRO
  // abbia recuperato
  //******************************
  delay(AGCdelay); // IMPORTANTE 
  //******************************
  vw_rx_stop();
  vw_send((uint8_t *)BYTEradio,BYTEStoTX); 
  vw_wait_tx();
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
