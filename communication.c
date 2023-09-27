#include <libusb.h>
#include <stdio.h>
#include <stdbool.h>
#include "communication.h"

libusb_context *ctx = NULL;

bool InitUsbLibrary()
{
	int r = libusb_init(&ctx); 
	if(r < 0) {
		// Debug output
		printf("Init Error %c\n", libusb_error_name(r));
		
		return false;
	}
	
	libusb_set_debug(ctx, 3); //set verbosity level to 3, as suggested in the documentation
	return true;
}

void ReleaseUsbLibrary()
{
	libusb_exit(ctx);
}

bool InitDevice(DeviceStruct *devobj, unsigned short vendor, unsigned short product)
{
	devobj->vendor = 0x16d0;
	devobj->product = 0x0f14;
	devobj->ep[0].id = 2;
	devobj->ep[0].timeout = 1000;
	devobj->ep[1].id = 3;
	devobj->ep[1].timeout = 1000;
	devobj->interface = 0;

	devobj->handle = libusb_open_device_with_vid_pid(ctx, devobj->vendor , devobj->product); 
	if(devobj->handle == NULL)
	{
		// Debug output
		perror("Cannot open device");
		
		return false;
	}
	
	// Detach kernel driver if attached
	if(libusb_kernel_driver_active(devobj->handle, 0) == 1)
		if(libusb_detach_kernel_driver(devobj->handle, 0) != 0) 
		{
			// Debug output
			printf("Kernel Driver Detached!\n");
			
			return false;
		}

	int r = libusb_claim_interface(devobj->handle, devobj->interface); 
	if(r < 0) {
		// Debug output
		printf("Cannot Claim Interface: %s\n", libusb_error_name(r));
		
		return false;
	}

	return true;
}

void ReleaseDevice(DeviceStruct *devobj)
{
	int r = libusb_release_interface(devobj->handle, devobj->interface);
	if(r!=0) {
		// Debug output
		printf("Cannot Release Interface: %c\n", libusb_error_name(r));

		return;
	}

	libusb_close(devobj->handle); //close the device we opened
}

unsigned char UsbSend(DeviceStruct *devobj, unsigned char endpoint_id)
{
	int actual;
	EndpointCommunicationStruct* ep = &devobj->ep[endpoint_id];
	int r = libusb_bulk_transfer(devobj->handle, ep->id | LIBUSB_ENDPOINT_OUT, ep->tx_data, ep->tx_len, &actual, ep->timeout);
	if(r == 0)
	{		
		if (actual != ep->tx_len) 
			printf("SENT %d out of %d bytes\n", actual, ep->tx_len);
	}
	else
	    perror("ERROR");
//		printf("ERROR: %c\n", libusb_error_name(r));
	return (unsigned char)r;
}

unsigned char UsbRead(DeviceStruct *devobj, unsigned char endpoint_id)
{
	int actual;
	EndpointCommunicationStruct* ep = &devobj->ep[endpoint_id];

	int r = libusb_bulk_transfer(devobj->handle, ep->id | LIBUSB_ENDPOINT_IN, ep->rx_data, ep->rx_len, &actual, ep->timeout);
	if (r != 0)
	{
		if (r == LIBUSB_ERROR_TIMEOUT)
		{
			//printf("Timeout - %i\r\n", r);
			return 6;
		}
   		perror("ERROR");
		//printf("ERROR: %c\n", libusb_error_name(r));
		return 6;
	}
	return 0;
}

