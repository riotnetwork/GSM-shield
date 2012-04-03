#ifndef uBlox_H
#define uBlox_H
#include <SoftwareSerial.h>
#include "GSM.h"

class uBlox : public virtual GSM
{

  private:
    int configandwait(char* pin);
    int setPIN(char *pin);
    int changeNSIPmode(char);

  public:
    uBlox();
    ~uBlox();
    int getCCI(char* cci);
	int poweroff();
	int softReset();
	int getIMEI(char* imei);
    int sendSMS(const char* to, const char* msg);
    int attachGPRS(int profile,char* domain, char* dom1, char* dom2);
    boolean readSMS(char* msg, int msglength, char* number, int nlength);
    boolean readCall(char* number, int nlength);
    boolean call(char* number, unsigned int milliseconds);
    int detachGPRS(int profile);
	int createTCPsocket();
	//char DNSlookup(char *server);
	int ip(char* ip);
    int connectTCP(const char* server, int port, int id);
    int disconnectTCP(int id);
    int connectTCPServer(int port);
    boolean connectedClient(int id);
    virtual int read(char* result, int resultlength);
	virtual uint8_t read();
	int signal(int param);
    int readCellData(int &mcc, int &mnc, long &lac, long &cellid);
	int available();
    void SimpleRead();
    void WhileSimpleRead();
    void SimpleWrite(char *comm);
	void SimpleWrite(String comm);
    void SimpleWrite(const char *comm);
	void SimpleWrite(__FlashStringHelper* comm);
	void SimpleWriteWOln(char *comm);
    void SimpleWrite(int comm);
	void SimpleWriteln(char *comm);
	void SimpleWriteln(const char *comm);
	void SimpleWriteln(int comm);
	
	
	void gps(int mode); // turn GPS on or off
void locate(int mode,long date,unsigned int time, double lon,double lat,int altitude,int certainty); // 0:GPS only, 1:local aiding, 2:active, 3:online assist, 5 all modes, 6 gsm only
void gpsOff();

};

extern uBlox gsm;

#endif

