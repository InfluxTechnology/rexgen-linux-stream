// SPDX-License-Identifier: GPL-2.0

#include "pipes.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "../rexusb/commands.h"

char* add_msg_chn(char msg[], unsigned short uid)
{
    char *s[20];
    strcpy(s, "");

    if (uid == accXuid)
        strcat(s, " accX ");
    else if (uid == accYuid)
        strcat(s," accY ");
    else if (uid == accZuid)
        strcat(s," accZ ");
    else if (uid == gyroXuid)
        strcat(s," gyroX");
    else if (uid == gyroYuid)
        strcat(s," gyroY");
    else if (uid == gyroZuid)
        strcat(s," gyroZ");
    else if (uid == can0uid)
        strcat(s," can0 ");
    else if (uid == can1uid)
        strcat(s," can1 ");
    
    strcat(s, " ");
    strcat(msg, s);
    return msg;
}

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

    //printf("open pipe: %s\n", path);
	int pipe = open(path, O_WRONLY);

	if (pipe == -1)
	{
		printf("pipe id: %s failed to open\n", path);
		return NULL;
	}

   	char *msg[50];

   	unsigned long d_size;
   	unsigned char d_data[pipe_buffer_size];

    while (true)
    {
	    pthread_mutex_lock(mutex);
	    d_size = *datasize;
	    if (d_size > 0)
	    	memcpy(&(d_data[0]), &(*data), d_size);
	    *datasize = 0;
    	pthread_mutex_unlock(mutex);

	    if (d_size > 0)
	    {
			unsigned short uid = ((unsigned short*)(d_data + 0))[0];
    	    unsigned char infSize = d_data[2];
			unsigned char dlc = d_data[3];
    	    unsigned int timestamp = ((unsigned int*)(d_data + 4))[0];
    	    strcpy(msg, "");
    	    add_msg_val(msg, timestamp, 0);
    	    add_msg_chn(msg, uid);
			unsigned int canID = ((unsigned int*)(d_data + 8))[0];
			add_msg_val(msg, canID, 1);
    	    add_msg_val(msg, dlc, 3);
			unsigned char flags = d_data[12];	
			for (int i = 0; i < dlc; i++)
				add_msg_val(msg, d_data[i + 13], 2);

			strcat(msg, "\n");
			//printf("%s\n", msg);
			write(pipe, msg, strlen(msg));
    	}
    	//else
	 	   	//printf(s, "can%d: empty\n", channel);
    }

    close(pipe);
}

void* execute_can0rx(void* ptr)
{
	CanRx(can0, &mutex_can0_rx, &datasize_can0rx, &data_can0rx);

    return NULL;
}

void* execute_can1rx(void* ptr)
{
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

		buffidx = 0;

		if (!StrToLong(row[3], &tmp))
			return;

		unsigned char dlc = tmp;
		//if (buffidx + 4 + 9 + dlc > 512)
			//BufferToApp();

		if (dlc < 0 || dlc > 64 || 4 + dlc != count)
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
		if (!HexToLong(row[2], &ident))
			return;
		memcpy(&(buffer[buffidx + 4]), &ident, 4);
		
		buffer[buffidx + 8] = 0;
		
		buffidx+= 9;
		for (int i = 0; i < dlc; i++)
		{
			if (!HexToLong(row[4 + i], &tmp))
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

char* SensorUidToType(unsigned short uid)
{
	if (uid == gyroXuid || uid == accXuid)
		return "X";
	if (uid == gyroYuid || uid == accYuid)
		return "Y";
	if (uid == gyroZuid || uid == accZuid)
		return "Z";
	return "NA";
}

void SensorRx(char* name, pthread_mutex_t* mutex, unsigned long* datasize, unsigned char* data)
{
   	char *path[50];
	sprintf(path, "%s/%s/rx", dir_rexgen, name);

	int pipe = open(path, O_WRONLY);

	if (pipe == -1)
	{
		printf("pipe id: %s failed to open\n", path);
		return NULL;
	}

   	char *msg[50];

   	unsigned long d_size;
   	unsigned char d_data[pipe_buffer_size];

    while (true)
    {
	    pthread_mutex_lock(mutex);
	    d_size = *datasize;
	    if (d_size > 0)
	    	memcpy(&(d_data[0]), &(*data), d_size);
	    *datasize = 0;
    	pthread_mutex_unlock(mutex);

	    if (d_size > 0)
	    {
			unsigned short uid = ((unsigned short*)(d_data + 0))[0];
    	    unsigned char infSize = d_data[2];
			unsigned char dlc = d_data[3];
    	    unsigned int timestamp = ((unsigned int*)(d_data + 4))[0];
		   	float value = ((float*)(d_data + 4 + infSize))[0];

    	    sprintf(msg, "(%u) %s %f\n", timestamp, SensorUidToType(uid), value);

			write(pipe, msg, strlen(msg));
    	}
    }

    close(pipe);
}

void* execute_gyrorx(void* ptr)
{
	SensorRx(gyro, &mutex_gyro_rx, &datasize_gyrorx, &data_gyrorx);
    return NULL;
}

void* execute_accrx(void* ptr)
{
	SensorRx(acc, &mutex_acc_rx, &datasize_accrx, &data_accrx);
    return NULL;
}

/*void SensorTx(char* name, pthread_mutex_t* mutex, unsigned long* datasize, unsigned char* data)
{

}

void* execute_gyrotx(void* ptr)
{
	SensorTx(gyro, &mutex_gyro_tx, &datasize_gyrotx, &data_gyrotx);
    return NULL;
}

void* execute_acctx(void* ptr)
{
	SensorTx(acc, &mutex_acc_tx, &datasize_acctx, &data_acctx);
    return NULL;
}*/

void InitFifo(char* name, bool tx, bool rx)
{
	struct stat st = {0};
   	char *path[50];

	sprintf(path, "%s/%s/", dir_rexgen, name);
	if (stat(path, &st) == -1)
		mkdir(path, 0666);
	if (tx)
	{
		sprintf(path, "%s/%s/tx", dir_rexgen, name);
    	if (stat(path, &st) == -1)
	    	mkfifo(path, 0666);
	}
	if (rx)
	{
		sprintf(path, "%s/%s/rx", dir_rexgen, name);
    	if (stat(path, &st) == -1)
	    	mkfifo(path, 0666);
	}
}

bool InitPipes()
{
	struct stat st = {0};
	if (!stat(dir_rexgen, &st))
		mkdir(dir_rexgen, 0777);

	InitFifo(can0, true, true);
	InitFifo(can1, true, true);
	InitFifo(gyro, false, true);
	InitFifo(acc, false, true);

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

	return true;
}

void DestroyPipes()
{
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

