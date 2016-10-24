////////////////////////////////
// DIGITAL-ANALOG
////////////////////////////////

/*
  	      	+-----------+
  	      	|           |
   light    --->| A0      2 |---> rele
   temperat --->| A1        |
                |           |
   radio rx --->| 11     12 |---> radio tx
  		|           |
  		+-----------+
                   ARDUINO
                  ATMEGA 328
*/

/*
* configurazioni
*/
#include <VirtualWire.h>
#include <EEPROM.h>
/*
** pins
*/
#define pin_rele   2
#define pin_rx    11
#define pin_tx    12
#define pin_light A0
#define pin_temp  A1
/*
** soglie
*/
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
/*
** domande (IN)
*/
#define MASTRa 101 // get luce/temp/rele               (CANTIa)
#define MASTRb 102 // !- set temp (soglia) up   +10    (CANTIb)
#define MASTRc 103 // !- set temp (soglorgia) down -10 (CANTIb)
#define MASTRd 104 // !- set luce (soglia a) up +5     (CANTIb)
#define MASTRe 105 // !- set luce (soglia a) dn -5     (CANTIb)
#define MASTRf 106 // !- set luce (soglia b) up +50    (CANTIb)
#define MASTRg 107 // !- set luce (soglia b) dn -50    (CANTIb)
#define MASTRh 108 // !---> get soglie                 (CANTIb)
#define MASTRi 109 // !- set AGC delay up +300         (CANTIc)
#define MASTRj 110 // !- set AGC delay dn -300         (CANTIc)
#define MASTRk 111 // !---> AGC delay                  (CANTIc)
#define MASTRl 112 // >>> salva  EEPROM                (CANTIokA)
#define MASTRm 113 // >>> carica EEPROM                (CANTIokB)
#define MASTRn 114 // >>> carica DEFAULT               (CANTIokC)
#define MASTRo 115 // get temp/luce STATO/tempo        (CANTId)
#define MASTRp 116 // rele ON                          (CANTIa)
#define MASTRq 117 // rele OFF                         (CANTIa)
#define MASTRr 118 // rele toggle                      (CANTIa)
/*
** risposte (OUT)
*/
#define CANTIa   1000 // get value luce/temp/rele
#define CANTIb   1001 // get soglie luce/temp
#define CANTIc   1002 // get AGC
#define CANTId   1003 // get temp/luce STATO/tempo
#define CANTIokA 1004 // get ok salva eprom
#define CANTIokB 1005 // get ok carica eprom
#define CANTIokC 1006 // get ok carica default
/*
** comunicazione radio principale
*/
#define VELOCITAstd   500
#define MESSnum         0
#define DATOa           1
#define DATOb           2
#define DATOc           3
#define BYTEStoTX       8
int     INTERIlocali[4]={0,0,0,0};
byte    BYTEradio[BYTEStoTX];
uint8_t buflen = BYTEStoTX; //for rx
/*
** stati
*/
#define SALITA              1
#define DISCESA             0
#define LUCEpoca            0
#define LUCEmedia           1
#define LUCEtanta           2
#define RELEon              1
#define RELEoff             0
/*
** variabili temperatura e luce
*/
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
//
int tempSOGLIA  = tempDEFsoglia;
int luceSOGLIAa = luceDEFsogliaA;
int luceSOGLIAb = luceDEFsogliaB;
int AGCdelay    = agcDEF;
/*
** varie
*/
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
/*
* setup()
*/
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
/*
* loop()
*/
void loop(){
/*
** tieni il tempo
*/
  if ((abs(millis()-tempo))>100){
    tempo=millis();
    decimi++;
    //BEGIN ogni decimo
    //END   ogni decimo
    if (decimi>9){
      //BEGIN ogni secondo
      //END ogni secondo
      decimi=0;
      secondi++;
      if (secondi>59){
	//BEGIN ogni minuto
	chkTemperatura();
	chkLuce();
	//END ogni minuto*
	secondi=0;
	minuti++;
	if (minuti>250){
	  minuti=0;
	}
      }
    }
/*
** radio rx
*/
    if (vw_get_message(BYTEradio, &buflen)){
      vw_rx_stop();
      decodeMessage();
      switch (INTERIlocali[MESSnum]){
      case MASTRa:
	// invio valori RAW di luce e temperatura
	ROU_CANTIa();
	break;
	// impostazione soglie variabili di luce/temperatura
      case MASTRb:
	// incremento soglia temperatura
	fxSOGLIE(tempSOGLIA, 10,tempMAXsoglia,tempMINsoglia);
	ROU_CANTIb();
	break;
      case MASTRc:
	// decremento soglia temperatura
	fxSOGLIE(tempSOGLIA,-10,tempMAXsoglia,tempMINsoglia);
	ROU_CANTIb();
	break;
      case MASTRd:
	// incremento prima soglia luce
	fxSOGLIE(luceSOGLIAa, 5,luceMAXsogliaA,luceMINsogliaA);
	ROU_CANTIb();
	break;
      case MASTRe:
	// decremento prima soglia luce
	fxSOGLIE(luceSOGLIAa,-5,luceMAXsogliaA,luceMINsogliaA);
	ROU_CANTIb();
	break;
      case MASTRf:
	// incremento seconda soglia luce
	fxSOGLIE(luceSOGLIAb, 50,luceMAXsogliaB,luceMINsogliaB);
	ROU_CANTIb();
	break;
      case MASTRg:
	// decremento seconda soglia luce
	fxSOGLIE(luceSOGLIAb,-50,luceMAXsogliaB,luceMINsogliaB);
	ROU_CANTIb();
	break;
      case MASTRh:
	// invio SOGLIE
	ROU_CANTIb();
	break;
      case MASTRi:
	// aumenta AGC
	fxSOGLIE(AGCdelay, 300,agcMAX,agcMIN);
	ROU_CANTIc();
	break;
      case MASTRj:
	// diminuisce AGC
	fxSOGLIE(AGCdelay,-300,agcMAX,agcMIN);
	ROU_CANTIc();
	break;
      case MASTRk:
	// invio valore AGC
	ROU_CANTIc();
	break;
      case MASTRl:
	// salva valori su EEPROM
	EEPROMsave() ;
	break;
      case MASTRm:
	// carica valori da EEPROM
	EEPROMload() ;
	break;
      case MASTRn:
	// carica valori di default
	DEFAULTload();
	break;
      case MASTRo:
	// invio STATI e tempi di temperatura e luce
	ROU_CANTId();
	break;
      case MASTRp:
	// rele ON
	digitalWrite(pin_rele,HIGH);
	ROU_CANTIa();
	break;
      case MASTRq:
	// rele OFF
	digitalWrite(pin_rele,LOW);
	ROU_CANTIa();
	break;
      case MASTRr:
	// rele TOGGLE
	digitalWrite(pin_rele,!digitalRead(pin_rele));
	ROU_CANTIa();
	break;
      }
      vw_rx_start();
    }
  }
}

/*
* ROU_CANTId()
*/
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
/*
* ROU_CANTIc()
*/
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
/*
* ROU_CANTIb()
*/
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
/*
* ROU_CANTIa()
*/
////////////////////////////////
// trasmissione valori sensori
////////////////////////////////
void ROU_CANTIa(){
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
/*
* chkTemperatura()
*/
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
/*
* chkLuce()
*/
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
    luceSTA=LUCEpoca;
  } else if ((sensorVal>luceSOGLIAa) & (sensorVal<=luceSOGLIAb)){
    luceSTA=LUCEmedia;
  } else {
    luceSTA=LUCEtanta;
  }
  // cambio stato?
  if (luceSTA==luceSTApre){
    luceMINUTIstato++;
  } else {
    luceMINUTIstato=0;
    luceSTApre=luceSTA;
  }
}
/*
* fxSOGLIE()
*/
////////////////////////////////
// aumenta o decrementa una variabile
// passata byRef
////////////////////////////////
void fxSOGLIE(int& x, int INCDECx, int MAXx, int MINx) {
//if (INCDECx>0){
  x+=INCDECx;
//} else {
//  INCDECx=abs(INCDECx);
//  x-=INCDECx;
//}
  int b=x;
  Serial.println(b);
  Serial.println(MAXx);
  Serial.println(MINx);

 // x+=INCDECx;
 if (b>MAXx){
  x=MAXx;
 }
 if (b<MINx){
  x=MINx;
  }
}

/*
* tx()
*/
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
/*
* decodeMessage()
*/
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
/*
* encodeMessage()
*/
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
/*
* cipher()
*/
////////////////////////////////
// cifratura XOR del messaggio
////////////////////////////////
void cipher(){
  for (byte n=0;n<8;n++){
    BYTEradio[n]=BYTEradio[n]^CIFR[n];
  }
}
/*
* EEPROMsave()
*/
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
/*
* EEPROMload()
*/
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
/*
* DEFAULTload()
*/
////////////////////////////////
// caricamento valori di default
////////////////////////////////
void DEFAULTload(){
  tempSOGLIA=tempDEFsoglia;
  luceSOGLIAa=luceDEFsogliaA;
  luceSOGLIAb=luceDEFsogliaB;
  AGCdelay=agcDEF;
}
/*
* INTtoBYTE()
*/
////////////////////////////////
// conversione da intero a due bytes
////////////////////////////////
void INTtoBYTE(int x, byte& lsb, byte& msb){
  lsb =x & 0x00FF;
  x = x >> 8;
  msb = x & 0x00FF;
}
/*
* BYTEtoINT
*/
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
