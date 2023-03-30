#ifndef _inf_commands_
#define _inf_commands_
#define NOT_FOUND -1
#include "communication.h"

extern int CANDebugMode, RebootAfterReflash;
bool HideRequest;
//char HOST[];

typedef struct
{
	uint32_t tx_len;
	uint32_t rx_len;
	unsigned char cmd_data[];
} cmmdStruct;

#pragma pack(push,1)

typedef struct
{	
	unsigned char bus;
	unsigned char fdf;
	unsigned char brs;
	unsigned char silent;
	uint32_t speed;
	uint32_t speed_fd;
	uint32_t rx_id;
} initStruct;

typedef struct
{	
   unsigned char Bus;   
   unsigned char Extended;
   unsigned char DLC;
   uint32_t ID;
   uint32_t TimeStamp;
   unsigned char Data[64];
} cmdCANstruct;

typedef struct
{	
	unsigned char Bus;   
   uint32_t Timeout;
} cmdRequestRX;

typedef struct
{
   unsigned char  Bus;  
   unsigned char  Flags;
   unsigned char  DLC;  
   uint32_t  ID;
   uint32_t  TimeStamp;
   uint32_t  Timeout;
   unsigned char  Data[64];
} CAN_TX_RX_STRUCT;

typedef struct
{	   
   unsigned char  Bus;  
   unsigned char  Flags;
   unsigned char  DLC;  
   uint32_t  ID;
   uint32_t  TimeStamp;
   unsigned char  Data[64];
} ResponseStruct;


typedef struct
{
    unsigned short  UID; 
    unsigned char InfSize;
    unsigned char DLC;
    uint32_t TimeStamp;
    uint32_t CANID;
    unsigned char CANFlag;
    unsigned char Data[8];
} structGNSSData;
	
#pragma pack(pop)

static const cmmdStruct cmmdGetFirmwareVersion = {2, 15,  {0x02, 0x00}}; 
static const cmmdStruct cmmdConfigInfo = {2, 26,  {0x10, 0x00}}; 
static const cmmdStruct cmmdGetSerialNumber = {2, 16,  {0x21, 0x00}}; 
static const cmmdStruct cmmdUSBStartLiveData = {3, 7,  {0x19, 0x00, 0x00}}; //Third Param is Channel 
static const cmmdStruct cmmdUSBStopLiveData = {3, 7,  {0x1A, 0x00, 0}};  //Third Param is Channel
static const cmmdStruct cmmdUSBGetLiveData = {3, 16,  {0x83, 0x00, 0}};  //Third Param is Channel
static const cmmdStruct cmmdDeinitBus = {2, 6,  {0x20, 0x00}}; 
static const cmmdStruct cmmdGetConfigSize = {2, 26,  {0x0A, 0x00}}; 
static const cmmdStruct cmmd41 = {4, 10,  {0x41, 0x00}};
static const cmmdStruct cmmd42 = {4, 10,  {0x42, 0x00}};
static const cmmdStruct cmmd43 = {4, 16,  {0x43, 0x00}};
static const cmmdStruct cmmdGetNFC = {2, 18,  {0x44, 0x00}}; 
static const cmmdStruct cmmdForceGoDeepSleep = {2, 6,  {0x45, 0x00}}; 
static cmmdStruct cmmdDateSet = {6, 6,  {0x12, 0x00}};
static const cmmdStruct cmmdDateGet = {2, 10,  {0x11, 0x00}}; 

unsigned short can0uid;
unsigned short can1uid;
unsigned short can2uid;
unsigned short can3uid;
unsigned short accXuid;
unsigned short accYuid;
unsigned short accZuid;
unsigned short gyroXuid;
unsigned short gyroYuid;
unsigned short gyroZuid;
unsigned short dig0uid;
unsigned short dig1uid;
unsigned short adc0uid;
unsigned short adc1uid;
unsigned short adc2uid;
unsigned short adc3uid;
unsigned short gnss0uid; // Latitude
unsigned short gnss1uid; // Longitude
unsigned short gnss2uid; // Altitude
unsigned short gnss3uid; // Datetime
unsigned short gnss4uid; // SpeedOverGround
unsigned short gnss5uid; // GroundDistance
unsigned short gnss6uid; // CourseOverGround
unsigned short gnss7uid; // GeoidSeparation
unsigned short gnss8uid; // NumberSatellites
unsigned short gnss9uid; // Quality
unsigned short gnss10uid; // HorizontalAccuracy
unsigned short gnss11uid; // VerticalAccuracy
unsigned short gnss12uid; // SpeedAccuracy

unsigned char SendCommand(DeviceStruct *devobj, unsigned char endpoint_id, const cmmdStruct *cmmd);
void SendStartLiveData(DeviceStruct *devobj, unsigned char channel);
void SendInitBus(DeviceStruct *devobj, initStruct *cmmdInit);
void SendCANCmd(DeviceStruct *devobj, cmdCANstruct *cmdCAN);		//
void ReadLiveData(DeviceStruct *devobj, unsigned char channel);
void ReadCANMsg(DeviceStruct *devobj, unsigned char bus, unsigned long msgCount);
unsigned char GetConfig(DeviceStruct *devobj);
int ReadFile();
void Reflash(DeviceStruct* devobj, char* filename);
void ApplyReflash(DeviceStruct* devobj);
void GreenLED_status(bool greenLEDstatus);
char* GetHostName(char res[]);
unsigned char SendConfig(DeviceStruct* devobj, char* filename);
void SendGNSSData(DeviceStruct *devobj, structGNSSData *GNSSData);	
void ReadUsbData(DeviceStruct *devobj, structGNSSData *GNSSData);	

#endif