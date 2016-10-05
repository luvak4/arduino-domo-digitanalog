#include <VirtualWire.h>
const int led_pin = 2;
const int receive_pin = 11;
const int transmit_pin = 12;
// max lenght of my message
const int MSG_LEN = 7;
// analogic port
const int sensorPin = A0;
unsigned long millPrec=0;
////////////////////////////////
// setup
////////////////////////////////
void setup() {
  pinMode(led_pin, OUTPUT);    // led set pin
  vw_set_tx_pin(transmit_pin); // radio set tx pin
  vw_set_rx_pin(receive_pin);  // radio set rx pin
  vw_setup(2000);              // radio speed
  vw_rx_start();               // radio rx ON
  pinMode(13,OUTPUT);          // out pin 13
  digitalWrite(13,LOW);        // ... set low
}

////////////////////////////////
// loop
////////////////////////////////
void loop(){
  uint8_t buf[MSG_LEN]={0,0,0,0,0,0,0}; // empty buffer
  uint8_t buflen = MSG_LEN;             // lenght of buffer
  if (vw_get_message(buf, &buflen)){    // received a message?
    if (buf[0]==0xAA){
      digitalWrite(13,HIGH);    
      switch (buf[1]){
      case 0x07:digitalWrite(led_pin,HIGH);txStatoRele();break;
      case 0x08:digitalWrite(led_pin,LOW);txStatoRele();break;
      case 0x09:txStatoRele();break;
      case 0x0A:txAnalogicoA0();break;
      case 0x0B:digitalWrite(led_pin,!digitalRead(led_pin));txStatoRele();break;
      }
      digitalWrite(13,LOW);    
    }
  }
  if ((millis()-millPrec)>5000){   
    millPrec=millis();
    txAnalogicoA0();
  }
}

void txStatoRele(){
  char msg[MSG_LEN] = {0xAD,0x01,0,0,0,0,0}; // set buffer base-value
  vw_rx_stop();
  delayForRadioRxAdj();
  if (!digitalRead(led_pin)){
    msg[1]=0x02;                   // overwrite 2nd byte
  }
  vw_send((uint8_t *)msg,MSG_LEN); // send to tx-radio
  vw_wait_tx();                    // wait until message is gone
  vw_rx_start();
}

void txAnalogicoA0(){
  char temp[3];
  char msg[MSG_LEN] = {0xAD,0x03,0,0,0,0,0}; // set buffer base-value
  int sensorValue = analogRead(sensorPin);   // read A0
  vw_rx_stop();
  delayForRadioRxAdj();
  itoa(sensorValue, temp, 10);  // int to char-array
  msg[2]=temp[0];               // overwrite 3rd byte
  msg[3]=temp[1];               // overwrite 4th byte
  msg[4]=temp[2];               // overwrite 5th byte
  vw_send((uint8_t *)msg,MSG_LEN); // send to tx radio
  vw_wait_tx();                    // wait until message is gone
  vw_rx_start();
}

void delayForRadioRxAdj(){
  // this delay is necessary because TX-KEYBOARD and
  // RX-DISPLAY are near and TX blind RX.
  // allows RX to listen regulary
  delay(500);
}
