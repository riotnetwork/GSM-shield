#include "SIM900.h"
#include <SoftwareSerial.h>
//#include "inetGSM.h"
//#include "sms.h"
//#include "call.h"

//To change pins for Software Serial, use the two lines in GSM.cpp.

//GSM Shield for Arduino
//www.open-electronics.org
//this code is based on the example of Arduino Labs.

//Simple sketch to communicate with SIM900 through AT commands.

//InetGSM inet;
//CallGSM call;
//SMSGSM sms;

char msg[150];
int numdata;
char inSerial[40];
int i=0;


void setup() 
{
  //Serial connection.
  Serial.begin(9600);
  Serial.println("GSM Shield testing.");
  //Start configuration of shield with baudrate.
  //For http uses is raccomanded to use 4800 or slower.
  if (gsm.begin(9600))
    Serial.println("\nstatus=READY");
  else Serial.println("\nstatus=IDLE");
};

void loop() 
{
  //Read for new byte on serial hardware,
  //and write them on NewSoftSerial.
  serialhwread();
  //Read for new byte on NewSoftSerial.
  serialswread();
};

void serialhwread(){
  i=0;
  if (Serial.available() > 0){            
    while (Serial.available() > 0) {
      inSerial[i]=(Serial.read());
      delay(10);
      i++;      
    }
    
    inSerial[i]='\0';
    if(!strcmp(inSerial,"/END")){
      Serial.println("_");
      inSerial[0]=0x1a;
      inSerial[1]='\0';
      gsm.SimpleWrite(inSerial);
    }
    //Send a saved AT command using serial port.
    if(!strcmp(inSerial,"TEST")){
      Serial.println("SIGNAL QUALITY");
      gsm.SimpleWrite("AT+CSQ");
    }
    //Read last message saved.
    if(!strcmp(inSerial,"MSG")){
      Serial.println(msg);
    }
    
    else{
      Serial.println(inSerial);
      gsm.SimpleWrite(inSerial);
    }    
    
    inSerial[0]='\0';
  }
}

void serialswread(){
  gsm.SimpleRead();
}
