
//needed for library
#include "ZiinodeArd.h"
#include "ds18b20.h" //http://www.pjrc.com/teensy/arduino_libraries/OneWire.zip   place under arduino libraries
#include "DHT.h" //https://github.com/adafruit/DHT-sensor-library    place under libraries and rename folder to DHT

#define MAC {0xDE, 0xAD, 0xBE, 0x01, 0x04, 0xED}

ZiinodeArd zn;

//analog 10k thermocouple
#define NTC10K 1
//dallas sensor
#define INPUT_DS 2
//digital themp & hummudity
#define INPUT_TH_11 3
#define INPUT_TH_22 4
//analog read
#define VOLTAGE 5

//total 4+1
#define IN_start 15
//total 20*6+1
#define TRIG_start 20

#define IN_COUNT 4
#define READ_COUNT 8
#define OUT_COUNT 4
#define TRIG_COUNT 20


typedef struct{
	uint8_t input;
	uint8_t oper;
	uint8_t out;
	uint8_t out_dir;
	int16_t val;
} trigger_t;//size 6


uint8_t in_pin[IN_COUNT] = { A0, A1, A2, A3};
uint8_t out_pins[OUT_COUNT] = { 9, 10, 11, 12 };

//reading digital may have hummuduty as well

//#define DEBUG 1


#define OPER_EQUAL 1
#define OPER_LESS 2
#define OPER_GREATER 3

int readings[READ_COUNT];

byte in_type[IN_COUNT];
trigger_t trigger[TRIG_COUNT];

byte out_s;

boolean hasAck=false;

void  savePostingInt(){
	write_eeprom_byte(14, 55);
	EEPROM_writeAnything(15,zn.postingInterval);
}

void saveInType(){
	write_eeprom_byte(19, 55);
	EEPROM_writeAnything(20,in_type);
}

void  saveTrig(){
	write_eeprom_byte(24, 55);
	EEPROM_writeAnything(25,trigger);
}

void readConf(){
  //total 4+1
  if(read_eeprom_byte(14)==55){
	  EEPROM_readAnything(15,zn.postingInterval);
  }

  if(read_eeprom_byte(19)==55){
	  EEPROM_readAnything(20,in_type);
  }
  //total 20*6+1
  if(read_eeprom_byte(24)==55){
	  EEPROM_readAnything(25,trigger);
  }
}



void sendInputType(){
	zn.write(CMD_INT_TYPE);
	zn.writeBody((const byte *)in_type,IN_COUNT);
}

void sendTrigAll(){
	zn.write(CMD_TRIG_ALL);
	zn.writeBody((const byte *)trigger,TRIG_COUNT * 6);
}

void sendBinTrap(){
	zn.write(TRAP);
	zn.writeInt((READ_COUNT * 2) + 1);
    for(int i=0;i<READ_COUNT;i++){
    	zn.writeIntE(readings[i]);
#if DEBUG
       	Serial.print(readings[i]);
        Serial.print(',');
#endif
    }
	//zn.write((const byte*)readings,8);
	zn.write(out_s);
#if DEBUG
	Serial.println();
#endif
}

void ack(){
#if DEBUG
		Serial.println("->ack....");
#endif
}

void cack(){
	hasAck = true;
	sendInputType();
	sendTrigAll();
	zn.sendNet();
#if DEBUG
	Serial.println("conn ack.... ");
#endif
}

void dataReceived(byte cmd, ByteBuffer *buf){
	if(cmd==CMD_POSTING_INTERVAL){
		zn.postingInterval = buf->getUInt32();
		savePostingInt();
	}else if(cmd==CMD_TRIG_OUT){
		byte idx = buf->get();
		uint8_t val = buf->get();
		if(idx<OUT_COUNT){
			uint8_t pin = out_pins[idx];
			uint8_t cur = digitalRead(pin);
			zn.writeLog(0,1,"setPin #:%i val:%i  idx:%i cur%i",pin,val, idx, cur);
			if(cur!=val){
				digitalWrite(pin,val);
				if(val){
					bitSet(out_s, idx);
				}else{
					bitClear(out_s, idx);
				}
			}
		}else{
			zn.writeLog(0,1,"ERR out idx:%i max:%i",idx,OUT_COUNT);
		}
	}else if(cmd==CMD_INT_TYPE){
		byte idx = buf->get();
		if(idx<IN_COUNT){
			in_type[idx]=buf->get();
			saveInType();
			zn.writeLog(0,1,"in type idx:%i val:%i",idx,in_type[idx]);
		}else{
			zn.writeLog(0,1,"ERR in type idx:%i max:%i",idx,IN_COUNT);
		}
	}else if(cmd==CMD_TRIG){
		//typedef struct{
		//	uint8_t input;
		//	uint8_t oper;
		//	uint8_t out;
		//	uint8_t out_dir;
		//	int val;
		//} trigger_t;//size 6
		byte idx = buf->get();
		if(idx<TRIG_COUNT){
			trigger[idx].input = buf->get();
			trigger[idx].oper = buf->get();
			trigger[idx].out = buf->get();
			trigger[idx].out_dir = buf->get();
			trigger[idx].val = buf->getInt();
			saveTrig();
			zn.writeLog(0,1,"trig idx:%i input:%i oper:%i out:%i out_dir:%i val:%i",idx,trigger[idx].input,trigger[idx].oper ,trigger[idx].out,trigger[idx].out_dir,trigger[idx].val);
		}else{
			zn.writeLog(0,1,"ERR ttrig idx:%i max:%i",idx,TRIG_COUNT);
		}
	}else if(cmd==CMD_TRIG_ALL){
		for(int idx=0;idx<TRIG_COUNT;idx++){
			trigger[idx].input = buf->get();
			trigger[idx].oper = buf->get();
			trigger[idx].out = buf->get();
			trigger[idx].out_dir = buf->get();
			trigger[idx].val = buf->getInt();
		}
		//EEPROM_writeAnything(e_trigger,trig);
	}else if(cmd==ACK){
		//ul_PreviousMillis = millis();
	}
	zn.write(ACK);
	zn.writeInt(1);
	zn.write(cmd);
	//zn.flush();
}


char oper[3] = {'=','<','>'};

void setPin(uint8_t i){
	if(i<OUT_COUNT){
		uint8_t pin = out_pins[trigger[i].out];
		uint8_t val = trigger[i].out_dir;
		if(digitalRead(pin)!=val){
			zn.writeLog(0,1,"setPin:%i val:%i",pin,val);
			zn.sendEvent(0,i,"Output changed #:%i state:%i on value:%i",trigger[i].out,val,trigger[i].val);
			digitalWrite(pin,val);
		}
		if(val){
			bitSet(out_s, trigger[i].out);
		}else{
			bitClear(out_s, trigger[i].out);
		}
	}else{
		zn.sendEvent(0,i,"Trigger #:%i on %i %c %i",i,trigger[i].val,oper[trigger[i].oper], readings[trigger[i].input]);
	}
}

void trigger_out() {
	for (uint8_t i = 0; i < TRIG_COUNT; i++) {
		if(trigger[i].oper>0 && trigger[i].oper<255){
			if(trigger[i].oper==OPER_EQUAL && trigger[i].val==readings[trigger[i].input] ){
				setPin(i);
			}if(trigger[i].oper==OPER_GREATER && trigger[i].val<readings[trigger[i].input] ){
				setPin(i);
			}if(trigger[i].oper==OPER_LESS && trigger[i].val>readings[trigger[i].input] ){
				setPin(i);
			}
		}
	}
}




void read_inputs() {

	for (int i = 0; i < IN_COUNT; i++) {
		///=====NTC10K=======================================================
		if(in_type[i]==NTC10K){
			readings[i * 2] = zn.readThermistor(in_pin[i]);
//#if DEBUG
//	zn.writeLog(1,"NTC10k :%i  temp:%i",i,readings[i * 2]);
//#endif
		}else if(in_type[i]==INPUT_DS){
			DS18B20 dsb(in_pin[i]);
			float ds=0;
			if(dsb.getTemperature(ds)==0){
				readings[i * 2] = ds * 10;
			}
//#if DEBUG
//	zn.writeLog(1,"DS:%i val:%i",in_pin[i],readings[i * 2]);
//#endif
		}else if(in_type[i]==INPUT_TH_11){
			DHT dht(in_pin[i], DHT11);
			dht.begin();
			readings[i * 2] = dht.readTemperature() * 10;
			readings[i * 2+1] = dht.readHumidity() * 10;
//#if DEBUG
//	zn.writeLog(1,"DS:%i humy:%i temp:%i",in_pin[i],readings[i * 2], readings[i * 2+1]);
//#endif
		}else if(in_type[i]==INPUT_TH_22){
			DHT dht(in_pin[i], DHT22);
			dht.begin();
			readings[i * 2] = dht.readTemperature() * 10;
			readings[i * 2+1] = dht.readHumidity() * 10;
//#if DEBUG
//	zn.debugL("DS:%i humy:%i temp:%i ",in_pin[i],readings[i * 2], readings[i * 2+1]);
//#endif
		}else if(in_type[i]==VOLTAGE){
			readings[i * 2]  = analogRead(in_pin[i]);
#if DEBUG
	zn.writeLog(0,1,"voltage:%i voltage:%i ",in_pin[i], readings[i * 2]);
#endif
		}
	}
}



uint32_t lastConnectionTime = 0;

void loop() {
//	if(!reset){
//		wdt_reset();
//	}
	zn.ether_loop();
	if((millis() - lastConnectionTime > zn.postingInterval)) {
		read_inputs();
		trigger_out();
		if(zn.checkConn()){
			if(hasAck){
				sendBinTrap();
			}
		}else{
			hasAck = false;
		}
		lastConnectionTime = millis();

		//enable this if you want to reset device id
		//if(digitalRead(2)){
		//	zn.resetDevId();
		//}
	}

}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //zn.resetDevId();
  byte emac[] = MAC;
  zn.begin(emac, dataReceived, cack, ack);
  //in seconds
  readConf();
  read_inputs();
  trigger_out();
}

