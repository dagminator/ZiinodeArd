
#ifndef ZiinodeArd_h
#define ZiinodeArd_h

#include <Arduino.h>
#include <avr/eeprom.h>

#include <SPI.h>
#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>

#include "ByteBuffer.h"

//prefix
#define ACK 4
#define ENQ 5
#define CONN_ACK 6
#define TRAP 7  //
#define ANNOTATION 8  //
#define LOG 9  //

//command
#define CMD_DISCOVER 11   //character response  +
#define CMD_MAC 12  //6 byte mac
#define CMD_NET 13  //net settings IP+GW+SM+DNS = 4x4=16bytes
#define CMD_INT_TYPE 14  // 8 byte
#define CMD_TRIG 15  //  6 byte
#define CMD_TRIG_ALL  16  // TRIG_COUNT * 6 byte
#define CMD_TRAP_ADDR 17  //28 byte adderess
#define CMD_TRIG_OUT 18  // 2byte
#define CMD_POSTING_INTERVAL 19  // 2byte

#define DEBUG 1

#define APITYPE  "ARDETH1"
//#define APIKEY  "00000A1"
#define PIN  "1111"
#define VERSION  0

#define read_eeprom_byte(address) eeprom_read_byte ((const uint8_t*)address)
#define write_eeprom_byte(address,value) eeprom_write_byte ((uint8_t*)address,(uint8_t)value)
//#define read_eeprom_word(address) eeprom_read_word ((const uint16_t*)address)
//#define write_eeprom_word(address,value) eeprom_write_word ((uint16_t*)address,(uint16_t)value)
#define read_eeprom_dword(address) eeprom_read_dword ((const uint32_t*)address)
#define write_eeprom_dword(address,value) eeprom_write_dword ((uint32_t*)address,(uint32_t)value)
//#define read_eeprom_float(address) eeprom_read_float ((const float *)address)
//#define write_eeprom_float(address,value) eeprom_write_float ((float*)address,(float)value)
#define read_eeprom_array(address,value_p,length) eeprom_read_block ((void *)value_p, (const void *)address, length)
#define write_eeprom_array(address,value_p,length) eeprom_write_block ((const void *)value_p, (void *)address, length)

template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
    	eeprom_write_byte((unsigned char *)ee++, *p++);
    return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
	  *p++ = eeprom_read_byte((unsigned char *)ee++);
    return i;
}

class ZiinodeArd
{
public:
	ZiinodeArd();
	typedef void (*TOnDataHandler)(byte, ByteBuffer*);
	typedef void (*TOnConnAckHandler)();
	typedef void (*TOnAckHandler)();
    void    begin(byte *emac, TOnDataHandler handler, TOnConnAckHandler cack, TOnAckHandler ack);
    void    resetDevId();
    void    stop();
    void    begin(char const *apName);
    void debugL(const char *fmt, ...);
    void flush();
    size_t    write(uint8_t b);
    size_t    write(const uint8_t *buf, size_t size);
    void    writeInt(int in);
    void    writeIntE(int in);
    void    writeUint32(uint32_t in);
    void    writeAnnot(int code, const char *fmt, ...);
    void    writeLog(int code, const char *fmt, ...);
    void    writeBody(const uint8_t *buf, int size);
    void 	sendNet();
    void    sendInterval();
    void    sendBinTrap();
    void    sendEnq();
    void    ether_loop();
    boolean checkConn();
    uint8_t connected();
    int readThermistor(int adcpin);
    boolean hasConnected();
    void setDevId(char * did);
    
    void    setTimeout(unsigned long seconds);
    void    setDebugOutput(boolean debug);

    int     serverLoop();
    int16_t getInt();
    uint32_t postingInterval = 1000;

private:
    bool hasAddr = false;
    ByteBuffer* _clientBuffer;
    EthernetClient _client;
    TOnDataHandler _onDataHandler;
    TOnConnAckHandler _cack;
    TOnAckHandler _ack;
    char _devid[8];
    char address[20];
    boolean _hasDevId = false;
};



#endif
