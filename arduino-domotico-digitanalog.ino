#include <VirtualWire.h>
const int led_pin = 2;
const int transmit_pin = 12;
const int receive_pin = 11;
// max lenght of my message
const int MSG_LEN = 7;
// sensore analogico
const int sensorPin = A0;
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
        switch (buf[1]){
          case 0x07:digitalWrite(led_pin,HIGH);break;
          case 0x08:digitalWrite(led_pin,LOW);break;
          case 0x09:txStatoRele();break;
          case 0x0A:txAnalogicoA0();break;
        }
      }
      vw_rx_start();
    }
}



void txStatoRele(){
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
  
  itoa(sensorValue, temp, 10);
  
  msg[2]=temp[0];
  msg[3]=temp[1];
  msg[4]=temp[2];  

  vw_send((uint8_t *)msg,MSG_LEN);
  vw_wait_tx(); // Wait until the whole message is gone  
}
