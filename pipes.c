// SPDX-License-Identifier: GPL-2.0

#include "pipes.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "rexgen-stream.h"
#include "../rexusb/commands.h"

char* add_msg_val(char msg[], unsigned int val, char type)
{
    char *s[50];

    if (type == 0)
        sprintf(s, "%u", val);
    else if (type == 1)
        sprintf(s, "%X", val);
    else if (type == 2)
        sprintf(s, "%.2X", val);
    else if (type == 3)
        sprintf(s, "%d", val);
    
    if (type == 0)
        strcat(msg, "(");
    if (type == 3)
        strcat(msg, "[");

    strcat(msg, s);

    if (type == 0)
        strcat(msg, ")");
    if (type == 3)
        strcat(msg, "]");

    strcat(msg, " ");
    return msg;
}

void CanRx(char* name, pthread_mutex_t* mutex, unsigned long* datasize, unsigned char* data)
{
   	char *path[50];
	sprintf(path, "%s/%s/rx", dir_rexgen, name);

    printf("open pipe: %s\n", path);
	int pipe = open(path, O_WRONLY | O_SYNC);

	if (pipe == -1)
	{
		printf("pipe id: %s failed to open\n", path);
		return NULL;
	}

   	char *msg[250];

   	unsigned long d_size;
   	unsigned char d_data[pipe_buffer_size];

   	void Records2Pipe(unsigned char* data, unsigned long size)
   	{
    	unsigned long d_pos = 0;
	    while (d_pos < size)
	    {
			unsigned short uid = ((unsigned short*)(data + d_pos))[0];
    	    unsigned char infSize = data[d_pos + 2];
			unsigned char dlc = data[d_pos + 3];
    	    unsigned int timestamp = ((unsigned int*)(data + d_pos + 4))[0];
			unsigned int canID = ((unsigned int*)(data + d_pos + 8))[0];
			unsigned char flags = data[d_pos + 12];

			strcpy(msg, "");
			char* ptr = msg;
			ptr+= sprintf(ptr, "(%u)  %s  ", timestamp, name);
			switch (flags & 0x0C)
			{
				case 0x00: 
					ptr+= sprintf(ptr, "    ");
					break;
				case 0x04:
					ptr+= sprintf(ptr, "[F] ");
					break;
				case 0x0C:
					ptr+= sprintf(ptr, "[FB]");
					break;
			}
			ptr+= sprintf(ptr, "  ");
			if (flags & 0x01)
				ptr+= sprintf(ptr, "%08X ", canID);
			else
				ptr+= sprintf(ptr, "     %03X ", canID);
    	    ptr+= sprintf(ptr, "[%i]", dlc);

			for (int i = 0; i < dlc; i++)
				ptr+= sprintf(ptr, " %02X", d_data[d_pos + i + 4 + infSize]);

			ptr+= sprintf(ptr, "\n\0");
			write(pipe, msg, strlen(msg));

			d_pos+= 4 + infSize + dlc;
    	}
   	}

    while (true)
    {
	    pthread_mutex_lock(mutex);
	    d_size = *datasize;
	    *datasize = 0;
	    if (d_size > 0)
	    	memcpy(&(d_data[0]), &(*data), d_size);
    	pthread_mutex_unlock(mutex);
	    usleep(1);
	    Records2Pipe(&(d_data[0]), d_size);
    }

    close(pipe);
}

void* execute_can0rx(void* ptr)
{
	//CanRx(can0, &mutex_rx_pipe, &datasize_can0rx, &data_can0rx);
	CanRx(can0, &mutex_can0_rx, &datasize_can0rx, &data_can0rx);

    return NULL;
}

void* execute_can1rx(void* ptr)
{
	//CanRx(can1, &mutex_rx_pipe, &datasize_can1rx, &data_can1rx);
	CanRx(can1, &mutex_can1_rx, &datasize_can1rx, &data_can1rx);

    return NULL;
}

bool startsWith(const char *pre, const char *str)
{
	size_t lenpre = strlen(pre);
	size_t lenstr = strlen(str);
	return lenstr < lenpre ? false : memcmp(pre, str, lenpre) == 0;
}

bool _StrToLong(const char* str, unsigned long* val, int base) 
{
    char *end;
    *val = strtol(str, &end, base);

    //  if the converted value would fall out of the range of the result type.
    if ((errno == ERANGE && (*val == LONG_MAX || *val == LONG_MIN)) || (errno != 0 && *val == 0))
        return false;   

    // No digits were found.
    if (end == str)
        return false;   

    // check if the string was fully processed.
    return *end == '\0';
}

bool StrToLong(const char* str, unsigned long* val) 
{
	return _StrToLong(str, val, 10);
}

bool HexToLong(const char* str, unsigned long* val) 
{
	return _StrToLong(str, val, 16);
}

void CanTx(char* name, unsigned short txuid, pthread_mutex_t* mutex, unsigned long* datasize, unsigned char* data)
{
    char *buffer = (char*)malloc(512);
    unsigned short buffidx = 2;

	void BufferToApp()
	{
		//memcpy(&(buffer[0]), &buffidx, 2);

   		bool empty, dowait = true;
   		while (dowait)
   		{
   			pthread_mutex_lock(mutex);
    		empty = *datasize == 0;
   			if (empty)
   			{
    			*datasize = buffidx;
   				memcpy(&(*data), &(buffer[0]), buffidx);
   				dowait = false;
   			}
   			pthread_mutex_unlock(mutex);
   		}
   		buffidx = 2;
	}

	void Row2Bin(TxCanRow row, int count)
	{
		unsigned long tmp;
		unsigned char flags = 0;
		unsigned char containflags = 0;

		buffidx = 0;

		if (row[2][0] == '[')
		{
			if (row[2][1] == 'F')
			{
				flags|= 4;
				if (row[2][2] == 'B')
					flags|= 8;
			}
			containflags = 1;
		}

		if (!StrToLong(row[3 + containflags], &tmp))
			return;

		unsigned char dlc = tmp;
		//if (buffidx + 4 + 9 + dlc > 512)
			//BufferToApp();

		if (dlc < 0 || dlc > 64 || 4 + dlc + containflags != count)
			return;

		if (!StrToLong(row[1], &tmp))
			return;
		int can = tmp;
		memcpy(&(buffer[buffidx + 0]), &txuid, 2);

		buffer[buffidx + 2] = (char)9;
		buffer[buffidx + 3] = dlc;

		buffidx+= 4;

		unsigned long ts;
		if (!StrToLong(row[0], &ts))
			return;
		memcpy(&(buffer[buffidx + 0]), &ts, 4);

		unsigned long ident;
		if (!HexToLong(row[2 + containflags], &ident))
			return;
		memcpy(&(buffer[buffidx + 4]), &ident, 4);
		
		if (strlen(row[2 + containflags]) > 3)
			flags|= 1;
		buffer[buffidx + 8] = flags;
		
		buffidx+= 9;
		for (int i = 0; i < dlc; i++)
		{
			if (!HexToLong(row[4 + i + containflags], &tmp))
				return;
			buffer[buffidx + i] = tmp;
		}

		buffidx+= dlc;

		BufferToApp();
	}

    char *line;
    ssize_t dread;

   	char *path[50];
	sprintf(path, "%s/%s/tx", dir_rexgen, name);

	FILE * pipe;
    size_t len = 0;
    ssize_t nread;
    TxCanRow row;
    int rc;

    while (true)
    {
	    pipe = fopen(path, "r");
		if (pipe == -1)
		{
			printf("pipe id: %s failed to open\n", path);
			continue;
		}

	   	// Init buffer
	    memset(buffer, 0, 512);
	    buffidx = 2;

    	while ((nread = getline(&line, &len, pipe)) != -1)
    	{
    		//printf("line: %s\n", line);
    		rc = 0;
    		row[rc] = strtok(line, " ()[]\n");
    		while (row[rc] != NULL)
    		{
	      		rc++;
	      		if (rc == 2)
      				row[rc] = strtok(NULL, " \n");
      			else
      				row[rc] = strtok(NULL, " ()[]\n");
   			}
   			// At least 4 words are needed - Timestamp, Can channel, Ident, Dlc
	   		if (rc < 4)
   				continue;

   			// Remove leading can symbols
   			if (startsWith("can", row[1]))
	    		row[1]+= 3;
	    	else
	    		continue;

   			/*printf("Detected %d words: ", rc);
	    	for (int i = 0; i < rc; i++)
      			printf( "%s, ", row[i] );
   			printf("\n");*/

	    	Row2Bin(row, rc);
    	}

    	//if (buffidx > 2)
    		//BufferToApp();

	    fclose(pipe);
    }


    if (line)
        free(line);
}

void* execute_can0tx(void* ptr)
{
	CanTx(can0, 1200, &mutex_can0_tx, &datasize_can0tx, &data_can0tx);
    return NULL;
}

void* execute_can1tx(void* ptr)
{
	CanTx(can1, 1201, &mutex_can1_tx, &datasize_can1tx, &data_can1tx);
    return NULL;
}

char* UidToStringName(unsigned short uid)
{
	if (uid == gyroXuid || uid == accXuid)
		return "X";
	if (uid == gyroYuid || uid == accYuid)
		return "Y";
	if (uid == gyroZuid || uid == accZuid)
		return "Z";
	if (uid == dig0uid || uid == adc0uid)
		return "0";
	if (uid == dig1uid || uid == adc1uid)
		return "1";
	return "NA";
}

void ByteWriter(char* targetstr, unsigned short uid, unsigned int timestamp, char* data)
{
	sprintf(targetstr, "(%u) %s %u\n", timestamp, 
		UidToStringName(uid), ((unsigned char*)(data))[0]
	);
}

void FloatWriter(char* targetstr, unsigned short uid, unsigned int timestamp, char* data)
{
	sprintf(targetstr, "(%u) %s %f\n", timestamp, 
		UidToStringName(uid), ((float*)(data))[0]
	);
}

void SensorRx(char* name, pthread_mutex_t* mutex, unsigned long* datasize, unsigned char* data, 
	void (*data_writer)(char* targetstr, unsigned short uid, unsigned int timestamp, char* data))
{
   	char *path[50];
	sprintf(path, "%s/%s/rx", dir_rexgen, name);

    printf("open pipe: %s\n", path);
	int pipe = open(path, O_WRONLY);

	if (pipe == -1)
	{
		printf("pipe id: %s failed to open\n", path);
		return NULL;
	}

   	char *msg[50];

   	unsigned long d_size;
   	unsigned char d_data[pipe_buffer_size];

   	void Records2Pipe()
   	{
    	unsigned long d_pos = 0;
	    while (d_pos < d_size)
	    {

			unsigned short uid = ((unsigned short*)(d_data + d_pos))[0];
    	    unsigned char infSize = d_data[d_pos + 2];
			unsigned char dlc = d_data[d_pos + 3];
    	    unsigned int timestamp = ((unsigned int*)(d_data + d_pos + 4))[0];

    	    data_writer(msg, uid, timestamp, d_data + d_pos + 4 + infSize);

			write(pipe, msg, strlen(msg));

			d_pos+= 4 + infSize + dlc;
    	}
   	}

    while (true)
    {
	    pthread_mutex_lock(mutex);
	    d_size = *datasize;
	    if (d_size > 0)
	    	memcpy(&(d_data[0]), &(*data), d_size);
	    *datasize = 0;
    	pthread_mutex_unlock(mutex);

    	usleep(1);
	    Records2Pipe();
    }

    close(pipe);
}

void* execute_gyrorx(void* ptr)
{
	SensorRx(gyro, &mutex_gyro_rx, &datasize_gyrorx, &data_gyrorx, &FloatWriter);
    return NULL;
}

void* execute_accrx(void* ptr)
{
	SensorRx(acc, &mutex_acc_rx, &datasize_accrx, &data_accrx, &FloatWriter);
    return NULL;
}

void* execute_digrx(void* ptr)
{
	SensorRx(dig, &mutex_dig_rx, &datasize_digrx, &data_digrx, &ByteWriter);
    return NULL;
}

void* execute_adcrx(void* ptr)
{
	SensorRx(adc, &mutex_adc_rx, &datasize_adcrx, &data_adcrx, &FloatWriter);
    return NULL;
}

void* execute_io(void* ptr)
{
   	char *path[50];
	sprintf(path, "%s/%s", dir_rexgen, sys);

    printf("open pipe: %s\n", path);
	int pipe = open(path, O_WRONLY);

	if (pipe == -1)
	{
		printf("pipe id: %s failed to open\n", path);
		return NULL;
	}

   	char *msg[150];

	EndpointCommunicationStruct* ep = &devobj.ep[epData];

	uint cycle = 0;
	while (true)
	{
		if (cycle % nfc_io_cycles == 0)
		{
			SendCommand(&devobj, epData, &cmmdGetNFC);
			//PrintRxCmmd(ep);
			char* ptr = &msg[0];

			time_t raw_time = ((unsigned int*)(ep->rx_data + 13))[0];
			//time(&raw_time);
			struct tm ts = *localtime(&raw_time);
			ptr+= strftime(ptr, 150, "%a %Y-%m-%d %H:%M:%S", &ts);
			ptr+= sprintf(ptr, ", NFC: ");
			for (int i = 0; i < 7; i++)
			{
				ptr += sprintf(ptr, "%02X", *((unsigned char*)(ep->rx_data + 5 + i)));
			}
			if (ep->rx_data[12] == 1 || ep->rx_data[12] == 2)
				ptr+= sprintf(ptr, ", Status: Valid\r\n");
			else if (ep->rx_data[12] == 0xFF)
				ptr+= sprintf(ptr, ", Status: Invalid\r\n");
			/*else if (ep->rx_data[12] == 0)
				ptr+= sprintf(ptr, ", Status: No card\r\n");
			else
				ptr+= sprintf(ptr, ", Status: Error\r\n");*/
			else 
				continue;
			write(pipe, msg, strlen(msg));
		}

		usleep(io_interval);
		cycle++;
	};
    close(pipe);
}

void* execute_usb(void* ptr)
{
	while (true)
	{
		parse_live_data();
		//read_live_data();
	};
}

void InitFifo(char* name, bool tx, bool rx)
{
	struct stat st = {0};
   	char *path[50];

	if (!tx && !rx)
	{
		sprintf(path, "%s/%s", dir_rexgen, name);
    	if (stat(path, &st) == -1)
	    	mkfifo(path, 0755);
	}
	else
	{
		sprintf(path, "%s/%s/", dir_rexgen, name);
		if (stat(path, &st) == -1)
			mkdir(path, 0755);

		if (tx)
		{
			sprintf(path, "%s/%s/tx", dir_rexgen, name);
    		if (stat(path, &st) == -1)
		    	mkfifo(path, 0777);
		}
		if (rx)
		{
			sprintf(path, "%s/%s/rx", dir_rexgen, name);
    		if (stat(path, &st) == -1)
		    	mkfifo(path, 0755);
		}
	}
}

bool InitPipes()
{
	umask(0);
	struct stat st = {0};
	if (stat(dir_rexgen, &st) == -1)
		mkdir(dir_rexgen, 0755);

	InitFifo(sys, false, false);
	InitFifo(can0, true, true);
	InitFifo(can1, true, true);
	InitFifo(gyro, false, true);
	InitFifo(acc, false, true);
	InitFifo(dig, false, true);
	InitFifo(adc, false, true);

	// Usb IO
	pthread_create(&io, NULL, &execute_io, NULL);

	// Usb receiver
	queue.qidx1 = 0;
	queue.qidx2 = 0;
	pthread_create(&usb, NULL, &execute_usb, NULL);

	// Can0
	datasize_can0tx = 0;
	pthread_create(&can0tx, NULL, &execute_can0tx, NULL);
	datasize_can0rx = 0;
	pthread_create(&can0rx, NULL, &execute_can0rx, NULL);

	// Can1
	datasize_can1tx = 0;
	pthread_create(&can1tx, NULL, &execute_can1tx, NULL);
	datasize_can1rx = 0;
	pthread_create(&can1rx, NULL, &execute_can1rx, NULL);

	// Gyro
	//datasize_gyrotx = 0;
	//pthread_create(&gyrotx, NULL, &execute_gyrotx, NULL);
	datasize_gyrorx = 0;
	pthread_create(&gyrorx, NULL, &execute_gyrorx, NULL);

	// Acc
	//datasize_acctx = 0;
	//pthread_create(&acctx, NULL, &execute_acctx, NULL);
	datasize_accrx = 0;
	pthread_create(&accrx, NULL, &execute_accrx, NULL);

	// Digital
	//datasize_digtx = 0;
	//pthread_create(&digtx, NULL, &execute_digtx, NULL);
	datasize_digrx = 0;
	pthread_create(&digrx, NULL, &execute_digrx, NULL);

	// ADC
	//datasize_adctx = 0;
	//pthread_create(&adctx, NULL, &execute_adctx, NULL);
	datasize_adcrx = 0;
	pthread_create(&adcrx, NULL, &execute_adcrx, NULL);

	return true;
}

void DestroyPipes()
{
	// IO
	pthread_exit(&io);
	//pthread_mutex_destroy(&mutex_io);

	// Usb
	pthread_exit(&usb);
	pthread_mutex_destroy(&mutex_usb);

	// Can0
	pthread_exit(&can0tx);
	pthread_mutex_destroy(&mutex_can0_tx);
	pthread_exit(&can0rx);
	pthread_mutex_destroy(&mutex_can0_rx);

	// Can1
	pthread_exit(&can1tx);
	pthread_mutex_destroy(&mutex_can1_tx);
	pthread_exit(&can1rx);
	pthread_mutex_destroy(&mutex_can1_rx);
	
	// Gyro
	//pthread_exit(&gyrotx);
	//pthread_mutex_destroy(&mutex_gyro_tx);
	pthread_exit(&gyrorx);
	pthread_mutex_destroy(&mutex_gyro_rx);

	// Acc
	//pthread_exit(&acctx);
	//pthread_mutex_destroy(&mutex_acc_tx);
	pthread_exit(&accrx);
	pthread_mutex_destroy(&mutex_acc_rx);
}

