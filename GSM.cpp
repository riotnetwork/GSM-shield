/*
This is a Beta version.
last modified 08/02/2012.

This library is based on one developed by Arduino Labs
and it is modified to preserve the compability
with the Arduino's product.

(http://www.open-electronics.org/arduino-gsm-shield/)
with the same commands of Arduino Shield,
based on uBlox G100/200 chip.
*/

#include "GSM.h"
#include "WideTextFinder.h"

#define _GSM_RXPIN_ 9
#define _GSM_TXPIN_ 10


GSM::GSM():_cell(_GSM_RXPIN_,_GSM_TXPIN_),_tf(_cell, 10),_status(IDLE)
{
};

int GSM::begin(long baud_rate){ //long baud_rate
	int response=-1;
	int cont=0;
	boolean norep=false;
	boolean turnedON=false;
	SetCommLineStatus(CLS_ATCMD);
	_cell.begin(baud_rate); //baud_rate
	setStatus(IDLE); 
	// release reset line
			pinMode(GSM_RESET, OUTPUT); //GSM module control
			pinMode(GSM_ON,OUTPUT);
			digitalWrite(GSM_ON, HIGH);
			
			digitalWrite(GSM_RESET , HIGH);     //startup sequence}
			delay(100);
			digitalWrite(GSM_RESET , LOW);     //startup sequence}
  
			
			// generate turn on pulse
			
			delay(100);
			digitalWrite(GSM_ON, HIGH);
			delay(30);
			digitalWrite(GSM_ON, LOW);
			delay(500);
	for (cont=0; cont<3; cont++){
		if (AT_RESP_ERR_NO_RESP == SendATCmdWaitResp("AT", 500, 100, "OK", 5)&&!turnedON) {		//check power
	    // there is no response => turn on the module
			#ifdef DEBUG_ON
				Serial.println(F("DB:NO RESP"));
			#endif
			digitalWrite(GSM_RESET , HIGH);     //startup sequence
			delay(500);
			digitalWrite(GSM_RESET , LOW);     //startup sequence
			delay(100);
			digitalWrite(GSM_ON, HIGH);
			delay(30);
			digitalWrite(GSM_ON, LOW);
			
			delay(1000);
			norep=true;
		}
		else{
			#ifdef DEBUG_ON
				Serial.println(F("DB:ELSE"));
			#endif
			norep=false;
		}
	}
	
	if (AT_RESP_OK == SendATCmdWaitResp("AT", 500, 100, "OK", 5)){
		#ifdef DEBUG_ON
			Serial.println(F("DB:CORRECT BR"));
		#endif
		turnedON=true;
	}
	if(cont==3&&norep){
		#ifdef DEBUG_ON
		Serial.println(F("ERROR: Module doesn't answer. Check power and serial pins in GSM.cpp"));
		#endif
		return 0;
	}

	SetCommLineStatus(CLS_FREE);

	if(turnedON){
		InitParam(PARAM_SET_0);
		InitParam(PARAM_SET_1);//configure the module  
		Echo(0);               //disable AT echo
		delay(20000);
		if(start())
		{
		#ifdef DEBUG_ON
			Serial.println(F("DB:START"));
		#endif
		setStatus(READY);
		return(1);
		}
		#ifdef DEBUG_ON
			Serial.println(F("DB:!START"));
		#endif

	}
	else
		return(0);
}
int GSM::start()
{
			//delay(1000);
			for (int attempt=0;attempt<=5;attempt++)
			{
			_cell.print(F("AT+CREG?\r"));
			//if(SendATCmdWaitResp(F("AT+CREG?"), 15000, 50, "1", 5) == RX_FINISHED_STR_RECV)
			if(WaitResp(5000, 200, "1"))
			{
			return 1;
			}
			
			}
			return 0;//0
}

void GSM::InitParam(byte group){
	switch (group) {
	case PARAM_SET_0:
		// check comm line
		//if (CLS_FREE != GetCommLineStatus()) return;

		SetCommLineStatus(CLS_ATCMD);
		// Reset to the factory settings
		SendATCmdWaitResp(F("AT&F"), 1000, 50, "OK", 2);      
		// switch off echo
		SendATCmdWaitResp(F("ATE0"), 500, 50, "OK", 2);
		// setup auto baud rate
		SendATCmdWaitResp(F("AT+IPR=0"), 500, 50, "OK", 2);
		// setup mode
		SendATCmdWaitResp(F("AT+IFC=0,0"), 500, 50, "OK", 2); // disable flow control
		// setup Disable flow control for this profile
		SendATCmdWaitResp(F("AT&K0"), 500, 50, "OK", 2);
		// Switch ON User LED - just as signalization we are here
		SendATCmdWaitResp(F("AT+UGPIOC=20,2"), 500, 50, "OK", 2);			// GPIO 1 is used as network status indicator	
		SetCommLineStatus(CLS_FREE);
		break;

	case PARAM_SET_1:
		// check comm line
		SetCommLineStatus(CLS_ATCMD);
		// Request calling line identification
		SendATCmdWaitResp(F("AT+CLIP=1"), 500, 50, "OK", 1);
		// Mobile Equipment Error Code, only reply with ERROR and no code given
		SendATCmdWaitResp(F("AT+CMEE=0"), 500, 50, "OK", 1);
		// Echo canceller enabled 
		//SendATCmdWaitResp("AT#SHFEC=1", 500, 50, "OK", 5);
		// Ringer tone select (0 to 32)
		//SendATCmdWaitResp(F("AT+CSGT=0,\"RESET\""), 500, 50, "OK", 5);// set welcome message
		// Microphone gain (0 to 7) - response here sometimes takes
		// more than 500msec. so 1000msec. is more safety
		// set the SMS mode to text 
		// SendATCmdWaitResp(F("AT+CMGF=1"), 500, 50, "OK", 1);
		// setup set default restart profile
		SendATCmdWaitResp(F("AT&Y1"), 500, 50, "OK", 1);
		// save current as default profile
		SendATCmdWaitResp(F("AT&W1"), 500, 50, "OK", 1);
		
		// checks comm line if it is free
		SetCommLineStatus(CLS_FREE);
		break;
	}
}

byte GSM::WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout, 
                   char const *expected_resp_string)
{
  byte status;
  byte ret_val;

  RxInit(start_comm_tmout, max_interchar_tmout); 
  // wait until response is not finished
  do {
    status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);

  if (status == RX_FINISHED) {
    // something was received but what was received?
    // ---------------------------------------------
	
    if(IsStringReceived(expected_resp_string)) {
      // expected string was received
      // ----------------------------
      ret_val = RX_FINISHED_STR_RECV;      
    }
    else ret_val = RX_FINISHED_STR_NOT_RECV;
  }
  else {
    // nothing was received
    // --------------------
    ret_val = RX_TMOUT_ERR;
  }
  return (ret_val);
}

/**********************************************************
Method sends AT command and waits for response

return: 
      AT_RESP_ERR_NO_RESP = -1,   // no response received
      AT_RESP_ERR_DIF_RESP = 0,   // response_string is different from the response
      AT_RESP_OK = 1,             // response_string was included in the response
**********************************************************/
char GSM::SendATCmdWaitResp(char const *AT_cmd_string,
                uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
                char const *response_string,
                byte no_of_attempts)
{
  byte status;
  char ret_val = AT_RESP_ERR_NO_RESP;
  byte i;

  for (i = 0; i < no_of_attempts; i++) {
    // delay 500 msec. before sending next repeated AT command 
    // so if we have no_of_attempts=1 tmout will not occurred
    if (i > 0) delay(500);  // 

    _cell.println(AT_cmd_string);
    status = WaitResp(start_comm_tmout, max_interchar_tmout); 
    if (status == RX_FINISHED) {
      // something was received but what was received?
      // ---------------------------------------------
      if(IsStringReceived(response_string)) {
        ret_val = AT_RESP_OK;      
        break;  // response is OK => finish
      }
      else ret_val = AT_RESP_ERR_DIF_RESP;
    }
    else {
      // nothing was received
      // --------------------
      ret_val = AT_RESP_ERR_NO_RESP;
    }
    
  }


  return (ret_val);
}


char GSM::SendATCmdWaitResp(__FlashStringHelper* AT_cmd_string,
                uint16_t start_comm_tmout, uint16_t max_interchar_tmout,
                char const *response_string,
                byte no_of_attempts)
{
  byte status;
  char ret_val = AT_RESP_ERR_NO_RESP;
  byte i;

  for (i = 0; i < no_of_attempts; i++) {
    // delay 500 msec. before sending next repeated AT command 
    // so if we have no_of_attempts=1 tmout will not occurred
    if (i > 0) delay(500); 

    _cell.println(AT_cmd_string);
    status = WaitResp(start_comm_tmout, max_interchar_tmout); 
    if (status == RX_FINISHED) {
      // something was received but what was received?
      // ---------------------------------------------
      if(IsStringReceived(response_string)) {
        ret_val = AT_RESP_OK;      
        break;  // response is OK => finish
      }
      else ret_val = AT_RESP_ERR_DIF_RESP;
    }
    else {
      // nothing was received
      // --------------------
      ret_val = AT_RESP_ERR_NO_RESP;
    }
    
  }


  return (ret_val);
}

byte GSM::WaitResp(uint16_t start_comm_tmout, uint16_t max_interchar_tmout)
{
  byte status;

  RxInit(start_comm_tmout, max_interchar_tmout); 
  // wait until response is not finished
  do {
    status = IsRxFinished();
  } while (status == RX_NOT_FINISHED);
  return (status);
}

byte GSM::IsRxFinished(void)
{
  byte num_of_bytes;
  byte ret_val = RX_NOT_FINISHED;  // default not finished

  // Rx state machine
  // ----------------

  if (rx_state == RX_NOT_STARTED) {
    // Reception is not started yet - check tmout
    if (!_cell.available()) {
      // still no character received => check timeout
	/*  
	#ifdef DEBUG_GSMRX
		
			DebugPrint("\r\nDEBUG: reception timeout", 0);			
			Serial.print((unsigned long)(millis() - prev_time));	
			DebugPrint("\r\nDEBUG: start_reception_tmout\r\n", 0);			
			Serial.print(start_reception_tmout);	
			
		
	#endif
	*/
      if ((unsigned long)(millis() - prev_time) >= start_reception_tmout) {
        // timeout elapsed => GSM module didn't start with response
        // so communication is takes as finished
		/*
			#ifdef DEBUG_GSMRX		
				DebugPrint("\r\nDEBUG: RECEPTION TIMEOUT", 0);	
			#endif
		*/
        comm_buf[comm_buf_len] = 0x00;
        ret_val = RX_TMOUT_ERR;
      }
    }
    else {
      // at least one character received => so init inter-character 
      // counting process again and go to the next state
      prev_time = millis(); // init tmout for inter-character space
      rx_state = RX_ALREADY_STARTED;
    }
  }

  if (rx_state == RX_ALREADY_STARTED) {
    // Reception already started
    // check new received bytes
    // only in case we have place in the buffer
    num_of_bytes = _cell.available();
    // if there are some received bytes postpone the timeout
    if (num_of_bytes) prev_time = millis();
      
    // read all received bytes      
    while (num_of_bytes) {
      num_of_bytes--;
      if (comm_buf_len < COMM_BUF_LEN) {
        // we have still place in the GSM internal comm. buffer =>
        // move available bytes from circular buffer 
        // to the rx buffer
        *p_comm_buf = _cell.read();

        p_comm_buf++;
        comm_buf_len++;
        comm_buf[comm_buf_len] = 0x00;  // and finish currently received characters
                                        // so after each character we have
                                        // valid string finished by the 0x00
      }
      else {
        // comm buffer is full, other incoming characters
        // will be discarded 
        // but despite of we have no place for other characters 
        // we still must to wait until  
        // inter-character tmout is reached
        
        // so just readout character from circular RS232 buffer 
        // to find out when communication id finished(no more characters
        // are received in inter-char timeout)
        _cell.read();
      }
    }

    // finally check the inter-character timeout 
	/*
	#ifdef DEBUG_GSMRX
		
			DebugPrint("\r\nDEBUG: intercharacter", 0);			
<			Serial.print((unsigned long)(millis() - prev_time));	
			DebugPrint("\r\nDEBUG: interchar_tmout\r\n", 0);			
			Serial.print(interchar_tmout);	
			
		
	#endif
	*/
    if ((unsigned long)(millis() - prev_time) >= interchar_tmout) {
      // timeout between received character was reached
      // reception is finished
      // ---------------------------------------------
	  
		/*
	  	#ifdef DEBUG_GSMRX
		
			DebugPrint("\r\nDEBUG: OVER INTER TIMEOUT", 0);					
		#endif
		*/
      comm_buf[comm_buf_len] = 0x00;  // for sure finish string again
                                      // but it is not necessary
      ret_val = RX_FINISHED;
    }
  }
		
	
  return (ret_val);
}

/**********************************************************
Method checks received bytes

compare_string - pointer to the string which should be find

return: 0 - string was NOT received
        1 - string was received
**********************************************************/
byte GSM::IsStringReceived(char const *compare_string)
{
  char *ch;
  byte ret_val = 0;

  if(comm_buf_len) {
  /*
		#ifdef DEBUG_GSMRX
			DebugPrint("DEBUG: Compare the string: \r\n", 0);
			for (int i=0; i<comm_buf_len; i++){
				Serial.print(byte(comm_buf[i]));	
			}
			
			DebugPrint("\r\nDEBUG: with the string: \r\n", 0);
			Serial.print(compare_string);	
			DebugPrint("\r\n", 0);
		#endif
	*/
    ch = strstr((char *)comm_buf, compare_string);
    if (ch != NULL) {
      ret_val = 1;
	  /*#ifdef DEBUG_PRINT
		DebugPrint("\r\nDEBUG: expected string was received\r\n", 0);
	  #endif
	  */
    }
	else
	{
	  /*#ifdef DEBUG_PRINT
		DebugPrint("\r\nDEBUG: expected string was NOT received\r\n", 0);
	  #endif
	  */
	}
  }

  return (ret_val);
}


void GSM::RxInit(uint16_t start_comm_tmout, uint16_t max_interchar_tmout)
{
  rx_state = RX_NOT_STARTED;
  start_reception_tmout = start_comm_tmout;
  interchar_tmout = max_interchar_tmout;
  prev_time = millis();
  comm_buf[0] = 0x00; // end of string
  p_comm_buf = &comm_buf[0];
  comm_buf_len = 0;
  _cell.flush(); // erase rx circular buffer
}

void GSM::Echo(byte state)
{
	if (state == 0 or state == 1)
	{
	  SetCommLineStatus(CLS_ATCMD);

	  _cell.print(F("ATE"));
	  _cell.print((int)state);    
	  _cell.print("\r");
	  delay(500);
	  SetCommLineStatus(CLS_FREE);
	}
}

char GSM::InitSMSMemory(void) 
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not initialized yet
  
  // Disable messages about new SMS from the GSM module 
  SendATCmdWaitResp("AT+CNMI=2,0", 1000, 50, "OK", 2);

  // send AT command to init memory for SMS in the SIM card
  // response:
  // +CPMS: <usedr>,<totalr>,<usedw>,<totalw>,<useds>,<totals>
  if (AT_RESP_OK == SendATCmdWaitResp("AT+CPMS=\"SM\",\"SM\",\"SM\"", 1000, 1000, "+CPMS:", 10)) {
    ret_val = 1;
  }
  else ret_val = 0;

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

int GSM::isIP(const char* cadena)
{
    int i;
    for (i=0; i<strlen(cadena); i++)
        if (!(cadena[i]=='.' || ( cadena[i]>=48 && cadena[i] <=57)))
            return 0;
    return 1;
}






