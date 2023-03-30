#include <stdio.h>
#include <memory.h>
#include "commands.h"
#include "communication.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

unsigned char Seq;
// progress vars
unsigned int PrgMax;
char PrgStr[] = "";
unsigned int PrgPos;
unsigned short PrgCnt = 10;
// Green LED blink vars
bool GreenLEDStatus;
unsigned int Time_Interval;
struct timeval Time_Before, Time_After;
int CANDebugMode, RebootAfterReflash;


void ProgressPos(unsigned int prgPos)
{
	unsigned short proc = prgPos * 100 / PrgMax;
	if (PrgPos == proc) return;

	PrgPos = proc;
	printf("\r%s %d %c ", PrgStr, proc, 37);
	fflush(stdout);
}

void GreenLED_time(double greenLEDtime)
{
	if (greenLEDtime == 0)
	{
		gettimeofday(&Time_Before, NULL);
		Time_Interval = 0;
		GreenLEDStatus = false;
		GreenLED_status(GreenLEDStatus);
		return;
	}

	struct timeval time_Result;
	unsigned long timeLED = greenLEDtime * 1000;// must be in micro seconds

	gettimeofday(&Time_After, NULL);
	timersub(&Time_After, &Time_Before, &time_Result);
	Time_Interval += time_Result.tv_usec;

	if (Time_Interval > timeLED)
	{
		gettimeofday(&Time_Before, NULL);
		Time_Interval = 0;
		GreenLEDStatus = !GreenLEDStatus;
		GreenLED_status(GreenLEDStatus);
	}
}

void GreenLED_status(bool greenLEDstatus)
{

	if (access("/home/root/rexusb/green_led.sh", F_OK) == 0)
	{
		if (greenLEDstatus)
			system("/home/root/rexusb/green_led.sh 1");
		else
			system("/home/root/rexusb/green_led.sh 0");
	}

}

void PrepareGreenLED(bool create)
{
return;		// not exists green LED in QP-build (exists only in Aplha boaurd)    
	char pwd[256], glf[256];
	if (getcwd(pwd, sizeof(pwd)) != NULL)
	{
//    	    strcat(glf, "/home/root/rexusb/green_led.sh");
		strcpy(glf, pwd);
		strcat(glf, "/green_led.sh");
	}

//	strcat(glf, "/home/root/rexusb/green_led.sh");
	if (access(glf, F_OK) == 0)
		remove(glf);

	if (!create)
		return;

	// create temp file to helping set/reset green LED
	FILE* fp;
	fp = fopen(glf, "w");
	fprintf(fp, "#!/bin/sh\n\n");
	fprintf(fp, "# set/reset GREEN led\n");
	fprintf(fp, "if [ $1 == %c0%c ]; then\n", 34, 34);
	fprintf(fp, "    echo 0 > /sys/class/gpio/gpio140/value\n");
	fprintf(fp, "else\n");
	fprintf(fp, "    echo 1 > /sys/class/gpio/gpio140/value\n");
	fprintf(fp, "fi\n");
	fclose(fp);
	system("chmod +x /home/root/rexusb/green_led.sh");
}

void BuildCheckSum(unsigned char* data, unsigned short count)
{
	data[count-1] = 0;
	for (int i = 0; i < count-1; i++)
		data[count-1] += data[i];
}

void BuildCmmd(EndpointCommunicationStruct *cmdobj, const cmmdStruct *cmd)
{
//for (int i = 0; i < cmd->tx_len; i++) printf("%i ", cmd->cmd_data[i]);
	cmdobj->tx_data[0] = Seq++;
	unsigned short len = cmd->tx_len + 4;
	memcpy(&cmdobj->tx_data[1], &len, sizeof(len));
	memcpy(&cmdobj->tx_data[3], cmd->cmd_data, cmd->tx_len);
	BuildCheckSum((unsigned char*)cmdobj->tx_data, len);
	cmdobj->tx_len = len;
	cmdobj->rx_len = cmd->rx_len;
//printf("%i \n", cmdobj->tx_len); // 
//for (int i = 0; i < len; i++) printf("%i ", cmdobj->tx_data[i]); // 
//for (int i = 0; i < cmd->rx_len; i++) printf("%i ", cmdobj->rx_data[i]); // 
}

void BuildCustomCmmd(EndpointCommunicationStruct *cmdobj, unsigned char tx_len, unsigned char rx_len, unsigned char *cmd)
{
//printf("%i %i \n", tx_len, rx_len); // 
	cmdobj->tx_data[0] = Seq++;
	unsigned short len = tx_len + 4;
	memcpy(&cmdobj->tx_data[1], &len, sizeof(len));
	memcpy(&cmdobj->tx_data[3], cmd, tx_len);
	BuildCheckSum((unsigned char*)cmdobj->tx_data, len);
	cmdobj->tx_len = len;
	cmdobj->rx_len = rx_len;
//for (int i = 0; i < len; i++) printf("%i ", cmdobj->tx_data[i]); // 
}

void PrintTxCmmd(EndpointCommunicationStruct *ep)
{
	// Debug output	
	if (CANDebugMode == 1)
	{		
		printf("Transmitted: ");
		for(int j = 0; j < ep->tx_len; j++)
			printf("%.2X ", ep->tx_data[j]);
		printf("\n");
	}	
}

void PrintRxCmmd(EndpointCommunicationStruct *ep)
{
	// Debug output
	if (CANDebugMode == 1)
	{
		printf("Received   : ");
//		printf("%.2X", ep->rx_len);
//		printf("\n");
		for(int j = 0; j < ep->rx_len; j++)
			printf("%.2X ", ep->rx_data[j]);
		printf("\n");
	}
	else
	{
		for(int j = 0; j < ep->rx_len; j++)
			printf("%.2X ", ep->rx_data[j]);		
		printf("\n");
	}
}

void PrintChannelLabel(unsigned short uid)
{
	
	if (uid == accXuid)
		printf(" accX ");
	else if (uid == accYuid)
		printf(" accY ");
	else if (uid == accZuid)
		printf(" accZ ");
	else if (uid == gyroXuid)
		printf(" gyroX");
	else if (uid == gyroYuid)
		printf(" gyroY");
	else if (uid == gyroZuid)
		printf(" gyroZ");
	else if (uid == can0uid)
		printf(" can0 ");
	else if (uid == can1uid)
		printf(" can1 ");
}

unsigned char ChannelSupported(unsigned short uid)
{
	if (uid == accXuid)
		return 0;
	else if (uid == accYuid)
		return 0;
	else if (uid == accZuid)
		return 0;
	else if (uid == gyroXuid)
		return 0;
	else if (uid == gyroYuid)
		return 0;
	else if (uid == gyroZuid)
		return 0;
	else if (uid == can0uid)
		return 0;
	else if (uid == can1uid)
		return 0;
	else
		return 1;
}

void PrintLiveData(EndpointCommunicationStruct *ep)
{
	if (ep->rx_len < 2)
		return;
	
	unsigned short blockSize = 0;
	unsigned short pos = 2;
	blockSize =  ((unsigned short*)ep->rx_data)[0];
	/*printf("Block Size is: %d\n", blockSize);
	printf("rxlen is: %d\n", ep->rx_len);
	for(int j = 0; j < ep->rx_len; j++)
			printf("%.2X ", ep->rx_data[j]); //DEBUG	
		printf("\n");*/
	while (pos < blockSize)	
	{
		unsigned short uid = ((unsigned short*)(ep->rx_data + pos))[0];
		unsigned char infSize = ep->rx_data[pos + 2];
		//printf("infSize is: %d\n", infSize);
		unsigned char dlc = ep->rx_data[pos + 3];
		unsigned int timestamp = ((unsigned int*)(ep->rx_data + pos + 4))[0];
		//printf("uid %d ", uid);
		//printf("supported %d\n", ChannelSupported(uid));
		if (ChannelSupported(uid) == 0)  // All uid that can be displayed
		{
			printf("%u", timestamp);
			PrintChannelLabel(uid);
			printf(" [%d]", dlc);		
			if (uid == accXuid || uid == accYuid || uid == accZuid || uid == gyroXuid || uid == gyroYuid || uid == gyroZuid)  // Accelerometer and Gyro
			{
				for (int i=0; i<dlc; i++)
				{
					printf(" %.2X", ep->rx_data[i + pos + 8]);
				}
				pos += 8 + dlc;	
			}
			else if (uid == can0uid || uid == can1uid)  // CAN Log all
			{
				unsigned int canID = ((unsigned int*)(ep->rx_data + pos + 8))[0];
				printf(" 0x%.2X", canID);
				unsigned char flags = ep->rx_data[pos + 12];	
				for (int i=0; i<dlc; i++)
				{
					printf(" %.2X", ep->rx_data[i + pos + 13]);
				}
				pos += 13 + dlc;	
			}
				
			printf("\n");
		}
		else
			pos += infSize + dlc + 4;   //skip bytes from unknown frames
			
	}
}

unsigned char SendCommand(DeviceStruct *devobj, unsigned char endpoint_id, const cmmdStruct *cmmd)
{
	if (CANDebugMode == 1)
		HideRequest = false;

	EndpointCommunicationStruct *ep = &devobj->ep[endpoint_id];
	BuildCmmd(ep, cmmd);
	PrintTxCmmd(ep);
//for (int i = 0; i < ep->tx_len; i++) {printf("%i ",  ep->tx_data[i]); fflush(stdout);}
//printf("\n %i \n",  ep->tx_len); fflush(stdout);

	UsbSend(devobj, endpoint_id);
	sleep(0.1);

	unsigned char res = UsbRead(devobj, endpoint_id);

//for (int i = 0; i < ep->rx_len; i++) {printf("%i ",  ep->rx_data[i]); fflush(stdout);}
//printf("\n %i \n",  ep->rx_len); fflush(stdout);
//printf("%i\n", HideRequest);
	if (!HideRequest)
		PrintRxCmmd(ep);
	HideRequest = true;
	return res;
}

unsigned char SendCustomCommand(DeviceStruct *devobj, unsigned char endpoint_id, unsigned char tx_len, unsigned char rx_len, unsigned char *cmd)
{
	EndpointCommunicationStruct* ep = &devobj->ep[endpoint_id];
	BuildCustomCmmd(ep, tx_len, rx_len, cmd);

	if (CANDebugMode == 1)
		PrintTxCmmd(ep);

	UsbSend(devobj, endpoint_id); 
	sleep(0.1);
 
	if (UsbRead(devobj, endpoint_id) == 0)
	{ 
		if (CANDebugMode == 1)
			PrintRxCmmd(ep);
		return 0;
	}
	else 
		return 6;
}

/*void BuildGNSSDataCmmd(COMMUNICATION_STRUCT *cmdobj, unsigned char tx_len, unsigned char rx_len, unsigned char *cmd)
{
	memcpy(&cmdobj->tx_data[0], cmd, tx_len);
///
	cmdobj->tx_len = tx_len;
	cmdobj->rx_len = rx_len;
}*/

void SendGNSSData(DeviceStruct *devobj, structGNSSData *GNSSData)
{
    /*unsigned char cmd_data[21];// = {0x1E, 0x00};	
	memcpy(&cmd_data[0], GNSSData, sizeof(structGNSSData));
	devobj->endpoint = 3;
	BuildGNSSDataCmmd(&devobj->msg, sizeof(structGNSSData), 1, (unsigned char*)&cmd_data);

	if (CANDebugMode == 1)
		PrintTxCmmd(devobj);
//printf("%i \n", devobj->msg.tx_data[0]);
	UsbSend(devobj);*/

	SendCustomCommand(devobj, epLive, sizeof(structGNSSData), 1, (unsigned char*)&GNSSData);
	if (CANDebugMode == 1)
		PrintTxCmmd(devobj);
}

void SendStartLiveData(DeviceStruct *devobj, unsigned char channel)
{
	EndpointCommunicationStruct* ep = &devobj->ep[epData];
	unsigned char cmd_data[3] = {0x19, 0x00, channel};

	BuildCustomCmmd(ep, 3, 7, (unsigned char*)&cmd_data);
	if (CANDebugMode == 1)
		PrintTxCmmd(ep);

	UsbSend(devobj, epData); 
	UsbRead(devobj, epData);
	if (CANDebugMode == 1)
		PrintRxCmmd(ep);
//	devobj->endpoint = 2;
}

/*void ReadLiveData(DeviceStruct *devobj, unsigned char channel)
{
	devobj->msg.rx_len = 512;
	UsbRead(devobj, epLive);

	//PrintRxCmmd(devobj);
	PrintLiveData(devobj);
}*/

void SendInitBus(DeviceStruct *devobj, initStruct *cmmdInit)
{
	EndpointCommunicationStruct *ep = &devobj->ep[epData];
	unsigned char cmd_data[18] = {0x1D, 0x00};	
	/*printf("initStructSize: ");
	printf("%i", sizeof(initStruct));
	printf("\n");*/
	memcpy(&cmd_data[2], cmmdInit, sizeof(initStruct));

	if (CANDebugMode == 1)
	{
		printf("CMDData: ");
		for(int j = 0; j < 18; j++)
			printf("%.2X ", cmd_data[j]);
		printf("\n");
	}	

	SendCustomCommand(devobj, epData, 18, 6, (unsigned char*)&cmd_data);
	if (ep->rx_data[1] == 6)
	{
		if (cmmdInit->speed_fd == 0)
			printf("CAN Initialized\n");
		else
			printf("CANFD Initialized\n");
	}
}

void RequestCanRX(DeviceStruct *devobj, cmdRequestRX *cmdRX)
{
	EndpointCommunicationStruct* ep = &devobj->ep[epData];
	unsigned char cmd_data[81] = {0x1F, 0x00};	

	memcpy(&cmd_data[2], cmdRX, sizeof(cmdRequestRX));


	if (CANDebugMode == 1)
	{
		printf("CMDData: ");
		for(int j = 0; j < 7; j++)
			printf("%.2X ", cmd_data[j]);
		printf("\n");
	}

	int received = 0;
	for (int i = 0; i < 2000; ++i)
	{
		//sleep(0.005);
		SendCustomCommand(devobj, epData, 7, 8 + sizeof(CAN_TX_RX_STRUCT), (unsigned char*)&cmd_data);

		/*for(int j = 0; j < 7; j++)
			printf("%.2X ", cmd_data[j]);
		printf("\n");*/

		if (ep->rx_data[5] != 0 || ep->rx_data[6] != 0)
		{
			received = 1;

			//printf("breaked");
			//printf("%d", i);
			break;
		}		
	}
	
	if (received != 1)
		return;
	ResponseStruct rx_data;
	if (ep->rx_data[3] == 0x1F)
	{		
		memcpy(&rx_data, ep->rx_data[7], sizeof(ResponseStruct));
		printf("can%i ", rx_data.Bus);
		printf("0x%.2X ", rx_data.ID);
		printf("[%i] ", rx_data.DLC);
		for(int j = 0; j < rx_data.DLC; j++)
			printf("%.2X ", rx_data.Data[j]);
		printf("\n");
	}
	else
		printf("No response received!");
}

void SendCANCmd(DeviceStruct *devobj, cmdCANstruct *cmdCAN)
{
	EndpointCommunicationStruct* ep = &devobj->ep[epData];
	unsigned char cmd_data[77] = {0x1E, 0x00};	

	if (CANDebugMode == 1)
	{
		printf("cmdCANstruct: ");
		printf("%i", sizeof(cmdCANstruct));
		printf("\n");
	}
	memcpy(&cmd_data[2], cmdCAN, sizeof(cmdCANstruct));


	printf("can%i ", cmdCAN->Bus);
	printf("0x%.2X ", cmdCAN->ID);
	printf("[%i] ", cmdCAN->DLC);
	for(int j = 0; j < cmdCAN->DLC; j++)
		printf("%.2X ", cmdCAN->Data[j]);
	printf("\n");
	//devobj->msg.Timeout = 1000;
	SendCustomCommand(devobj, epData, 13 + cmdCAN->DLC, 11, (unsigned char*)&cmd_data);
	cmdRequestRX canRX = {
		Bus: cmdCAN->Bus,
		Timeout: 2000,
	};
	
	RequestCanRX(devobj, &canRX);
}


void ReadCANMsg(DeviceStruct *devobj, unsigned char bus, unsigned long msgCount)
{
	cmdRequestRX canRX = {
		Bus: bus,
		Timeout: 2000,
	};
	
	
	if (msgCount > 0)
	{
		for (int i = 0; i < msgCount; ++i)
			{
				RequestCanRX(devobj, &canRX);
			}
	}
	else if (msgCount == 0)
		while (1 > 0)
			{
				RequestCanRX(devobj, &canRX);
			}
}

unsigned char GetConfig(DeviceStruct *devobj)   //read configuration and get the uid for all supported channels
{
	EndpointCommunicationStruct *ep = &devobj->ep[epData];
	unsigned char cmd_data[200] = {0x1C, 0x00};
	unsigned int configSize, currentPos;
	unsigned short bufferSize = 200;
	unsigned char* pBinCfg;
	currentPos = 0;
	if (SendCommand(devobj, epData, &cmmdGetConfigSize) == 0)
	{
		configSize = ((unsigned int*)(ep->rx_data + 21))[0]; 
		pBinCfg = (unsigned char*)calloc(configSize, 1);
		//printf("\npBinCfg:%d\n", pBinCfg);
		printf("\nConfig Size:%d\n", configSize);
		if (configSize < bufferSize)
			bufferSize = configSize;

		while (bufferSize > 0)
		{
			memcpy(&cmd_data[2], &bufferSize, sizeof(bufferSize));
			//Get the configuration from RexGen
			if (SendCustomCommand(devobj, epData, 4, bufferSize + 6, (unsigned char*)&cmd_data) == 0)  
			{
				for (int i = 0; i < bufferSize; ++i)
				{
					*(pBinCfg + i + currentPos) = ep->rx_data[5 + i];
				}
				//memcpy(pBinCfg, &devobj->msg.tx_data[5], bufferSize);
				currentPos += bufferSize;
				if ((currentPos + bufferSize) > configSize)
					bufferSize = (configSize - currentPos);
			}
		}
		currentPos = 0;
		unsigned short blockType;
		unsigned short blockSize;
		unsigned short uid;
		unsigned short canInt0Uid = 0;
		unsigned short canInt1Uid = 0;
		unsigned short canInt2Uid = 0;
		unsigned short canInt3Uid = 0;
		unsigned char numb;
		while (currentPos < configSize)  //Get the uid of the CAN Interfaces
		{
			blockType = ((unsigned short*)(pBinCfg + currentPos))[0];
			blockSize = ((unsigned short*)(pBinCfg + currentPos + 4))[0];
			uid = ((unsigned short*)(pBinCfg + currentPos + 6))[0];
			if (blockType == 2) //CAN Interface
			{
				unsigned char canphys = ((unsigned char*)(pBinCfg + currentPos + 9))[0];
				if (canphys == 0)
					canInt0Uid = uid;
				else if (canphys == 1)
					canInt1Uid = uid;
				else if (canphys == 2)
					canInt2Uid = uid;
				else if (canphys == 3)
					canInt3Uid = uid;
			}
			currentPos += blockSize;
		}
		currentPos = 0;
		while (currentPos < configSize)
		{
			blockType = ((unsigned short*)(pBinCfg + currentPos))[0];
			blockSize = ((unsigned short*)(pBinCfg + currentPos + 4))[0];
			uid = ((unsigned short*)(pBinCfg + currentPos + 6))[0];
			//printf("BlockType: %d\n", blockType);
			//printf("BlockSize: %d\n", blockSize);
			//printf("uid: %d\n", uid);
			unsigned char interfaceID;
			unsigned char direction;
			switch (blockType)
			{
				case 3: //CAN Message
					//unsigned int identEnd = ((unsigned int*)(pBinCfg + currentPos + 12))[0];
					interfaceID = ((unsigned char*)(pBinCfg + currentPos + 19))[0];
					direction = ((unsigned char*)(pBinCfg + currentPos + 16))[0];
					if (direction == 0)
					{
    					if (interfaceID == canInt0Uid)
							can0uid = uid;
						else if (interfaceID == canInt1Uid)
							can1uid = uid;
						else if (interfaceID == canInt2Uid)
							can2uid = uid;
						else if (interfaceID == canInt3Uid)
							can3uid = uid;
					}
					break;
				case 10: // ADC
					numb = ((unsigned char*)(pBinCfg + currentPos + 8))[0];
					if (numb == 0)
						adc0uid = uid;
					else if (numb == 1)
						adc1uid = uid;
					else if (numb == 2)
						adc2uid = uid;
					else if (numb == 3)
						adc3uid = uid;
					//printf("id: %d\n", numb);
					break;
				case 21: // Digital input
					numb = ((unsigned char*)(pBinCfg + currentPos + 8))[0];
					if (numb == 0)
						dig0uid = uid;
					else if (numb == 1)
						dig1uid = uid;
					//printf("id: %d\n", numb);
					break;
				case 24: // Accelerometer
					numb = ((unsigned char*)(pBinCfg + currentPos + 9))[0];
					if (numb == 0)
						accXuid = uid;
					else if (numb == 1)
						accYuid = uid;
					else if (numb == 2)
						accZuid = uid;
					//printf("axis: %d\n", numb);
					break;
				case 25: // Gyroscope
					numb = ((unsigned char*)(pBinCfg + currentPos + 9))[0];
					if (numb == 0)
						gyroXuid = uid;
					else if (numb == 1)
						gyroYuid = uid;
					else if (numb == 2)
						gyroZuid = uid;
					//printf("axis: %d\n", numb);
					break;
				case 27: // GNSS
					numb = ((unsigned char*)(pBinCfg + currentPos + 10))[0];
					if (numb == 0)
						gnss0uid = uid;
					else if (numb == 1)
						gnss1uid = uid;
					else if (numb == 2)
						gnss2uid = uid;
					else if (numb == 3)
						gnss3uid = uid;
					else if (numb == 4)
						gnss4uid = uid;
					else if (numb == 5)
						gnss5uid = uid;
					else if (numb == 6)
						gnss6uid = uid;
					else if (numb == 7)
						gnss7uid = uid;
					else if (numb == 8)
						gnss8uid = uid;
					else if (numb == 9)
						gnss9uid = uid;
					else if (numb == 10)
						gnss10uid = uid;
					else if (numb == 11)
						gnss11uid = uid;
					else if (numb == 12)
						gnss12uid = uid;
					//printf("axis: %d\n", numb);
					break;
			}
			currentPos += blockSize;
		}
		/*for (int i=0; i<configSize; i++)
		{
			printf("%.2X ", *(pBinCfg + i));
		}*/
		return 0;		
	}
	return 1;
		
}



int ReadFile()
{

    FILE *fp;
    unsigned  char* pBinCfg;
    unsigned int fileSize;
    fp = fopen("live.bin", "r");
    fseek(fp, 0L, SEEK_END);
	fileSize = ftell(fp);
	rewind(fp);
	pBinCfg = (unsigned char*)calloc(fileSize, 1);
    fread(pBinCfg, fileSize, 1, fp);
    for (int i = 0; i < fileSize; ++i)
    {
    	printf("%.2X ", ((unsigned char*)(pBinCfg))[i] );
    }
    printf("\n");
    
    
    fclose(fp);
    return 0;
}



void ApplyReflash(DeviceStruct* devobj)
{
	char host[256];
	GetHostName(host);
//	bool isNano = false;
//	if (strcmp(host, "imx8mnea-ucom") == 0 || strcmp(host, "imx8mmea-ucom") == 0) 
//	    isNano = true;
	bool isNano = strcmp(host, "imx8mnea-ucom") == 0 || strcmp(host, "imx8mmea-ucom") == 0;

	if (isNano)
		PrepareGreenLED(true);

	time_t timer1, timer2;
	double seconds;
	unsigned short c = 0;
	unsigned short secondsCount = 20;
	char chars[] = { '-', '\\', '|', '/' };

	time(&timer1);
	time(&timer2);
	seconds = difftime(timer2, timer1);

	GreenLED_time(0);
	if (isNano)
		secondsCount = 15;

	while (seconds < secondsCount)
	{
		GreenLED_time(300);

		time(&timer2);
		seconds = difftime(timer2, timer1);
		//	InitUsbLibrary();
		printf("\rApply reflashing ... %c ", chars[c % sizeof(chars)]);
		fflush(stdout);

		c++;
		if (c > 3) c = 0;
	}
	printf("\rReflashing done.          \n");

	GreenLED_status(true);
	if (isNano)
	{
		PrepareGreenLED(false);

		// rebooting
		if ( RebootAfterReflash == 1)
		{
		        printf("\nRebooting ...\n");
			system("reboot");
		}
	}
}

char* GetHostName(char res[])
{
	FILE *cmd = popen("uname -n", "r");
        char buff[256];
	while (fgets(buff, sizeof(buff), cmd) != NULL) { }
	strcpy(res, buff);
        pclose(cmd);
	int size = strlen(res);
	res[size - 1] = '\0';
	return res;
}

int UsbSendRead(DeviceStruct* devobj, unsigned char endpoint_id, bool flag)
{
//		unsigned short len, sendCRC, readCRC;
//		len = devobj->msg.tx_len;
//		sendCRC = devobj->msg.tx_data[len - 1];
	if (UsbSend(devobj, endpoint_id) != 0)
		return 1;
	sleep(0.1);
	if (! flag)
		if (UsbRead(devobj, endpoint_id) != 0)
 			return 1;
	return 0;
}

void Reflash(DeviceStruct* devobj, char* filename)
{
	EndpointCommunicationStruct* ep = &devobj->ep[epData];

	bool flag = false;
	
	

	if (access(filename, F_OK) != 0)
	{
		printf("File '%s' not exsists. \n", filename);
		return;
	}

	FILE* fp;
	unsigned char ReflashBuffer[0x80000];
	unsigned short FW_CRC;
	unsigned short CMD_send;
	unsigned long ReturnBytes;
	unsigned int StartAddress, MemorySize, MaxAddress, bi;
	unsigned int memTransferSize, memMAXTransferSize;
	unsigned char* msg = &(ep->tx_data[0]);
	unsigned long len, cnt = 0;
	libusb_device_handle* tmphandle;

	memMAXTransferSize = 490;//42;
	MaxAddress = 0x80000;
	memset((void*)ReflashBuffer, 0, sizeof(ReflashBuffer));

	fp = fopen(filename, "rb");
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	//*ReflashBuffer = malloc(len); // ne znam kakvo pravi. moje i bez nego

	if (fp == NULL || len == 0) // is file empty
	{
		printf("File '%s' is empty. \n", filename);
		return;
	}

	unsigned long readlen = fread(&ReflashBuffer, 1, len, fp);
	if (readlen != len)
	{ 	// file read error
		printf("There was an error reading the file '%s' \n", filename);
		return;
	}

	FW_CRC = 0;
	ReturnBytes = 0;
	StartAddress = 0;
	MemorySize = MaxAddress;

	CMD_send = 7;
	len = 14;
	ep->tx_len = len;
	ep->rx_len = 8;
	msg[0] = Seq++;
	msg[1] = len; 		msg[2] = len >> 8; 		msg[3] = CMD_send; 		msg[4] = CMD_send >> 8;
	msg[5] = StartAddress;	msg[6] = StartAddress >> 8;	msg[7] = StartAddress >> 16;	msg[8] = StartAddress >> 24;
	msg[9] = MemorySize;	msg[10] = MemorySize >> 8;	msg[11] = MemorySize >> 16;	msg[12] = MemorySize >> 24;
	BuildCheckSum((unsigned char*)msg, len);

	//CANDebugMode = 1;
	tmphandle = devobj->handle;
	if (UsbSendRead(devobj, epData, flag) != 0) return;

	memTransferSize = memMAXTransferSize;
	StartAddress = 0;

	PrgMax = MaxAddress;
	PrgPos = 0;
	char prgStr[] = "File transfering ...";
	strcpy(PrgStr, prgStr);

	while (StartAddress <= (MaxAddress - 1))
	{
		devobj->handle = tmphandle; // patch - some how handle is changed and must be safe.

		CMD_send = 8;
		len = 12 + memTransferSize;

		ep->tx_len = len;
		ep->rx_len = 8;
		msg[0] = Seq++;
		msg[1] = len; 		msg[2] = len >> 8; 		msg[3] = CMD_send; 		msg[4] = CMD_send >> 8;
		msg[5] = StartAddress;	msg[6] = StartAddress >> 8;	msg[7] = StartAddress >> 16;	msg[8] = StartAddress >> 24;
		msg[9] = memTransferSize;	 msg[10] = memTransferSize >> 8;
		memcpy((void*)&msg[11], (void*)&ReflashBuffer[StartAddress], memTransferSize);
		BuildCheckSum((unsigned char*)msg, len);

		if (UsbSendRead(devobj, epData, flag) != 0)	return;

		StartAddress += memTransferSize;
		if ((StartAddress + memMAXTransferSize) < (MaxAddress - 1))
			memTransferSize = memMAXTransferSize;
		else
			memTransferSize = MaxAddress - StartAddress;

		ProgressPos(StartAddress);
	}

	FW_CRC = 0;
	for (int i = 0; i < MaxAddress; i++) FW_CRC += ReflashBuffer[i];
	StartAddress = 0; MemorySize = MaxAddress; //

	CMD_send = 9;
	len = 8;
	ep->tx_len = len;
	ep->rx_len = 8;
	msg[0] = Seq++;
	msg[1] = len; 		msg[2] = len >> 8; 	msg[3] = CMD_send; 	msg[4] = CMD_send >> 8;
	msg[5] = FW_CRC;	msg[6] = FW_CRC >> 8;
	BuildCheckSum((unsigned char*)msg, len);

	devobj->handle = tmphandle; // patch - some how handle is changed and must be safe.
	flag = true;	// da ne probva UsbRead
	if (UsbSendRead(devobj, epData, flag) != 0) return;

	printf("\rFile transfer complete.        \n");
	CANDebugMode = 0;
}

unsigned char SendConfig(DeviceStruct* devobj, char* filename)
{
	EndpointCommunicationStruct* ep = &devobj->ep[epData];

	if (access(filename, F_OK) != 0)
	{
		printf("File '%s' not exsists. \n", filename);
		return 0;
	}

	unsigned char ConfigBuffer[0x80000];
	bool bRes;
 	FILE* fp;
	
	unsigned short FW_CRC;
	unsigned short CMD_send;
	unsigned long ReturnBytes;
	unsigned int StartAddress, MemorySize, MaxAddress, bi;
	unsigned int memTransferSize, memMAXTransferSize;
	
	unsigned char* msg = &(ep->tx_data[0]);
	unsigned long len = ep->tx_len;
	memMAXTransferSize = 490;
	MaxAddress = 0x80000;// 0x80000;
	memset((void*)ConfigBuffer, 0, sizeof(ConfigBuffer));
	fp = fopen(filename, "rb");
	
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	if (fp == NULL || len == 0) // is file empty
	{
		printf("File '%s' is empty. \n", filename);
		return 0;
	}	
	MaxAddress = len;
	unsigned long readlen = fread(&ConfigBuffer, 1, len, fp);
	fclose(fp);

	FW_CRC = 0;
	// USB_CMD_CONFIG_START

	ReturnBytes = 0;
	CMD_send = 11;	
	len = 26;  
	msg[0] = Seq++; 
	msg[1] = len; 
	msg[2] = len >> 8; 
	msg[3] = CMD_send; 
	msg[4] = CMD_send >> 8;
	MemorySize = MaxAddress;
	msg[5] = MemorySize;	
	msg[6] = MemorySize >> 8;	
	msg[7] = MemorySize >> 16;	
	msg[8] = MemorySize >> 24;
 	ep->tx_len = len;
	ep->rx_len = 8;
	
//	msg[9] = 1;	msg[10] = 2;	msg[11] = 3;	msg[12] = 4;
	memcpy((void*)&msg[9], (void*)&ConfigBuffer[6], 16);//GUID in file
	BuildCheckSum((unsigned char*)msg, len);
    PrintTxCmmd(devobj);
	bRes = UsbSend(devobj, epData);
	if (bRes != 0)		
		return 0;
	////////////////	
	msg = &(ep->rx_data[0]);
	len = 8;
	bRes = UsbRead(devobj, epData);
	if (bRes != 0)	 
		return 0;
	//PrintRxCmmd(devobj);
	msg = &(ep->tx_data[0]);
	StartAddress = 0;

	if ((StartAddress + memMAXTransferSize) < (MaxAddress - 1))
		memTransferSize = memMAXTransferSize;
	else
		memTransferSize = (MaxAddress)-StartAddress;

	while (StartAddress <= (MaxAddress - 1))
	{
		len = 12 + memTransferSize;
		CMD_send = 12; 
		msg[0] = Seq++; 
		msg[1] = len; 
		msg[2] = len >> 8; 
		msg[3] = CMD_send; 
		msg[4] = CMD_send >> 8;
		msg[5] = StartAddress;	
		msg[6] = StartAddress >> 8;	
		msg[7] = StartAddress >> 16;	
		msg[8] = StartAddress >> 24;
		msg[9] = memTransferSize;	 
		msg[10] = memTransferSize >> 8;
		ep->tx_len = len;
		ep->rx_len = 8;
		memcpy((void*)&msg[11], (void*)&ConfigBuffer[StartAddress], memTransferSize);
		BuildCheckSum((unsigned char*)msg, len);
		//PrintTxCmmd(devobj);
		bRes = UsbSend(devobj, epData);
		if (bRes != 0)		
			return 0;
		bRes = UsbRead(devobj, epData);
		if (bRes != 0)	 
			return 0;
		//PrintRxCmmd(devobj);
		StartAddress += memTransferSize;
		if ((StartAddress + memMAXTransferSize) < (MaxAddress - 1))
			memTransferSize = memMAXTransferSize;
		else
			memTransferSize = (MaxAddress)-StartAddress;
	}

	for (bi = 0; bi < MaxAddress; bi++) 
		FW_CRC += ConfigBuffer[bi];
	len = 8;
	ep->tx_len = len;
	ep->rx_len = 8;
	CMD_send = 13;	len = 8;  msg[0] = Seq++; msg[1] = len; msg[2] = len >> 8; msg[3] = CMD_send; msg[4] = CMD_send >> 8;
	StartAddress = 0; MemorySize = MaxAddress;
	msg[5] = FW_CRC;	msg[6] = FW_CRC >> 8;
	BuildCheckSum((unsigned char*)msg, len);

	bRes = UsbSend(devobj, epData);
	if (bRes != 0)		
		return 0;

	len = 8;
	bRes = UsbRead(devobj, epData);
	bRes = ep->rx_data[5];
    //printf("%d\n", bRes);
 	if (bRes == 0)
 		printf("Configuration Uploaded Successfully\n");
 	else if (bRes == 1)
 		printf("Reconfigure Failed\n");
 	else if (bRes == 2)
 		printf("Configuration Error\n");
	return bRes;
}