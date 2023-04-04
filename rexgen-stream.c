// SPDX-License-Identifier: GPL-2.0
// v1.5 - added sys pipe with support of NFC
// v1.6 - added canfd support, adc and digital support
// v1.7 - added gnss support, added 2 more adc
// v1.8 - added support for can2 and can3

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

#include "pipes.h"
#include "rexgen-stream.h"

#include "communication.h"
#include "commands.h"

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

void printkBuffer(void *data, int len, char* prefix)
{
    char buff[255];
    char* ptr = &buff[0];
    int i, c = 0;
    for (i = 0; i < len; i++)
    {
        ptr += sprintf(ptr, "%02X ", *((unsigned char*)(data + i)));
        if (ptr - &buff[0] > 200)
        {
            c++;
            *ptr = 0;
            printf("%s: %s (%i) - %s", "ReX", prefix, c, buff);
            ptr = &buff[0];
        }
    }
    *ptr = 0;

    if (c > 0)
        printf("%s: %s (%i) - %s", "ReX", prefix, c+1, buff);
    else
        printf("%s: %s - %s", "ReX", prefix, buff);
}

void parse_live_data(unsigned short pipe_flags)
{
	bool isAccelerometer(unsigned short uid)
	{
		return (uid == accXuid || uid == accYuid || uid == accZuid);
	}

	bool isGyroscope(unsigned short uid)
	{
		return (uid == gyroXuid || uid == gyroYuid || uid == gyroZuid);
	}

	bool isDigital(unsigned short uid)
	{
		return (uid == dig0uid || uid == dig1uid);
	}

	bool isADC(unsigned short uid)
	{
		return (uid == adc0uid || uid == adc1uid || uid == adc2uid || uid == adc3uid);
	}

	bool isGnss(unsigned short uid)
	{
		return (
			uid == gnss0uid || uid == gnss1uid || uid == gnss2uid || uid == gnss3uid || uid == gnss4uid ||
			uid == gnss5uid || uid == gnss6uid || uid == gnss7uid || uid == gnss8uid || uid == gnss9uid ||
			uid == gnss10uid || uid == gnss11uid || uid == gnss12uid
			);
	}

	unsigned char q[max_queue_buffers][pipe_buffer_size];
	unsigned char qcount = 0;

	int i;
	pthread_mutex_lock(&mutex_usb);
	if (queue.qidx1 != queue.qidx2)
	{

		i = queue.qidx1;
		do 
		{
			{
				memcpy(&(q[qcount][0]), &(queue.data[i][0]), pipe_buffer_size);
				qcount++;
			}
			if (++i == max_queue_buffers)
				i = 0;
		} while (i != queue.qidx2);
	}
	queue.qidx1 = 0;
	queue.qidx2 = 0;
	pthread_mutex_unlock(&mutex_usb);

	//printf("Queue count: %i\r\n", qcount);
	if (qcount == 0)
		return;

	unsigned char* data;

	for (int qb = 0; qb < qcount; qb++)
	{
		data = &q[qb][0];
		//datasize = &q[qb].datasize;

    	unsigned short blockSize = 0;
    	unsigned short pos = 2;

    	blockSize =  ((unsigned short*)data)[0];
    	while (pos < blockSize)	
    	{
			unsigned short uid = ((unsigned short*)(data + pos))[0];
			unsigned char infSize = data[pos + 2];
			unsigned char dlc = data[pos + 3];
			unsigned int timestamp = ((unsigned int*)(data + pos + 4))[0];
			unsigned long recsize = 4 + infSize + dlc;

			if (uid == 0)
			{

			}
			else if ((pipe_flags & flag_can0) && uid == can0uid)
			{
				pthread_mutex_lock(&mutex_can0_rx);
				if (datasize_can0rx + recsize > pipe_buffer_size)
					datasize_can0rx = 0;
				memcpy(&(data_can0rx[datasize_can0rx]), &(data[pos]), recsize);
				datasize_can0rx+= recsize;
				pthread_mutex_unlock(&mutex_can0_rx);
			}
			else if ((pipe_flags & flag_can1) && uid == can1uid)
			{
				pthread_mutex_lock(&mutex_can1_rx);
				if (datasize_can1rx + recsize > pipe_buffer_size)
					datasize_can1rx = 0;
				memcpy(&(data_can1rx[datasize_can1rx]), &(data[pos]), recsize);
				datasize_can1rx+= recsize;
				pthread_mutex_unlock(&mutex_can1_rx);
			}
			else if ((pipe_flags & flag_can2) && uid == can2uid)
			{
				pthread_mutex_lock(&mutex_can2_rx);
				if (datasize_can2rx + recsize > pipe_buffer_size)
					datasize_can2rx = 0;
				memcpy(&(data_can2rx[datasize_can2rx]), &(data[pos]), recsize);
				datasize_can2rx+= recsize;
				pthread_mutex_unlock(&mutex_can2_rx);
			}
			else if ((pipe_flags & flag_can3) && uid == can3uid)
			{
				pthread_mutex_lock(&mutex_can3_rx);
				if (datasize_can3rx + recsize > pipe_buffer_size)
					datasize_can3rx = 0;
				memcpy(&(data_can3rx[datasize_can3rx]), &(data[pos]), recsize);
				datasize_can3rx+= recsize;
				pthread_mutex_unlock(&mutex_can3_rx);
			}
   			else if ((pipe_flags & flag_acc) && isAccelerometer(uid))
   			{
				pthread_mutex_lock(&mutex_acc_rx);
				if (datasize_accrx + recsize > pipe_buffer_size)
					datasize_accrx = 0;
				memcpy(&(data_accrx[datasize_accrx]), &(data[pos]), recsize);
				datasize_accrx+= recsize;
				pthread_mutex_unlock(&mutex_acc_rx);
   			}
 			else if ((pipe_flags & flag_gyro) && isGyroscope(uid))
 			{
				pthread_mutex_lock(&mutex_gyro_rx);
				if (datasize_gyrorx + recsize > pipe_buffer_size)
					datasize_gyrorx = 0;
				memcpy(&(data_gyrorx[datasize_gyrorx]), &(data[pos]), recsize);
				datasize_gyrorx+= recsize;
				pthread_mutex_unlock(&mutex_gyro_rx);
 			}
 			else if ((pipe_flags & flag_dig) && isDigital(uid))
 			{
				pthread_mutex_lock(&mutex_dig_rx);
				if (datasize_digrx + recsize > pipe_buffer_size)
					datasize_digrx = 0;
				memcpy(&(data_digrx[datasize_digrx]), &(data[pos]), recsize);
				datasize_digrx+= recsize;
				pthread_mutex_unlock(&mutex_dig_rx);
 			}
 			else if ((pipe_flags & flag_adc) && isADC(uid))
 			{
				pthread_mutex_lock(&mutex_adc_rx);
				if (datasize_adcrx + recsize > pipe_buffer_size)
					datasize_adcrx = 0;
				memcpy(&(data_adcrx[datasize_adcrx]), &(data[pos]), recsize);
				datasize_adcrx+= recsize;
				pthread_mutex_unlock(&mutex_adc_rx);
 			}
 			else if ((pipe_flags & flag_gnss) && isGnss(uid))
 			{
				pthread_mutex_lock(&mutex_gnss_rx);
				if (datasize_gnssrx + recsize > pipe_buffer_size)
					datasize_gnssrx = 0;
				memcpy(&(data_gnssrx[datasize_gnssrx]), &(data[pos]), recsize);
				datasize_gnssrx+= recsize;
				pthread_mutex_unlock(&mutex_gnss_rx);
 			}

    		pos += recsize;   //skip bytes from unknown frames
		}
	}
	//usleep(1);
}

void send_live_data(unsigned short pipe_flags)
{
	EndpointCommunicationStruct *ep = &devobj.ep[epLive];

	void PipeToCan(int channel, pthread_mutex_t* mutex, unsigned long* datasize, unsigned char* data)
	{
	    ep->tx_len = 0;
		pthread_mutex_lock(mutex);
		if (*datasize > 0)
		{
	    	ep->tx_len = *datasize;
			memcpy(&ep->tx_data[0], &(*data), ep->tx_len);
			*datasize = 0;
		}
		pthread_mutex_unlock(mutex);

		if (ep->tx_len > 0)
		{
			/*printf("can%d tx bytes: %d\n", channel, ep->tx_len);
			for (int i = 0; i < ep->tx_len; i++) 
				printf("%i ", ep->tx_data[i]);
			printf("\n");*/

    		UsbSend(&devobj, epLive);
    	}

	}

	if (pipe_flags & flag_can0)
		PipeToCan(0, &mutex_can0_tx, &datasize_can0tx, &data_can0tx);
	if (pipe_flags & flag_can1)
		PipeToCan(1, &mutex_can1_tx, &datasize_can1tx, &data_can1tx);
	if (pipe_flags & flag_can2)
		PipeToCan(2, &mutex_can2_tx, &datasize_can2tx, &data_can2tx);
	if (pipe_flags & flag_can3)
		PipeToCan(3, &mutex_can3_tx, &datasize_can3tx, &data_can3tx);
}

void read_live_data()
{
	EndpointCommunicationStruct* ep = &devobj.ep[epLive];

    ep->rx_len = 512;
	if (UsbRead(&devobj, epLive) == 0)
	{
		//PrintLiveData(ep);
		pthread_mutex_lock(&mutex_usb);
		memcpy(&queue.data[queue.qidx2], &(ep->rx_data[0]), ep->rx_len);
		if (++queue.qidx2 == max_queue_buffers)
			queue.qidx2 = 0;
		if (queue.qidx1 == queue.qidx2)
			queue.qidx1++;
		if (queue.qidx1 == max_queue_buffers)
			queue.qidx1 = 0;
		pthread_mutex_unlock(&mutex_usb);
	}
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
        printf("Stream tool	1.8 \n");
		return;
    }

//    if (exec("ps | grep rexgen_data | wc -l") > 1)
//	return 0;

    signal(SIGINT, handleSIGINT);
    signal(SIGPIPE, handleSIGPIPE);

    if (! InitUsbLibrary())
        return 1;
    if (! InitDevice(&devobj, 0x16d0, 0x0f14))
        return 1;

    HideRequest = true;

	CANDebugMode = 0;
	unsigned short pipe_flags = 0;

    GetConfig(&devobj);
    if (can0uid != 0)
    	pipe_flags |= flag_can0;
    if (can1uid != 0)
    	pipe_flags |= flag_can1;
    if (can2uid != 0)
    	pipe_flags |= flag_can2;
    if (can3uid != 0)
    	pipe_flags |= flag_can3;
    if (accXuid != 0 || accYuid != 0 || accZuid != 0)
    	pipe_flags |= flag_acc;
    if (gyroXuid != 0 || gyroYuid != 0 || gyroZuid != 0)
    	pipe_flags |= flag_gyro;
    if (dig0uid != 0 || dig1uid != 0)
    	pipe_flags |= flag_dig;
    if (adc0uid != 0 || adc1uid != 0 || adc2uid != 0 || adc3uid != 0)
    	pipe_flags |= flag_adc;
    if (gnss0uid != 0 || gnss1uid != 0 || gnss2uid != 0 || gnss3uid != 0 || gnss4uid != 0 ||
    	gnss5uid != 0 || gnss6uid != 0 ||	gnss7uid != 0 || gnss8uid != 0 || gnss9uid != 0 ||
    	gnss10uid != 0 || gnss11uid != 0 || gnss12uid != 0)
    	pipe_flags |= flag_gnss;

    printf(
    	"Detected UIDs (%i):\n"
    	"CAN 0/1/2/3 - %i/%i/%i/%i\n"
    	"Accelerometer X/Y/Z - %i/%i/%i\n"
    	"Gyroscope X/Y/Z - %i/%i/%i\n"
    	"Digital 0/1 - %i/%i\n"
    	"ADC 0/1/2/3 - %i/%i/%i/%i\r\n"
    	"GNSS: Latitude - %i, Longitude - %i, Altitude - %i, Datetime - %i, SpeedOverGround - %i, "
    	"GroundDistance - %i, CourseOverGround - %i, GeoidSeparation - %i, NumberSatellites - %i, "
    	"Quality - %i, HorizontalAccuracy - %i, VerticalAccuracy - %i, SpeedAccuracy - %i\r\n",
    	pipe_flags,
    	can0uid, can1uid, can2uid, can3uid,
    	accXuid, accYuid, accZuid, 
    	gyroXuid, gyroYuid, gyroZuid,
    	dig0uid, dig1uid,
    	adc0uid, adc1uid, adc2uid, adc3uid,
		gnss0uid, gnss1uid, gnss2uid, gnss3uid, gnss4uid, gnss5uid, gnss6uid,
		gnss7uid, gnss8uid, gnss9uid, gnss10uid, gnss11uid, gnss12uid
    );

    SendStartLiveData(&devobj, 0);
    InitPipes(pipe_flags);

    while (true)
    {
    	read_live_data();
		send_live_data(pipe_flags);
    }
    DestroyPipes();

    //close(_pipe_can0_rx);
    return 0;
}
