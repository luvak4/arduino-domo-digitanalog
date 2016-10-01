#include <VirtualWire.h>
const int led_pin = 2;
const int transmit_pin = 12;
const int receive_pin = 11;

void setup() {
  // LEDs
  pinMode(led_pin, OUTPUT);
  // radio tx rx
  vw_set_tx_pin(transmit_pin);
  vw_set_rx_pin(receive_pin);  
  vw_setup(2000);      
  vw_rx_start(); 
  //  Serial.begin(9600);
  // SPEGNE PIN 13 ACCESO DI DEFAULT
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
}

void loop(){
    uint8_t buf[MSG_LEN]={0,0,0,0,0,0,0};
    uint8_t buflen = MSG_LEN;  
    if (vw_get_message(buf, &buflen)){
      vw_rx_stop(); 
      if (buf[0]==0xAA){
	// mittente tastiera
	if (buf[1]==0x06){
	  // rele ON
	  txOK();
          digitalWrite(led_pin,HIGH);
	}
	if (buf[1]==0x07){
	  // rele OFF
 	  txOK();
          digitalWrite(led_pin,LOW);
	}
	if (buf[1]==0x08){
	  // leggi stato rele
	  txOK();
	  txStatoRele();
	}
	if (buf[1]==0x09){
	  // leggi analogico A0
	  txOK();
	  txAnalogicoA0();
	}
      }
      vw_rx_start(); 
    }
}

void txOK(){
  delay(30);
  char msg[MSG_LEN] = {0xAD,0xFE,0,0,0,0,0};
  vw_send((uint8_t *)msg,MSG_LEN);
  vw_wait_tx(); // Wait until the whole message is gone
}

void txStatoRele(){
  delay(30);
  // acceso
  char msg[MSG_LEN] = {0xAD,0x01,0,0,0,0,0};
  if (!digitalRead(led_pin)){
    // spento
    msg[1]=0x02;
  }
  vw_send((uint8_t *)msg,MSG_LEN);
  vw_wait_tx(); // Wait until the whole message is gone  
}

void txAnalogicoA0(){
  char temp[3];
  char msg[MSG_LEN] = {0xAD,0x03,0,0,0,0,0};
  int sensorValue = analogRead(sensorPin);

  delay(30);
  
  itoa(sensorValue, temp, 10);
  
  msg[2]=temp[0];
  msg[3]=temp[1];
  msg[4]=temp[2];  

  vw_send((uint8_t *)msg,MSG_LEN);
  vw_wait_tx(); // Wait until the whole message is gone  
}
