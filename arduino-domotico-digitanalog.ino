#include <VirtualWire.h>
#include <EEPROM.h>
////////////////////////////////
// pins
////////////////////////////////
/*
  static const uint8_t A0 = 14;
  static const uint8_t A1 = 15;
  static const uint8_t A2 = 16;
  static const uint8_t A3 = 17;
  static const uint8_t A4 = 18;
  static const uint8_t A5 = 19;
  static const uint8_t A6 = 20;
  static const uint8_t A7 = 21;
*/
#define pin_rele   2
#define pin_rx    11
#define pin_tx    12
#define pin_light A0
#define pin_temp  A1
////////////////////////////////
// soglie
////////////////////////////////
#define tempMAXsoglia  1000
#define tempDEFsoglia  100
#define tempMINsoglia  10
#define luceMAXsogliaA 50
#define luceDEFsogliaA 15
#define luceMINsogliaA 1
#define luceMAXsogliaB 1000
#define luceDEFsogliaB 400
#define luceMINsogliaB 300
#define agcMIN 300
#define agcDEF 1000
#define agcMAX 1500
//
int tempSOGLIA  = tempDEFsoglia;
int luceSOGLIAa = luceDEFsogliaA;
int luceSOGLIAb = luceDEFsogliaB;
int AGCdelay    = agcDEF;
////////////////////////////////
// indirizzi radio RX
////////////////////////////////
#define MASTRa 101 // get luce/temp/rele <--(CANTIa)
#define MASTRb 102 // !- set temp (soglia) up   +10
#define MASTRc 103 // !- set temp (soglia) down -10
#define MASTRd 104 // !- set luce (soglia a) up +5
#define MASTRe 105 // !- set luce (soglia a) dn -5
#define MASTRf 106 // !- set luce (soglia b) up +50
#define MASTRg 107 // !- set luce (soglia b) dn -50
#define MASTRh 108 // !---> get soglie   <--(CANTIb)
#define MASTRi 109 // !- set AGC delay up +100
#define MASTRj 110 // !- set AGC delay dn -100
#define MASTRk 111 // !---> AGC delay    <--(CANTIc)
#define MASTRl 112 // >>> salva  EEPROM  <--(CANTIok)
#define MASTRm 113 // >>> carica EEPROM  <--(CANTIok)
#define MASTRn 114 // >>> carica DEFAULT <--(CANTIok)
#define MASTRo 115 // get temp/luce STATO/tempo
////////////////////////////////
// indirizzi radio TX
////////////////////////////////
#define CANTIa   1000 // get value luce/temp/rele
#define CANTIb   1001 // get soglie luce/temp
#define CANTIc   1002 // get AGC 
#define CANTId   1003 // get temp/luce STATO/tempo
#define CANTIokA 1004 // get ok salva eprom
#define CANTIokB 1005 // get ok carica eprom
#define CANTIokC 1006 // get ok carica default
////////////////////////////////
// comunicazione radio principale
////////////////////////////////
#define VELOCITAstd   500
#define MESSnum       0
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
// STATI
////////////////////////////////
#define SALITA              1
#define DISCESA             0
#define PORTACHIUSA         0
#define PORTAAPERTA         1
#define LUCECORRIDOIOACCESA 2
////////////////////////////////
// valori di temperatura e luce
////////////////////////////////
int  tempVAL;
int  tempVALpre;
byte tempSTA;    //1=RAISE 0=FALL
byte tempSTApre; //1=RAISE 0=FALL
unsigned int  tempMINUTIstato;
//
int  luceVAL;
int  luceVALpre;
byte luceSTA;    
byte luceSTApre; 
unsigned int  luceMINUTIstato;
////////////////////////////////
// varie
////////////////////////////////
byte CIFR[]={223,205,228,240,43,146,241,//
	     87,213,48,235,131,6,81,26,//
	     70,34,74,224,27,111,150,22,//
	     138,239,200,179,222,231,212};
#define mask 0x00FF

unsigned long tempo;
byte decimi;
byte secondi;
byte minuti;
// 
////////////////////////////////
// setup
////////////////////////////////
void setup() {
  pinMode(pin_rele, OUTPUT);
  vw_set_tx_pin(pin_tx);
  vw_set_rx_pin(pin_rx); 
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
      // decifra
      decodeMessage();

      ///////primo switch/////////////
      switch (INTERIlocali[MESSnum]){
	// lettura valori luce/temperatura/stato rele
      case MASTRa: 
	ROU_CANTIa(); 
	break;
	// impostazione soglie variabili di luce/temperatura
      case MASTRb:
	fxSOGLIE(tempSOGLIA, 10,tempMAXsoglia,tempMINsoglia);
	ROU_CANTIb();
	break;
      case MASTRc:
	fxSOGLIE(tempSOGLIA,-10,tempMAXsoglia,tempMINsoglia);
	ROU_CANTIb();
	break;
      case MASTRd:
	fxSOGLIE(luceSOGLIAa, 5,luceMAXsogliaA,luceMINsogliaA);
	ROU_CANTIb();
	break;
      case MASTRe:
	fxSOGLIE(luceSOGLIAa,-5,luceMAXsogliaA,luceMINsogliaA);
	ROU_CANTIb();
	break;
      case MASTRf:
	fxSOGLIE(luceSOGLIAb, 50,luceMAXsogliaB,luceMINsogliaB);
	ROU_CANTIb();
	break;
      case MASTRg:
	fxSOGLIE(luceSOGLIAb,-50,luceMAXsogliaB,luceMINsogliaB);
	ROU_CANTIb();
	break;
      case MASTRh:
	ROU_CANTIb();
	break;
	// impostazione AGC
      case MASTRi:
	fxSOGLIE(AGCdelay, 300,agcMAX,agcMIN);
	ROU_CANTIc();
	break;
      case MASTRj:
	fxSOGLIE(AGCdelay,-300,agcMAX,agcMIN);
	ROU_CANTIc();
	break;
      case MASTRk:
	ROU_CANTIc();
	break;      
	// EEPROM / DEFAULT
      case MASTRl:
	EEPROMsave() ;
	break;
      case MASTRm:
	EEPROMload() ;
	break;
      case MASTRn:
	DEFAULTload();
	break;
	// invio stati e tempi di temperatra e luce
      case MASTRo:
	ROU_CANTId();
	break;
      }
      vw_rx_start();
    }
  }
}

////////////////////////////////
// trasmissione valore STATO/tempo
////////////////////////////////
void ROU_CANTId(){
  // imposta l'indirizzo
  INTERIlocali[MESSnum]=CANTId;
  // valori in memoria
  INTERIlocali[DATOa]=BYTEtoINT(tempSTA,luceSTA);
  // usato un 'int' per memorizzare due byte (temperSTA e luceSTA)
  INTERIlocali[DATOb]=tempMINUTIstato;
  INTERIlocali[DATOc]=luceMINUTIstato;
  //
  tx();
}

////////////////////////////////
// trasmissione valore AGC
////////////////////////////////
void ROU_CANTIc(){
  // imposta l'indirizzo
  INTERIlocali[MESSnum]=CANTIc;
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
void ROU_CANTIb(){
  // imposta l'indirizzo
  INTERIlocali[MESSnum]=CANTIb;
  // valori in memoria
  INTERIlocali[DATOa]=tempSOGLIA;
  INTERIlocali[DATOb]=luceSOGLIAa;
  INTERIlocali[DATOc]=luceSOGLIAb;
  //
  tx();
}

////////////////////////////////
// trasmissione valori sensori
////////////////////////////////
void ROU_CANTIa(){
  // esegue comando
  switch (INTERIlocali[DATOa]){
  case RELE_ON:digitalWrite(pin_rele,HIGH); break;
  case RELE_OFF:digitalWrite(pin_rele,LOW); break;
  case RELE_TOGGLE: digitalWrite(pin_rele,!digitalRead(pin_rele));break;
  case READ_DATA: break;
  }
  // imposta l'indirizzo
  INTERIlocali[MESSnum]=CANTIa;
  // recupera valori
  int sensorVal = analogRead(pin_temp);
  float voltage = (sensorVal / 1024.0) * 5.0;
  float temperature = (voltage - .5) * 10000;
  int temper=temperature;
  // valori in memoria
  INTERIlocali[DATOa]=analogRead(pin_light);
  INTERIlocali[DATOb]=temper;
  INTERIlocali[DATOc]=digitalRead(pin_rele);
  // tx
  tx();
}

////////////////////////////////
// leggendo ogni minuto i valori di
// temperatura, determina se la
// temperatura sta salendo o diminuendo.
// Quando cambia di stato (salita-diminuzione
// o viceversa) inizia un conteggio
// (quanti minuti e' in salita, quanti in discesa)
// Viene usata una soglia di temperatura
// per evitare oscillazioni di su e giu
// Prende nota dei minuti che la temperatura
// sta nello stato corrente
////////////////////////////////
void chkTemperatura(){
  int sensorVal = analogRead(pin_temp);
  float voltage = (sensorVal / 1024.0) * 5.0;
  float temperature = (voltage - .5) * 10000;
  tempVAL=temperature;
  int diff=abs(tempVAL-tempVALpre);
  // la differenza
  // col valore precedente e' consistente
  if (tempVAL > (tempVALpre+tempSOGLIA)){
    tempSTA=SALITA;
    tempVALpre=tempVAL;
  } else {
    if (tempVAL < (tempVALpre-tempSOGLIA)){
      tempSTA=DISCESA;	
      tempVALpre=tempVAL;
    }
  }  
  if (tempSTA==tempSTApre){
    // lo stato e' lo stesso di prima
    tempMINUTIstato++;
  } else {
    // cambio di stato:
    tempMINUTIstato=0;
    tempSTApre=tempSTA;
  }
}

////////////////////////////////
// controllando le soglie identifica
// lo stato della porta della cantina
// Prende nota dei minuti che la luce
// sta nello stato corrente
////////////////////////////////
void chkLuce(){
  int sensorVal = analogRead(pin_light);
  // soglia
  if ((sensorVal>0) & (sensorVal<=luceSOGLIAa)) {
    luceSTA=PORTACHIUSA;
  } else if ((sensorVal>luceSOGLIAa) & (sensorVal<=luceSOGLIAb)){
    luceSTA=PORTAAPERTA;
  } else {
    luceSTA=LUCECORRIDOIOACCESA;
  }
  // cambio stato?
  if (luceSTA==luceSTApre){
    luceMINUTIstato++;
  } else {
    luceMINUTIstato=0;
    luceSTApre=luceSTA;
  }
}

////////////////////////////////
// aumenta o decrementa una variabile
// passata byRef
////////////////////////////////
void fxSOGLIE(int& x, int INCDECx, int MINx, int MAXx){
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
  for (byte n=0; n<4;n++){
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
  for (byte n=0; n<4;n++){
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

////////////////////////////////
// salvataggio valori su EEPROM
////////////////////////////////
void EEPROMsave(){
  byte lsb;
  byte msb;
  INTtoBYTE(tempSOGLIA,lsb,msb);
  EEPROM.write(0,lsb);
  EEPROM.write(1,msb);
  INTtoBYTE(luceSOGLIAa,lsb,msb);
  EEPROM.write(2,lsb);
  EEPROM.write(3,msb);
  INTtoBYTE(luceSOGLIAb,lsb,msb);
  EEPROM.write(4,lsb);
  EEPROM.write(5,msb);
  INTtoBYTE(AGCdelay,lsb,msb);
  EEPROM.write(6,lsb);
  EEPROM.write(7,msb);
}

////////////////////////////////
// caricamento valori da EEPROM
////////////////////////////////
void EEPROMload(){
  byte lsb;
  byte msb;
  lsb=EEPROM.read(0);
  msb=EEPROM.read(1);
  tempSOGLIA=BYTEtoINT(lsb, msb);
  lsb=EEPROM.read(2);
  msb=EEPROM.read(3);
  luceSOGLIAa=BYTEtoINT(lsb, msb);
  lsb=EEPROM.read(4);
  msb=EEPROM.read(5);
  luceSOGLIAb=BYTEtoINT(lsb, msb);
  lsb=EEPROM.read(6);
  msb=EEPROM.read(7);
  AGCdelay=BYTEtoINT(lsb, msb);
}

////////////////////////////////
// caricamento valori di default
////////////////////////////////
void DEFAULTload(){
  tempSOGLIA=tempDEFsoglia;
  luceSOGLIAa=luceDEFsogliaA;
  luceSOGLIAb=luceDEFsogliaB;
  AGCdelay=agcDEF;
}

////////////////////////////////
// conversione da intero a due bytes
////////////////////////////////
void INTtoBYTE(int x, byte& lsb, byte& msb){
  lsb =x & 0x00FF;
  x = x >> 8;
  msb = x & 0x00FF;
}

////////////////////////////////
// conversione da due byte ad un intero
////////////////////////////////
int BYTEtoINT(byte& lsb, byte& msb){
  int x;
  x = msb;
  x = x << 8;
  x = x & lsb;
  return x;
}
