// SPDX-License-Identifier: GPL-2.0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "./pipes.h"

#include "../rexusb/communication.h"
#include "../rexusb/commands.h"

DeviceStruct _DeviceObj;
int _pipe_can0_rx = -1, _pipe_can0_tx = -1;
char _name_can0_rx[50], _name_can0_tx[50];

void handleSIGINT(int sig_num)
{
//printf("*** CTRL+C ***\n"); fflush(stdout);
//    do_close();
    exit(1);
}

void handleSIGPIPE(int sig_num)
{
    // prevents the application crash in case of broken pipe 
    sigset_t set;
    sigset_t old_state;

    // get the current state
    sigprocmask(SIG_BLOCK, NULL, &old_state);

    // add signal_to_block to that existing state
    set = old_state;
    sigaddset(&set, sig_num);

    // block that signal also
    sigprocmask(SIG_BLOCK, &set, NULL);
}

void check_pipes_exists()
{
    struct stat st = {0};
    char *dir_rexgen = "/var/run/rexgen";
    char *dir_can0[50], *dir_can1[50], *dir_imu[50];
    
    // check folders exists
    strcpy(dir_can0, dir_rexgen);
    strcat(dir_can0, "/can0/");
    strcpy(dir_can1, dir_rexgen);
    strcat(dir_can1, "/can1/");
    strcpy(dir_imu, dir_rexgen);
    strcat(dir_imu, "/imu/");

    if (stat(dir_rexgen, &st) == -1) 
        mkdir(dir_rexgen, 0666);
    if (stat(dir_can0, &st) == -1) 
        mkdir(dir_can0, 0666);
/*    if (stat(dir_can1, &st) == -1) 
        mkdir(dir_can1, 0666);
    if (stat(dir_imu, &st) == -1) 
        mkdir(dir_imu, 0666);
*/

    strcpy(_name_can0_rx, dir_can0);
    strcat(_name_can0_rx, "rx");
    strcpy(_name_can0_tx, dir_can0);
    strcat(_name_can0_tx, "tx");
}

void get_read_live_data(DeviceStruct *devobj)
{
    if (devobj->msg.rx_len < 2)
		return;
    
    unsigned short blockSize = 0;
    unsigned short pos = 2;

    blockSize =  ((unsigned short*)devobj->msg.rx_data)[0];

    while (pos < blockSize)	
    {
		unsigned short uid = ((unsigned short*)(devobj->msg.rx_data + pos))[0];
		unsigned char infSize = devobj->msg.rx_data[pos + 2];
		unsigned char dlc = devobj->msg.rx_data[pos + 3];
		unsigned int timestamp = ((unsigned int*)(devobj->msg.rx_data + pos + 4))[0];
		if (ChannelSupported(uid) == 0)  // All uid that can be displayed
		{
			if (uid == can0uid)
			{
				pthread_mutex_lock(&mutex_can0_rx);
				datasize_can0rx = 4 + infSize + dlc;
				memcpy(&(data_can0rx[0]), &(devobj->msg.rx_data[pos]), datasize_can0rx);
				pthread_mutex_unlock(&mutex_can0_rx);
			}
			else if (uid == can1uid)
			{
				pthread_mutex_lock(&mutex_can1_rx);
				datasize_can1rx = 4 + infSize + dlc;
				memcpy(&(data_can1rx[0]), &(devobj->msg.rx_data[pos]), datasize_can1rx);
				pthread_mutex_unlock(&mutex_can1_rx);
			}
   			else if (uid == accXuid || uid == accYuid || uid == accZuid)
   			{
				pthread_mutex_lock(&mutex_acc_rx);
				datasize_accrx = 4 + infSize + dlc;
				memcpy(&(data_accrx[0]), &(devobj->msg.rx_data[pos]), datasize_accrx);
				pthread_mutex_unlock(&mutex_acc_rx);
   			}
 			else if (uid == gyroXuid || uid == gyroYuid || uid == gyroZuid)
 			{
				pthread_mutex_lock(&mutex_gyro_rx);
				datasize_gyrorx = 4 + infSize + dlc;
				memcpy(&(data_gyrorx[0]), &(devobj->msg.rx_data[pos]), datasize_gyrorx);
				pthread_mutex_unlock(&mutex_gyro_rx);
 			}
		}

    	pos += infSize + dlc + 4;   //skip bytes from unknown frames
	}
}

void read_live_data(DeviceStruct *devobj)//, unsigned char channel)
{
    unsigned char cmd_data[3] = {0x19, 0x00, 0};//channel};

    BuildCustomCmmd(&devobj->msg, 3, 7, (unsigned char*)&cmd_data);

	if (UsbSend(devobj) != 0)
		return;
	if (UsbRead(devobj) != 0)
		return;

    devobj->msg.rx_len = 512;
	if (UsbReadEP(devobj, 3) != 0)	
		return;

    get_read_live_data(devobj);
}

void get_send_live_data(DeviceStruct *devobj)
{
}

void send_live_data(DeviceStruct *devobj)
{
	void PipeToCan(int channel, pthread_mutex_t* mutex, unsigned long* datasize, unsigned char* data)
	{
	    devobj->msg.tx_len = 0;
		pthread_mutex_lock(mutex);
		if (*datasize > 0)
		{
	    	devobj->msg.tx_len = *datasize;
			memcpy(&devobj->msg.tx_data[0], &(*data), devobj->msg.tx_len);
			*datasize = 0;
		}
		pthread_mutex_unlock(mutex);

		if (devobj->msg.tx_len > 0)
		{
			/*printf("can%d tx bytes: %d\n", channel, devobj->msg.tx_len);
			for (int i = 0; i < devobj->msg.tx_len; i++) 
				printf("%i ", devobj->msg.tx_data[i]);
			printf("\n");*/

    		UsbSendEP(devobj, 3);
    	}

	}

	PipeToCan(0, &mutex_can0_tx, &datasize_can0tx, &data_can0tx);
	PipeToCan(1, &mutex_can1_tx, &datasize_can1tx, &data_can1tx);
}

int main (int argc, char *argv[])
{
    int check_arg(char *arg)
    {
	for (int i = 1; i < argc; i++)
	if (strcmp(argv[i], arg) == 0)
    	    return 1;
	return 0;    
    }

    int i = 0;
    
    if (check_arg("-v") == 1)
    {	
        printf("1.1 \n");
	return;
    }

//    if (exec("ps | grep rexgen_data | wc -l") > 1)
//	return 0;

    signal(SIGINT, handleSIGINT);
    signal(SIGPIPE, handleSIGPIPE);

    check_pipes_exists();

    if (! InitUsbLibrary())
        return 1;
    if (! InitDevice(&_DeviceObj, 0x16d0, 0x0f14, 2))
        return 1;

    HideRequest = true;
    GetConfig(&_DeviceObj);

    InitPipes();
    while (true)
    {
	read_live_data(&_DeviceObj);

	send_live_data(&_DeviceObj);
    }
    DestroyPipes();

    //close(_pipe_can0_rx);
    return 0;
}
