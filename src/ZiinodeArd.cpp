
#include "ZiinodeArd.h"
#include "HttpClient.h"
#include <inttypes.h>
#include <avr/io.h>


ZiinodeArd::ZiinodeArd(){
	_clientBuffer = (ByteBuffer*)malloc(sizeof(ByteBuffer));
	_clientBuffer->init(156);
}

void ZiinodeArd::sendNet(){
//	_client.write(CMD_NET);
//	IPAddress ip =_client.localIP();
//	IPAddress gw =_client.gatewayIP();
//	IPAddress sm =_client.subnetMask();
//	IPAddress dns =_client.dnsIP();
//	net_t net = {{ip[0],ip[1],ip[2],ip[3]},{gw[0],gw[1],gw[2],gw[3]},{sm[0],sm[1],sm[2],sm[3]},{dns[0],dns[1],dns[2],dns[3]}};
//	writeBody((const byte*)&net,16);
}


size_t ZiinodeArd::write(uint8_t b)
{
    return _client.write(b);
}

size_t ZiinodeArd::write(const uint8_t *buf, size_t size){
	return _client.write(buf,size);
}

void  ZiinodeArd::writeInt(int in){
    byte *pointer = (byte *)&in;
    _client.write(pointer[1]);
    _client.write(pointer[0]);
}


void  ZiinodeArd::writeIntE(int in){
    byte *pointer = (byte *)&in;
    _client.write(pointer[0]);
    _client.write(pointer[1]);
}

void ZiinodeArd::writeUint32(uint32_t in){
    byte *pointer = (byte *)&in;
    _client.write(pointer[3]);
    _client.write(pointer[2]);
    _client.write(pointer[1]);
    _client.write(pointer[0]);
}

char* log_buf;
char lb[255];

void ZiinodeArd::writeInt64(int64_t in){
    byte *pointer = (byte *)&in;
    _client.write(pointer[7]);
    _client.write(pointer[6]);
    _client.write(pointer[5]);
    _client.write(pointer[4]);
    _client.write(pointer[3]);
    _client.write(pointer[2]);
    _client.write(pointer[1]);
    _client.write(pointer[0]);
}

//ANNOTATION
void ZiinodeArd::writeAnnot(int code, const char *fmt, ...){
	if(!log_buf){
		log_buf = lb;
	}
	va_list va;
	va_start (va, fmt);
	int size = vsprintf (log_buf, fmt, va);
	va_end (va);
	if(_client.connected()){
		_client.write(ANNOTATION);
		writeInt(size+4+8);
		writeInt64(0);
		writeInt(code);
		writeInt(size);
		_client.write((const uint8_t *)log_buf,size);
	}
}

void ZiinodeArd::writeLog(int code, const char *fmt, ...){
	if(!log_buf){
		log_buf = lb;
	}
	va_list va;
	va_start (va, fmt);
	int size = vsprintf (log_buf, fmt, va);
	va_end (va);
	if(_client.connected()){
		_client.write(LOG);
		writeInt(size+4+8);
		writeInt64(0);
		writeInt(code);
		writeInt(size);
		_client.write((const uint8_t *)log_buf,size);
	}
}


void  ZiinodeArd::writeBody(const uint8_t *buf, int size) {
	writeInt(size);
	_client.write(buf,size);
}

unsigned long ul_LastComm = 0UL;
boolean ZiinodeArd::checkConn(){
	if(!hasAddr){
		return false;
	}
	if(!_client.connected() || (ul_LastComm!=0 && (millis() - ul_LastComm) > ((postingInterval*3)+5000))){
	 stop();
	}
	return hasAddr;

}
void lastComm(){
	ul_LastComm = millis();
}


void  ZiinodeArd::sendInterval(){
	_client.write(CMD_POSTING_INTERVAL);
	writeInt(4);
	writeUint32(postingInterval);
}

uint8_t ZiinodeArd::connected(){
	if(!hasAddr){
		return 0;
	}
	return _client && (_client.connected() || _client.available());
}


// Number of milliseconds to wait without receiving any data before we give up
const uint16_t kNetworkTimeout = 30*1000;
// Number of milliseconds to wait if no data is available before trying again
const uint16_t kNetworkDelay = 100;

//const char *apitype = APITYPE;
//const char *apikey = APIKEY;
unsigned long timeoutStart;
byte cmd=0;
int size=0;

void ZiinodeArd::stop(){
#if DEBUG
	 Serial.println("stop..");
#endif
	_clientBuffer->clear();
	cmd=0;
	size=0;
	hasAddr = false;
	_client.stop();
}

void ZiinodeArd::begin(byte *emac, TOnDataHandler handler, TOnConnAckHandler cack, TOnAckHandler ack){
  _onDataHandler = handler;
  _cack = cack;
  _ack = ack;
	if(read_eeprom_byte(0)==55){
		read_eeprom_array(1,_devid,7);
	    _hasDevId = true;
	}else{
		strncpy(_devid,APITYPE,7);
	}

	 while (Ethernet.begin(emac) == 0) {
	#if DEBUG
		 Serial.println("DHCP failed");
	#endif
	  }
}

void ZiinodeArd::setDevId(char* did) {
  _hasDevId=true;
  strncpy(_devid,did,7);
#if DEBUG
	Serial.print("set devid:");
	Serial.println(_devid);
#endif
  write_eeprom_byte(0, 55);
  write_eeprom_array(1,_devid,7);
}

void ZiinodeArd::resetDevId() {
#if DEBUG
	 Serial.println("reset devid");
#endif
  hasAddr = false;
  _hasDevId=false;
  strncpy(_devid,APITYPE,7);
  write_eeprom_byte(0, 255);
  //write_eeprom_array(1,_devid.c_str(),7);
}

int16_t ZiinodeArd::getInt(){
	int ret;
    byte *pointer = (byte *)&ret;
	pointer[0] =  _client.read();
	pointer[1] =  _client.read();
	return ret;
}

void ZiinodeArd::ether_loop() {
	while(!hasAddr) {
		char path[40];
	    sprintf(path,"/api/v1/node/host/%s%s%s",APITYPE,_devid,PIN);

#if DEBUG
		Serial.println("[HTTP] begin...");
		Serial.println(path);
		Serial.println(APITYPE);
		Serial.println(_devid);
		Serial.println(PIN);
#endif
		// start connection and send HTTP header
		int err =0;
		_client.stop();
		HttpClient http(_client);
		err = http.get("www.ziinode.io", path);
		  if (err == 0)
		  {

			err = http.responseStatusCode();
			if (err == 200)
			{
				#if DEBUG
					Serial.print("Got status code:");
					Serial.println(err);
				#endif

			  // Usually you'd check that the response code is 200 or a
			  // similar "success" code (200-299) before carrying on,
			  // but we'll print out whatever response we get

			  err = http.skipResponseHeaders();
			  if (err >= 0)
			  {
		        int bodyLen = http.contentLength();
				#if DEBUG
					Serial.print("Content length is:");
					Serial.println(bodyLen);
					Serial.print("available:");
					Serial.println(http.available());
				#endif

		        // Now we've got to the body, so we can print it out
		        timeoutStart = millis();
		        // Whilst we haven't timed out & haven't reached the end of the body
		        int rec=0;

		        char resp[bodyLen];
		        while ( (http.connected() || http.available()) &&
		               ((millis() - timeoutStart) < kNetworkTimeout) )
		        {
		        	if (http.available() && rec<bodyLen) {
		        		while(http.available() && rec<bodyLen){
		        			resp[rec]=(char)http.read();
		        			rec++;
		        		}
		        		if(rec == 4 && bodyLen == 4 && resp[0]==resp[1] && resp[1]==resp[2] && resp[2]==resp[3] && resp[3]==0){
#if DEBUG
	Serial.println("wait for reg, got 0.0.0.0 ");
#endif
							http.stop();
							hasAddr = false;
							delay(5000);
							return;
		        		}else if(rec == bodyLen){
#if DEBUG
   Serial.print("hd:");
   Serial.print(_hasDevId);
   Serial.print(" resp:");
   Serial.println(resp);
#endif
							http.stop();
							if(!_hasDevId){
								setDevId(resp);
								return;
							}
							hasAddr = true;
							strncpy(address,resp,bodyLen);
							timeoutStart = millis();
							while(((millis() - timeoutStart) < kNetworkTimeout) ){
#if DEBUG
	Serial.println("connecting ..");
#endif
								if(_client.connect(address,8787)){
									ul_LastComm = 0UL;
									write(ENQ);
									write((const byte*)APITYPE,7);
									write((const byte*)_devid,7);
									write((const byte*)PIN,4);
									writeInt(VERSION);
#if DEBUG
	Serial.println("enq sent");
#endif
									return;
								}
							}
#if DEBUG
	Serial.println("conn timeout");
#endif
						    hasAddr = false;
		        		}
		        	}
		            else
		            {
#if DEBUG
	Serial.println("delay...");
#endif
		                delay(kNetworkDelay);
		            }
		        }
#if DEBUG
	Serial.println("http read timeout");
#endif
			  }else{
				#if DEBUG
					Serial.print("Failed to skip response headers");
					Serial.println(err);
				#endif
			  }
			}else {
			#if DEBUG
				Serial.print("Getting response failed:");
				Serial.println(err);
			#endif
			}
		  }
		  else
		  {
			#if DEBUG
				Serial.print("Connect failed:::");
				Serial.println(err);
			#endif
		  }
		  http.stop();
	}
	if (connected()){

		if(_client.available() > 0){
			if(cmd==0){
				lastComm();
				cmd = _client.read();
				size = 0;
				if(cmd==ACK){
					if( _ack != 0 ){
						_ack();
					}
					cmd = 0;
					return;
				}
				if(cmd==CONN_ACK){
					if( _cack != 0 ){
						_cack();
					}
					cmd = 0;
					return;
				}
			}
			if(cmd!=0 && _client.available() > 1 && size==0 && _clientBuffer->getSize()==0){
				size = getInt();
				timeoutStart = millis();
#if DEBUG
			Serial.print("av:");
			Serial.print(_client.available());
			Serial.print(" size:");
			Serial.println(size);
#endif
			}
			while(size>0 && _client.available() > 0  && _clientBuffer->getCapacity()>0 && size>0 && ((millis() - timeoutStart) < 2000)) {
				_clientBuffer->put(_client.read());
				size--;
			}

			if(size>0 && ((millis() - timeoutStart) > 2000)){
#if DEBUG
			Serial.print("tiemout:");
			Serial.println(cmd);
#endif
				cmd=0;
				size=0;
				_clientBuffer->clear();
			}
			if(cmd!=0 && _clientBuffer->getSize()>0 && size==0){
				//packet ready
#if DEBUG
			Serial.print("packet:");
			Serial.println(_clientBuffer->getSize());
#endif
				if( _onDataHandler != 0 ){
					_onDataHandler(cmd,_clientBuffer);
				}else{
					//
				}
				_clientBuffer->clear();
				cmd=0;
				size=0;
			}
		}
	}
}


// resistance at 25 degrees C
#define THERMISTORNOMINAL 10000
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3470
// the value of the 'other' resistor
#define SERIESRESISTOR 10000
//========================================

int samples[NUMSAMPLES];

float readAnalog(int adcpin){
	uint8_t i;
	float average;
// take N samples in a row, with a slight delay
	for (i = 0; i < NUMSAMPLES; i++) {
		samples[i] = analogRead(adcpin);
		delay(10);
	}
// average all the samples out
	average = 0;
	for (i = 0; i < NUMSAMPLES; i++) {
		average += samples[i];
	}
	return average /= NUMSAMPLES;
}

int ZiinodeArd::readThermistor(int adcpin) {
//	if (RawADC == 0) {
//		return 0;
//	}
	float average = readAnalog(adcpin);
//#if DEBUG
//	Serial.print("Average analog reading ");
//	Serial.println(average);
//#endif
	if (average == 0)
		return 0;
// convert the value to resistance
	average = 1023 / average - 1;
	average = SERIESRESISTOR / average;
//#if DEBUG
//	Serial.print("Thermistor resistance ");
//	Serial.println(average);
//#endif
	float steinhart;
	steinhart = average / THERMISTORNOMINAL; // (R/Ro)
	steinhart = log(steinhart); // ln(R/Ro)
	steinhart /= BCOEFFICIENT; // 1/B * ln(R/Ro)
	steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
	steinhart = 1.0 / steinhart; // Invert
	steinhart -= 273.15; // convert to C
//#if DEBUG
//	//Serial.print("Temperature ");
//	Serial.print(steinhart);
//	//Serial.println(" *C");
//#endif
//delay(1000);
	return (int) steinhart * 10;
}


