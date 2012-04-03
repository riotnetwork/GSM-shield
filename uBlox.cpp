/*
This is a Beta version.
last modified 17/01/2012.

This library is based on one developed by Arduino Labs
and it is modified to preserve the compability
with the Arduino's product.

The library is modified to use the GSM Shield,
developed by www.open-electronics.org
(http://www.open-electronics.org/arduino-gsm-shield/)
and based on SIM900 chip,
with the same commands of Arduino Shield,
based on QuectelM10 chip.
*/

#include "uBlox.h"  
#include "Streaming.h"

#define _GSM_CONNECTION_TOUT_ 5
#define _TCP_CONNECTION_TOUT_ 20
#define _GSM_DATA_TOUT_ 10


uBlox gsm;
uBlox::uBlox(){};
uBlox::~uBlox(){};
  
int uBlox::configandwait(char* pin)
{
  int connCode;
  _tf.setTimeout(_GSM_CONNECTION_TOUT_);

  if(pin) setPIN(pin); //syv

  // Try 10 times to register in the network. Note this can take some time!
  for(int i=0; i<10; i++)
  {  	
    //Ask for register status to GPRS network.
    _cell << F("AT+CGREG?") <<   crl; 

    //Se espera la unsolicited response de registered to network.
    while (_tf.find("+CGREG: 0,")) 
	{
		connCode=_tf.getValue();
		if((connCode==1)||(connCode==5)) // 1: registered in home network, 5: regestered as roaming
		{
		  setStatus(READY);
		  
		_cell << F("AT+CMGF=1") <<   crl; //SMS text mode.
		delay(200);
		  // Buah, we should take this to readCall()
		_cell << F("AT+CLIP=1") <<   crl; //SMS text mode.
		delay(200);
		//_cell << "AT+QIDEACT" <<  _DEC(cr) << endl; //To make sure not pending connection.
		//delay(1000);
	  
		  return 1;
		}
	}
  }
  return 0;
}



int uBlox::softReset()
{
	#ifdef DEBUG_ON
	Serial.println(F("DB:SRESET"));
	#endif
	SendATCmdWaitResp(F("AT+CFUN=16"), 5000, 100, "OK", 1);
	_cell.end();
}

int uBlox::poweroff( )
{
	#ifdef DEBUG_ON
	Serial.println(F("DB:POWEROFF"));
	#endif
	SendATCmdWaitResp(F("AT+CPWROFF"), 2000, 100, "OK", 1);
}
int uBlox::sendSMS(const char* to, const char* msg)
{

  //Status = READY or ATTACHED.
  /*
  if((getStatus() != READY)&&(getStatus() != ATTACHED))
    return 0;
  */    

  _tf.setTimeout(_GSM_DATA_TOUT_);	//Timeout for expecting modem responses.

  //_cell.flush();

  //AT command to send a SMS. Destination telephone number 
  _cell << F("AT+CMGS=\"") << to << "\"" <<  _DEC(cr) << endl; // Establecemos el destinatario

  //Expect for ">" character.
  
  switch(WaitResp(5000, 50, ">")){
	case RX_TMOUT_ERR: 
		return 0;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }

  //SMS text.
  _cell << msg << _DEC(ctrlz) << _DEC(cr) << "\"\r"; 
   if (!_tf.find("OK")) 
	return 0;
  return 1;
}

int uBlox::attachGPRS(int profile, char* domain, char* dom1, char* dom2)
{
	int i=0;
	if(profile>6)
	{
	return 0;
	}
   delay(200);
   
  _tf.setTimeout(_GSM_DATA_TOUT_);	//	Timeout for expecting modem responses.
  
   _cell << F("AT+CGATT?\r"); 			// check GPRS ATTACH?
  if(WaitResp(5000, 50, "1")!=RX_FINISHED_STR_RECV){ // profile is active, now we want to close it and start a new one
  	#ifdef DEBUG_ON
		Serial.println(F("DB: !GPRS"));
	#endif
	_cell << F("AT+CGATT=1\r");
	delay(5000);
  }
  
  _cell << F("AT+UPSND=")<<profile<<",8\r"; 			// check GPRS connection profile 0 is active?
  if(WaitResp(5000, 50, ",8,0")!=RX_FINISHED_STR_RECV)
  { // profile is active, now we want to close it and start a new one
  	#ifdef DEBUG_ON
		Serial<<F("DB:GPRS ")<<profile<<F(" ACTIVE\r");
	#endif
	return 1;
  }
  else	{
		#ifdef DEBUG_ON
			Serial.println(F("DB:STARTING NEW CONNECTION"));
		#endif
		delay(200);
		 _cell <<F( "AT+UPSD=")<<profile<<",1,\"" << domain << "\"\r"; // APN name
		 if( WaitResp(2000, 50, "OK")==RX_FINISHED_STR_RECV)
		 {
		 _cell << F("AT+UPSD=")<<profile<<",2,\""<< dom1  << "\"\r";
			 if( WaitResp(2000, 50, "OK")==RX_FINISHED_STR_RECV)
			 {
			 _cell << F("AT+UPSD=")<<profile<<",3,\"" << dom2 << "\"\r";
				  if( WaitResp(2000, 50, "OK")==RX_FINISHED_STR_RECV)
				 {
				  _cell << F("AT+UPSD=")<<profile<<F(",7,\"0.0.0.0\"\r");	// dynamic IP assignment
				  if( WaitResp(2000, 50, "OK")==RX_FINISHED_STR_RECV)
					 {
					  _cell << F("AT+UPSDA=")<<profile<<",3\r";			// activate GPRS connection
						  if( WaitResp(4000, 50, "OK")==RX_FINISHED_STR_RECV)
						 {
						  _cell << F("AT+UPSND=")<<profile<<",8\r"; 			// check GPRS connection profile 0 is active?
							 if(WaitResp(5000, 50, ",8,1")==RX_FINISHED_STR_RECV){ // profile is active
								#ifdef DEBUG_ON
									Serial.println(F("DB:APN OK"));
								#endif
								return 1;
							  }
						  }else{} //APN activate gprs connection
					  }else{} //APN DHCP
				  }else{} //APN password
			 }else{} //APN username
		 }else{} //APN name
		 
		  
		  
		 return 0;
		} 
}

int uBlox::detachGPRS(int profile)
{
  if (getStatus()==IDLE) return 0;
   #ifdef DEBUG_ON
			Serial.println(F("DB:DETACH GPRS"));
		#endif
  _tf.setTimeout(_GSM_CONNECTION_TOUT_);
  _cell <<F( "AT+UPSDA=")<<profile<<",4\r";			// deactivate GPRS connection, 
  WaitResp(500, 50, "OK");

  //GPRS dettachment.
  _cell << F("AT+CGATT=0\r");
  
  if(!_tf.find("OK")) 
  {
    setStatus(ERROR);
    return 0;
  }
  delay(500);
  

  setStatus(READY);
  return 1;
}


int uBlox::createTCPsocket() // create a socket to be used for subsequent connections
{
		_tf.setTimeout(_TCP_CONNECTION_TOUT_);
		_cell <<F( "AT+USOCR=6\r"); // create a TCP socket, will return with socketID /r/n OK
		delay(100);
		int socketID =-1;
		//int firstno=_tf.getValue();	// only because echo is turned off
		socketID = _tf.getValue();	
		if(socketID >= 0 && socketID<=6)// valid Socket created
		{ 
			#ifdef DEBUG_ON
			Serial.print(F("DB:SOCKET CREATED : "));
			Serial.println(socketID);
			#endif		
		return socketID;
		}
			#ifdef DEBUG_ON
			Serial.println(F("DB:SOCKET ERROR"));
			#endif	 	
	return -1;	
}

/*
char uBlox::DNSlookup(char *server) // Performs a DNS lookup to get an IP adress for a given host identifier
{		
	char serverIP[15];
	if(isIP(server)) // we already have an IP adress
		{
		return server;
		}
		_tf.setTimeout(_TCP_CONNECTION_TOUT_);
		_cell << "AT+UDNSRN=0,\"" << server << "\"" << "\r"; //will respond with the IP address of the domain name, will reply : \rvvv.www.xxx.yyy\rOK
		_tf.getString("+UDNSRN: \"", "\"", serverIP, 15);
		return serverIP;
}
*/

int uBlox::ip( char* ip)
{
	#ifdef DEBUG_ON
	Serial.println(F("DB:IP LOOKUP"));
	#endif
	_cell << F("AT+UPSND=0,0\r");
		if(_tf.find("+UPSND: 0"))
		{
		_tf.getString(",0,\"", "\"", ip, 15);
		}
		else
		{
		#ifdef DEBUG_ON
		Serial.println(F("DB:IP FAIL"));
		#endif
		return 0;
		}
		if(isIP(ip))
		{
		return 1;
		}
		ip = "0.0.0.0";
		return 0;
}

int uBlox::connectTCP(const char* server, int port, int id) // **** page 13/51 uBlox AT command examples
{
  _tf.setTimeout(_TCP_CONNECTION_TOUT_);
/*
create TCP socket : createTCPsocket(); will return with socket identifier
		get IP of host from DNS lookup ( of the socket we created) DNSlookup(server); will return the IP adress
		connect to remote host ( of the socket we created)
*/
	#ifdef DEBUG_ON
	Serial.println(F("DB:TCP CONNECTION"));
	#endif
	
	id = createTCPsocket(); // if this is -1, we do not have a socket ID
  //Visit the remote TCP server.
  if(id>=0 && id <=6)
  {
  delay(200);
  _cell <<F( "AT+USOCO=") << id <<",\"" << server << "\"," << port <<  "\r"; // will return OK if ok, else will return ERROR/r/n/r/n+UUSOCL: # ( socket # closed)
 // Serial <<F( "AT+USOCO=") << id <<",\"" << server << "\"," << port <<  "\r"; // will return OK if ok, else will return ERROR/r/n/r/n+UUSOCL: # ( socket # closed)
 
  switch(WaitResp(20000, 200, "OK")){
	case RX_TMOUT_ERR: 
	#ifdef DEBUG_ON
	Serial.println(F("DB:TCP TIMEOUT"));
	#endif
		_cell <<F( "AT+USOCL=")<<id<<crl;
		detachGPRS(0);// detach profile 0
		return 0;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		_cell <<F( "AT+USOCL=")<<id<<crl;
		detachGPRS(0);// detach profile 0
		#ifdef DEBUG_ON
		Serial.println(F("DB:TCP NO OK"));
		#endif
		return 0; 
	break;
  }
  delay(20);
  _cell << F("AT+USODL=")<< id <<"\r"; // go into direct datalink mode
  switch(WaitResp(5000, 200, "CONNECT"))
  {
	case RX_TMOUT_ERR: 
		return 0;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }
  #ifdef DEBUG_ON
	Serial.println(F("DB:CONNECT"));
  #endif
  //delay(20);
  return 1; // we are connected to a server via TCP
  }
  return 0;
}

int uBlox::disconnectTCP(int id)
{
  //Status = TCPCONNECTEDCLIENT or TCPCONNECTEDSERVER.
  /*
  if ((getStatus()!=TCPCONNECTEDCLIENT)&&(getStatus()!=TCPCONNECTEDSERVER))
     return 0;
  */
   
  #ifdef DEBUG_ON
	Serial.println(F("DB:TCP DISCONNECT"));
  #endif
  _tf.setTimeout(_GSM_CONNECTION_TOUT_);

  

  //Switch to AT mode.
  delay(2000); // wait 2 seconds after any data sent before we send the termination string to return to AT mode
  _cell.flush();
  _cell << "+++";
  delay(1000); // wait 1 second ( no data is allowed here otherwise we cannot exit data direct link mode
  //detachGPRS(0);// detach profile 0
    if(WaitResp(5000, 200, "DISCONNECT")==RX_FINISHED_STR_RECV)
	  {
	   #ifdef DEBUG_ON
		Serial.println(F("DB:DISCONNECT"));
	  #endif
	 // delay(500);
	  }
  _cell<<"AT+USOCL=0\r";
   switch(WaitResp(5000, 200, "OK"))
  {
	case RX_FINISHED_STR_RECV: 
		return 1;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }
  return 1;
}


boolean uBlox::connectedClient(int id)
{
  /*
  if (getStatus()!=TCPSERVERWAIT)
     return 0;
  */
   _cell << F("AT+USOCTL=")<< id << ",10\r";
  // Alternative: AT+QISTAT, although it may be necessary to call an AT 
  // command every second,which is not wise
  /*
  switch(WaitResp(1000, 200, "OK")){
	case RX_TMOUT_ERR: 
		return 0;
	break;
	case RX_FINISHED_STR_NOT_RECV: 
		return 0; 
	break;
  }*/
  _tf.setTimeout(1);
  if(_tf.find("4"))  // response 4 means socket connection is established page 21/51
  {
    setStatus(TCPCONNECTEDSERVER);
    return true;
  }
  else
    return false;
 }

int uBlox::read(char* result, int resultlength)
{
  // Or maybe do it with AT+QIRD

  int charget;
  _tf.setTimeout(3);
  // Not well. This way we read whatever comes in one second. If a CLOSED 
  // comes, we have spent a lot of time
    //charget=_tf.getString("",'\0',result, resultlength);
    charget=_tf.getString("","",result, resultlength);
  /*if(strtok(result, "CLOSED")) // whatever chain the Q10 returns...
  {
    // TODO: use strtok to delete from the chain everything from CLOSED
    if(getStatus()==TCPCONNECTEDCLIENT)
      setStatus(ATTACHED);
    else
      setStatus(TCPSERVERWAIT);
  }  */
  
  return charget;
}

int uBlox::signal(int param)
{
	#ifdef DEBUG_ON
	Serial.println(F("DB:SIGNAL"));
	#endif
	int rssi,ber,value;
	_cell << "AT+CSQ\r";
		if(_tf.find("+CSQ"))
		{
		rssi=_tf.getValue();
		ber=_tf.getValue();
		}
		else
		{
		#ifdef DEBUG_ON
		Serial.println(F("DB:SIGNAL FAIL"));
		#endif
		}
		switch(param)
		{
		case 0:
				value = rssi;
		break;
		case 1:
				value = ber;
		break;
		}
		return value;
		
}

 int uBlox::readCellData(int &mcc, int &mnc, long &lac, long &cellid)
{
  if (getStatus()==IDLE)
    return 0;
    
   _tf.setTimeout(_GSM_DATA_TOUT_);
   //_cell.flush();
  _cell << F("AT+QENG=1,0") << endl; 
  _cell << F("AT+QENG?" )<< endl; 
  if(!_tf.find("+QENG:"))
    return 0;

  mcc=_tf.getValue(); // The first one is 0
  mcc=_tf.getValue();
  mnc=_tf.getValue();
  lac=_tf.getValue();
  cellid=_tf.getValue();
  _tf.find("OK");
  _cell << F("AT+QENG=1,0") << endl; 
  _tf.find("OK");
  return 1;
}

boolean uBlox::readSMS(char* msg, int msglength, char* number, int nlength)
{
  long index;
  /*
  if (getStatus()==IDLE)
    return false;
  */
  _tf.setTimeout(_GSM_DATA_TOUT_);
  //_cell.flush();
  _cell << F("AT+CMGL=\"REC UNREAD\",1") << endl;
  if(_tf.find("+CMGL: "))
  {
    index=_tf.getValue();
    _tf.getString("\"+", "\"", number, nlength);
    _tf.getString("\r\n", "\r\nOK", msg, msglength);
    _cell << F("AT+CMGD=") << index << endl;
    _tf.find("OK");
    return true;
  };
  return false;
};

boolean uBlox::readCall(char* number, int nlength)
{
  int index;

  if (getStatus()==IDLE)
    return false;
  
  _tf.setTimeout(_GSM_DATA_TOUT_);

  if(_tf.find("+CLIP: \""))
  {
    _tf.getString("", "\"", number, nlength);
    _cell << "ATH" << endl;
    delay(1000);
    //_cell.flush();
    return true;
  };
  return false;
};

boolean uBlox::call(char* number, unsigned int milliseconds)
{ 
  if (getStatus()==IDLE)
    return false;
  
  _tf.setTimeout(_GSM_DATA_TOUT_);

  _cell << "ATD" << number << ";" << endl;
  delay(milliseconds);
  _cell << "ATH" << endl;

  return true;
 
}

int uBlox::setPIN(char *pin)
{
  //Status = READY or ATTACHED.
  if((getStatus() != IDLE))
    return 2;
      
  _tf.setTimeout(_GSM_DATA_TOUT_);	//Timeout for expecting modem responses.

  //_cell.flush();

  //AT command to set PIN.
  _cell << F("AT+CPIN=") << pin <<  _DEC(cr) << endl; // Establecemos el pin

  //Expect "OK".
  if(!_tf.find("OK"))
    return 0;
  else  
    return 1;
}

int uBlox::changeNSIPmode(char mode)
{
    _tf.setTimeout(_TCP_CONNECTION_TOUT_);
    
    //if (getStatus()!=ATTACHED)
    //    return 0;

    //_cell.flush();

    _cell << F("AT+QIDNSIP=") << mode <<  _DEC(cr) << endl;

    if(!_tf.find("OK")) return 0;
    
    return 1;
}

int uBlox::getCCI(char *cci)
{
  //Status must be READY
  if((getStatus() != READY))
    return 2;
      
  _tf.setTimeout(_GSM_DATA_TOUT_);	//Timeout for expecting modem responses.

  //_cell.flush();

  //AT command to get CCID.
  _cell << F("AT+CCID") << _DEC(cr) << endl; // Establecemos el pin
  
  //Read response from modem
  _tf.getString("+CCID: ","\r\n",cci, 21);
  
  //Expect "OK".
  if(!_tf.find("OK"))
    return 0;
  else  
    return 1;
}
  
int uBlox::getIMEI(char *imei)
{
      
  _tf.setTimeout(_GSM_DATA_TOUT_);	//Timeout for expecting modem responses.

  //_cell.flush();

  //AT command to get IMEI.
  _cell << F("AT+GSN") << _DEC(cr) << endl; 
  
  //Read response from modem
  _tf.getString("","\r\n",imei, 15);
  
  //Expect "OK".
  if(!_tf.find("OK"))
    return 0;
  else  
    return 1;
}

uint8_t uBlox::read()
{
  return _cell.read();
}

void uBlox::SimpleRead()
{
	char datain;
	if(_cell.available()>0){
		datain=_cell.read();
		if(datain>0){
			Serial.print(datain);
		}
	}
}


int uBlox::available()
{
return _cell.available();
}

void uBlox::SimpleWrite(char *comm)
{
	_cell.print(comm);
}

void uBlox::SimpleWrite(String comm)
{
	_cell.print(comm);
}

void uBlox::SimpleWrite(const char *comm)
{
	_cell.print(comm);
}

void uBlox::SimpleWrite(__FlashStringHelper* comm)
{
	_cell.print(comm);
}
void uBlox::SimpleWrite(int comm)
{
	_cell.print(comm);
}

void uBlox::SimpleWriteln(char *comm)
{
	_cell.println(comm);
}

void uBlox::SimpleWriteln(char const *comm)
{
	_cell.println(comm);
}

void uBlox::SimpleWriteln(int comm)
{
	_cell.println(comm);
}
void uBlox::WhileSimpleRead()
{
	char datain;
	while(_cell.available()>0){
		datain=_cell.read();
		if(datain>0){
			Serial.print(datain);
		}
	}
}
















//---------------------------------------------
/**********************************************************
Turns on/off the speaker

off_on: 0 - off
        1 - on
**********************************************************/
void GSM::SetSpeaker(byte off_on)
{
  if (CLS_FREE != GetCommLineStatus()) return;
  SetCommLineStatus(CLS_ATCMD);
  if (off_on) {
    //SendATCmdWaitResp("AT#GPIO=5,1,2", 500, 50, "#GPIO:", 1);
  }
  else {
    //SendATCmdWaitResp("AT#GPIO=5,0,2", 500, 50, "#GPIO:", 1);
  }
  SetCommLineStatus(CLS_FREE);
}


byte GSM::IsRegistered(void)
{
  return (module_status & STATUS_REGISTERED);
}

byte GSM::IsInitialized(void)
{
  return (module_status & STATUS_INITIALIZED);
}


/**********************************************************
Method checks if the GSM module is registered in the GSM net
- this method communicates directly with the GSM module
  in contrast to the method IsRegistered() which reads the
  flag from the module_status (this flag is set inside this method)

- must be called regularly - from 1sec. to cca. 10 sec.

return values: 
      REG_NOT_REGISTERED  - not registered
      REG_REGISTERED      - GSM module is registered
      REG_NO_RESPONSE     - GSM doesn't response
      REG_COMM_LINE_BUSY  - comm line between GSM module and Arduino is not free
                            for communication
**********************************************************/
byte GSM::CheckRegistration(void)
{
  byte status;
  byte ret_val = REG_NOT_REGISTERED;

  if (CLS_FREE != GetCommLineStatus()) return (REG_COMM_LINE_BUSY);
  SetCommLineStatus(CLS_ATCMD);
  _cell.println("AT+CREG?");
  // 5 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  status = WaitResp(5000, 50); 

  if (status == RX_FINISHED) {
    // something was received but what was received?
    // ---------------------------------------------
    if(IsStringReceived("+CREG: 0,1") 
      || IsStringReceived("+CREG: 0,5")) {
      // it means module is registered
      // ----------------------------
      module_status |= STATUS_REGISTERED;
    
    
      // in case GSM module is registered first time after reset
      // sets flag STATUS_INITIALIZED
      // it is used for sending some init commands which 
      // must be sent only after registration
      // --------------------------------------------
      if (!IsInitialized()) {
        module_status |= STATUS_INITIALIZED;
        SetCommLineStatus(CLS_FREE);
        InitParam(PARAM_SET_1);
      }
      ret_val = REG_REGISTERED;      
    }
    else {
      // NOT registered
      // --------------
      module_status &= ~STATUS_REGISTERED;
      ret_val = REG_NOT_REGISTERED;
    }
  }
  else {
    // nothing was received
    // --------------------
    ret_val = REG_NO_RESPONSE;
  }
  SetCommLineStatus(CLS_FREE);
 

  return (ret_val);
}


/**********************************************************
Method sets speaker volume

speaker_volume: volume in range 0..14

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..14 current speaker volume 
**********************************************************/
/*
char GSM::SetSpeakerVolume(byte speaker_volume)
{
  
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // remember set value as last value
  if (speaker_volume > 14) speaker_volume = 14;
  // select speaker volume (0 to 14)
  // AT+CLVL=X<CR>   X<0..14>
  _cell.print("AT+CLVL=");
  _cell.print((int)speaker_volume);    
  _cell.print("\r"); // send <CR>
  // 10 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(10000, 50)) {
    ret_val = -2; // ERROR
  }
  else {
    if(IsStringReceived("OK")) {
      last_speaker_volume = speaker_volume;
      ret_val = last_speaker_volume; // OK
    }
    else ret_val = -3; // ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}
*/
/**********************************************************
Method increases speaker volume

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..14 current speaker volume 
**********************************************************/
/*
char GSM::IncSpeakerVolume(void)
{
  char ret_val;
  byte current_speaker_value;

  current_speaker_value = last_speaker_volume;
  if (current_speaker_value < 14) {
    current_speaker_value++;
    ret_val = SetSpeakerVolume(current_speaker_value);
  }
  else ret_val = 14;

  return (ret_val);
}
*/
/**********************************************************
Method decreases speaker volume

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module did not answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0..14 current speaker volume 
**********************************************************/
/*
char GSM::DecSpeakerVolume(void)
{
  char ret_val;
  byte current_speaker_value;

  current_speaker_value = last_speaker_volume;
  if (current_speaker_value > 0) {
    current_speaker_value--;
    ret_val = SetSpeakerVolume(current_speaker_value);
  }
  else ret_val = 0;

  return (ret_val);
}
*/

/**********************************************************
Method sends DTMF signal
This function only works when call is in progress

dtmf_tone: tone to send 0..15

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - GSM module has answered "ERROR" string

        OK ret val:
        -----------
        0.. tone
**********************************************************/
/*
char GSM::SendDTMFSignal(byte dtmf_tone)
{
  char ret_val = -1;

  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  // e.g. AT+VTS=5<CR>
  _cell.print("AT+VTS=");
  _cell.print((int)dtmf_tone);    
  _cell.print("\r");
  // 1 sec. for initial comm tmout
  // 50 msec. for inter character timeout
  if (RX_TMOUT_ERR == WaitResp(1000, 50)) {
    ret_val = -2; // ERROR
  }
  else {
    if(IsStringReceived("OK")) {
      ret_val = dtmf_tone; // OK
    }
    else ret_val = -3; // ERROR
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}
*/

/**********************************************************
Method returns state of user button


return: 0 - not pushed = released
        1 - pushed
**********************************************************/
byte GSM::IsUserButtonPushed(void)
{
  byte ret_val = 0;
  if (CLS_FREE != GetCommLineStatus()) return(0);
  SetCommLineStatus(CLS_ATCMD);
  //if (AT_RESP_OK == SendATCmdWaitResp("AT#GPIO=9,2", 500, 50, "#GPIO: 0,0", 1)) {
    // user button is pushed
  //  ret_val = 1;
  //}
  //else ret_val = 0;
  //SetCommLineStatus(CLS_FREE);
  //return (ret_val);
}



/**********************************************************
Method reads phone number string from specified SIM position

position:     SMS position <1..20>

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0
        phone_number is empty string

        OK ret val:
        -----------
        0 - there is no phone number on the position
        1 - phone number was found
        phone_number is filled by the phone number string finished by 0x00
                     so it is necessary to define string with at least
                     15 bytes(including also 0x00 termination character)

an example of usage:
        GSM gsm;
        char phone_num[20]; // array for the phone number string

        if (1 == gsm.GetPhoneNumber(1, phone_num)) {
          // valid phone number on SIM pos. #1 
          // phone number string is copied to the phone_num array
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone number: ", 0);
            gsm.DebugPrint(phone_num, 1);
          #endif
        }
        else {
          // there is not valid phone number on the SIM pos.#1
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG there is no phone number", 1);
          #endif
        }
**********************************************************/


char GSM::GetPhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;

  char *p_char; 
  char *p_char1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // not found yet
  phone_number[0] = 0; // phone number not found yet => empty string
  
  //send "AT+CPBR=XY" - where XY = position
  _cell.print("AT+CPBR=");
  _cell.print((int)position);  
  _cell.print("\r");

  // 5000 msec. for initial comm tmout
  // 50 msec. for inter character timeout
  switch (WaitResp(5000, 50, "+CPBR")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      ret_val = -2;
      break;

    case RX_FINISHED_STR_RECV:
      // response in case valid phone number stored:
      // <CR><LF>+CPBR: <index>,<number>,<type>,<text><CR><LF>
      // <CR><LF>OK<CR><LF>

      // response in case there is not phone number:
      // <CR><LF>OK<CR><LF>
      p_char = strchr((char *)(comm_buf),'"');
      if (p_char != NULL) {
        p_char++;       // we are on the first phone number character
        // find out '"' as finish character of phone number string
        p_char1 = strchr((char *)(p_char),'"');
        if (p_char1 != NULL) {
          *p_char1 = 0; // end of string
        }
        // extract phone number string
        strcpy(phone_number, (char *)(p_char));
        // output value = we have found out phone number string
        ret_val = 1;
      }
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // only OK or ERROR => no phone number
      ret_val = 0; 
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}

/**********************************************************
Method writes phone number string to the specified SIM position

position:     SMS position <1..20>
phone_number: phone number string for the writing

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone number was not written
        1 - phone number was written
**********************************************************/
char GSM::WritePhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // phone number was not written yet
  
  //send: AT+CPBW=XY,"00420123456789"
  // where XY = position,
  //       "00420123456789" = phone number string
  _cell.print("AT+CPBW=");
  _cell.print((int)position);  
  _cell.print(",\"");
  _cell.print(phone_number);
  _cell.print("\"\r");

  // 5000 msec. for initial comm tmout
  // 50 msec. for inter character timeout
  switch (WaitResp(5000, 50, "OK")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      break;

    case RX_FINISHED_STR_RECV:
      // response is OK = has been written
      ret_val = 1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // other response: e.g. ERROR
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}


/**********************************************************
Method del phone number from the specified SIM position

position:     SMS position <1..20>

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone number was not deleted
        1 - phone number was deleted
**********************************************************/
char GSM::DelPhoneNumber(byte position)
{
  char ret_val = -1;

  if (position == 0) return (-3);
  if (CLS_FREE != GetCommLineStatus()) return (ret_val);
  SetCommLineStatus(CLS_ATCMD);
  ret_val = 0; // phone number was not written yet
  
  //send: AT+CPBW=XY
  // where XY = position
  _cell.print("AT+CPBW=");
  _cell.print((int)position);  
  _cell.print("\r");

  // 5000 msec. for initial comm tmout
  // 50 msec. for inter character timeout
  switch (WaitResp(5000, 50, "OK")) {
    case RX_TMOUT_ERR:
      // response was not received in specific time
      break;

    case RX_FINISHED_STR_RECV:
      // response is OK = has been written
      ret_val = 1;
      break;

    case RX_FINISHED_STR_NOT_RECV:
      // other response: e.g. ERROR
      break;
  }

  SetCommLineStatus(CLS_FREE);
  return (ret_val);
}





/**********************************************************
Function compares specified phone number string 
with phone number stored at the specified SIM position

position:       SMS position <1..20>
phone_number:   phone number string which should be compare

return: 
        ERROR ret. val:
        ---------------
        -1 - comm. line to the GSM module is not free
        -2 - GSM module didn't answer in timeout
        -3 - position must be > 0

        OK ret val:
        -----------
        0 - phone numbers are different
        1 - phone numbers are the same


an example of usage:
        if (1 == gsm.ComparePhoneNumber(1, "123456789")) {
          // the phone num. "123456789" is stored on the SIM pos. #1
          // phone number string is copied to the phone_num array
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone numbers are the same", 1);
          #endif
        }
        else {
          #ifdef DEBUG_PRINT
            gsm.DebugPrint("DEBUG phone numbers are different", 1);
          #endif
        }
**********************************************************/
char GSM::ComparePhoneNumber(byte position, char *phone_number)
{
  char ret_val = -1;
  char sim_phone_number[20];


  ret_val = 0; // numbers are not the same so far
  if (position == 0) return (-3);
  if (1 == GetPhoneNumber(position, sim_phone_number)) {
    // there is a valid number at the spec. SIM position
    // -------------------------------------------------
    if (0 == strcmp(phone_number, sim_phone_number)) {
      // phone numbers are the same
      // --------------------------
      ret_val = 1;
    }
  }
  return (ret_val);
}

//-----------------------------------------------------

//GPS things

void uBlox::gps(int mode) // turn GPS on or off, will always use hybrid positioning ( cellocate and GPS correlated )
{
#ifdef DEBUG_ON
	Serial.println(F("DB:GPS START"));
  #endif
	SendATCmdWaitResp(F("AT+UGRMC=1"), 1000, 50, "OK", 2); // enable RMC string storage
	SendATCmdWaitResp(F("AT+UGIND=1"), 1000, 50, "OK", 2); // enable GPS response strings
	switch( mode)
		{
		case 0: // normal GPS only
		SendATCmdWaitResp(F("AT+ULOCCELL=0"), 1000, 50, "OK", 2); // enable Cell tower triangulation, basic only
		SendATCmdWaitResp(F("AT+ULOCGNSS=0"), 1000, 50, "OK", 2); // enable basic GPS positioning
		//ulocgnss=0
		break;
		case 1: // local aiding
		SendATCmdWaitResp(F("AT+ULOCCELL=0"), 1000, 50, "OK", 2); // enable Cell tower triangulation, basic only
		SendATCmdWaitResp(F("AT+ULOCGNSS=1"), 1000, 50, "OK", 2); // enable locally aided GPS positioing ( it remembers the previos fix data)
		break;
		case 2: // Assistnow offline ( will use a pre downloaded database )
		// activate GPRS
		SendATCmdWaitResp(F("AT+ULOCCELL=1"), 1000, 50, "OK", 2); // enable Cell tower triangulation, all service providers
		SendATCmdWaitResp(F("AT+ULOCGNSS=2"), 1000, 50, "OK", 2); // enableGPS with a 14 day sattelite table download to get a quick fix
		break;
		case 3: // Assistnow online ( will use a pre downloaded database  and correlate online)
		// activate GPRS
		SendATCmdWaitResp(F("AT+ULOCCELL=1"), 1000, 50, "OK", 2); // enable Cell tower triangulation, all service providers
		SendATCmdWaitResp(F("AT+ULOCGNSS=4"), 1000, 50, "OK", 2); // enable GPS with immediate sattelite data download
		break;
		case 4: // Assistnow autonomous( will contact u-blox server )
		// activate GPRS
		SendATCmdWaitResp(F("AT+ULOCCELL=1"), 1000, 50, "OK", 2); // enable Cell tower triangulation, all service providers
		SendATCmdWaitResp(F("AT+ULOCGNSS=8"), 1000, 50, "OK", 2); // enable GPS positioning with immediate sattelite and cell data download and correlation
		break;
		case 5: // All aiding modes used
		// activate GPRS
		SendATCmdWaitResp(F("AT+ULOCCELL=1"), 1000, 50, "OK", 2); // enable Cell tower triangulation, all service providers
		SendATCmdWaitResp(F("AT+ULOCGNSS=15"), 1000, 50, "OK", 2); // enable  GPS positioning with all available positioing aids running
		break;
		}
}

void uBlox::locate(int mode,long date,unsigned int time, double lon,double lat,int altitude,int certainty) // 0:GPS only, 1:local aiding, 2:active, 3:online assist
{
// AT+ULOC=2,x,0,y,z: x based on the setup used in gpsInit, y timeout in seconds, z desired accuracy
// reply : +UULOC:<date>,<time>,<lat>,<long>,<alt>,<uncertainty>
#ifdef DEBUG_ON
	Serial.print(F("DB:GPS LOCATE"));
  #endif
switch(mode)
    {
	case 0: // normal GPS only
	_cell << "AT+ULOC=2,1,0,80,200" << crl; // single shot position,use GPS only,basic response,120s timeout,200m accuracy please
	if(WaitResp(80000, 200, "+UULOC:"))
	{
	date = _tf.getValue('/'); // save the date DDMMYYYY
	_tf.find(","); // skip the comma
	time = _tf.getValue(':'); // save the time   HHMMSS
	_tf.find(","); // skip the comma
	lon = _tf.getFloat(); // save the longitude
	_tf.find(","); // skip the comma
	lat = _tf.getFloat(); // save the lattitude
	_tf.find(","); // skip the comma
	altitude = _tf.getValue(); // save the altitude
	_tf.find(","); // skip the comma
	certainty = _tf.getValue(); // save the certainty
	}
	break;
	case 1: // local aiding // depends on the services initialised in the gps
	attachGPRS(0,"internet", "guest", "guest"); // activate GPRS
	_cell << "AT+ULOC=2,1,0,80,200" << crl; // single shot position,use GPS only,basic response,120s timeout,200m accuracy please
	if(WaitResp(80000, 200, "+UULOC:"))
	{
	date = _tf.getValue('/'); // save the date DDMMYYYY
	_tf.find(","); // skip the comma
	time = _tf.getValue(':'); // save the time   HHMMSS
	_tf.find(","); // skip the comma
	lon = _tf.getFloat(); // save the longitude
	_tf.find(","); // skip the comma
	lat = _tf.getFloat(); // save the lattitude
	_tf.find(","); // skip the comma
	altitude = _tf.getValue(); // save the altitude
	_tf.find(","); // skip the comma
	certainty = _tf.getValue(); // save the certainty
	}
	break;
	case 2: // Assistnow offline ( will use a pre downloaded database )
	attachGPRS(0,"internet", "guest", "guest"); // activate GPRS// activate GPRS
	_cell << "AT+ULOC=2,1,0,80,200" << crl; // single shot position,use GPS only,basic response,120s timeout,200m accuracy please
	if(WaitResp(80000, 200, "+UULOC:"))
	{
	date = _tf.getValue('/'); // save the date DDMMYYYY
	_tf.find(","); // skip the comma
	time = _tf.getValue(':'); // save the time   HHMMSS
	_tf.find(","); // skip the comma
	lon = _tf.getFloat(); // save the longitude
	_tf.find(","); // skip the comma
	lat = _tf.getFloat(); // save the lattitude
	_tf.find(","); // skip the comma
	altitude = _tf.getValue(); // save the altitude
	_tf.find(","); // skip the comma
	certainty = _tf.getValue(); // save the certainty
	}
	break;
	case 3: // Assistnow online ( will use a pre downloaded database  and correlate online)
	attachGPRS(0,"internet", "guest", "guest"); // activate GPRS// activate GPRS
	_cell << "AT+ULOC=2,3,0,80,200" << crl; // single shot position,use GPS and GSM,basic response,120s timeout,200m accuracy please
	if(WaitResp(80000, 200, "+UULOC:"))
	{
	date = _tf.getValue('/'); // save the date DDMMYYYY
	_tf.find(","); // skip the comma
	time = _tf.getValue(':'); // save the time   HHMMSS
	_tf.find(","); // skip the comma
	lon = _tf.getFloat(); // save the longitude
	_tf.find(","); // skip the comma
	lat = _tf.getFloat(); // save the lattitude
	_tf.find(","); // skip the comma
	altitude = _tf.getValue(); // save the altitude
	_tf.find(","); // skip the comma
	certainty = _tf.getValue(); // save the certainty
	}
	break;
	case 4: // Assistnow autonomous( will contact u-blox server )
	attachGPRS(0,"internet", "guest", "guest"); // activate GPRS// activate GPRS
	_cell << "AT+ULOC=2,3,0,80,200" << crl; // single shot position,use GPS and GSM,basic response,120s timeout,200m accuracy please
	if(WaitResp(80000, 200, "+UULOC:"))
	{
	date = _tf.getValue('/'); // save the date DDMMYYYY
	_tf.find(","); // skip the comma
	time = _tf.getValue(':'); // save the time   HHMMSS
	_tf.find(","); // skip the comma
	lon = _tf.getFloat(); // save the longitude
	_tf.find(","); // skip the comma
	lat = _tf.getFloat(); // save the lattitude
	_tf.find(","); // skip the comma
	altitude = _tf.getValue(); // save the altitude
	_tf.find(","); // skip the comma
	certainty = _tf.getValue(); // save the certainty
	}
	break;
	case 5: // All aiding modes used
	#ifdef DEBUG_ON
	Serial.println(F(" 5"));
	#endif
	attachGPRS(0,"internet", "guest", "guest"); // activate GPRS
	_cell << "AT+ULOC=2,3,0,40,200" << crl; // single shot position,use GPS and GSM,basic response,40s timeout,200m accuracy please
	if(WaitResp(1000, 200, "OK"))
	{
	#ifdef DEBUG_ON
	Serial.println(F("OK:GOT"));
	#endif
	 
	delay(40000);
	//SimpleRead();
	
		if(WaitResp(40000, 200, "+UULOC"))
		{
		date = _tf.getValue('/'); // save the date DDMMYYYY
		//_tf.find(","); // skip the comma
		time = _tf.getValue(':'); // save the time   HHMMSS
		_tf.find(","); // skip the comma
		lon = _tf.getFloat(); // save the longitude
		//_tf.find(","); // skip the comma
		lat = _tf.getFloat(); // save the lattitude
		//_tf.find(","); // skip the comma
		altitude = _tf.getValue(); // save the altitude
		//_tf.find(","); // skip the comma
		certainty = _tf.getValue(); // save the certainty
		
		#ifdef DEBUG_ON
		Serial.println(F(" RESPONDED"));
		#endif
		}
		
		#ifdef DEBUG_ON
		Serial.println(F(" NO RESPONSE"));
		#endif
		
	}
	break;
	case 6: // use GSM only
	attachGPRS(0,"internet", "guest", "guest");
	_cell << "AT+ULOC=2,2,0,120,200" << crl; // single shot position,use GSM only,basic response,120s timeout,200m accuracy please
	if(WaitResp(20000, 200, "+UULOC:"))
	{
	date = _tf.getValue('/'); // save the date DDMMYYYY
	_tf.find(","); // skip the comma
	time = _tf.getValue(':'); // save the time   HHMMSS
	_tf.find(","); // skip the comma
	lon = _tf.getFloat(); // save the longitude
	_tf.find(","); // skip the comma
	lat = _tf.getFloat(); // save the lattitude
	_tf.find(","); // skip the comma
	altitude = _tf.getValue(); // save the altitude
	_tf.find(","); // skip the comma
	certainty = _tf.getValue(); // save the certainty
	}
	break;
	}
}

void uBlox::gpsOff() // turn GPS on or off, will always use hybrid positioning ( cellocate and GPS correlated )
{
	SendATCmdWaitResp(F("AT+UGPS=0"), 1000, 50, "OK", 2); 
}
