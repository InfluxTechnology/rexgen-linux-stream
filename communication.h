#ifndef _inf_communication_
#define _inf_communication_

#include "libusb.h"
#include <stdbool.h>

#define TRANSFER_BLOCK_SIZE 0x4000

static const unsigned char epData = 0;
static const unsigned char epLive = 1;

#pragma pack(push,1)
typedef struct
{
	unsigned char id; // Endpoint id
	unsigned long timeout;
	unsigned long tx_len;
	unsigned long rx_len;
	unsigned char tx_data[2*TRANSFER_BLOCK_SIZE];
	unsigned char rx_data[2*TRANSFER_BLOCK_SIZE];
} EndpointCommunicationStruct;

typedef struct
{
	libusb_device_handle *handle;
	unsigned short vendor;
	unsigned short product;
	unsigned char interface;
	
	EndpointCommunicationStruct ep[2]; // 0 - tx/rx, 1 - live data
} DeviceStruct;
#pragma pack(pop)

bool InitUsbLibrary();
void ReleaseUsbLibrary();

bool InitDevice(DeviceStruct *devobj, unsigned short vendor, unsigned short product);
void ReleaseDevice(DeviceStruct *devobj);

unsigned char UsbSend(DeviceStruct *devobj, unsigned char endpoint_id);
unsigned char UsbRead(DeviceStruct *devobj, unsigned char endpoint_id);

#endif