#include <stdio.h>
#include "communication.h"
#include "commands.h"
#include "common.h"
#include "string.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

// 1.2	Help for commands and parameters usage
//	Firmware version command
//	Implemented firmware reflash command
// 1.6	Read Accelerometer and Gyroscope live data
//	Send configuration created by RexDesk tool to the logger
// 1.9	Option to check NFC hardware
// 1.10	Option to shutdown the device
// 1.11	Added commands to set/get system date and time 
// 1.13	Show rexgen-stream version
// 1.14 Fixed get configuration command (for diagnostic of hardware configuration)
//	fixed get serial number command
// 1.15	Added command to get current structure name 
// 1.16	Added functions to get processor type from rexgen device and .bin file 
// 1.17	Support SD card functions
//	Format, general info		
//	Log info by index and log count
//	Download all rxd files and by index.
//	User configured rxd download folder


DeviceStruct DeviceObj;
bool SDCommandFlag;

void sigintHandler(int sig_num)
{
    exit(1);
}

int getParam(int argc, char *argv[], char arg[])
{
    for (int i = 1; i < argc; ++i)
    {
	if (strcmp(argv[i], arg) == 0)
	{
	    return i;
	}
    }
}

int strpos(char *haystack, char *needle)
{
    char *p = strstr(haystack, needle);
    if (p)
	return p - haystack;
    return NOT_FOUND;
}

int usageMsg(char *msg)
{
	printf("USAGE: \n");
	printf("	%s\n\n", msg);
	return 1;	
}

void printdatef(char *name, int idx)
{
	char buff[50];
	StructDateToStr(&DeviceObj, idx, "%Y-%m-%d %H:%M:%S", buff);

	printf("\t\t%s\t%s\n", name, buff);
}

int checkDateArg(char *arg[])
{
	if ((arg == NULL) || ((strcmp(arg, "set") != 0) && (strcmp(arg, "get") != 0)))
		return	usageMsg("rexgen date set|get");		
	return 0;
}

int checkSDArg(char *arg1[], char *arg2[])
{
	if (arg1 == NULL)
		return usageMsg("rexgen SD log|rxd|format|info");

	conf_file conf;
	if (CheckConfigFile(&conf) == 1) 
		GetSerialNumber(&DeviceObj, SN_IDX_SHORT, 1);
	SDCommandFlag = true;

	if (strcmp(arg1, "log") == 0)
	{
		if (arg2 == NULL)
			return usageMsg("rexgen SD log count|0..COUNT-1");

	    	if (strcmp(arg2, "count") == 0)
		{
			SendCommand(&DeviceObj, 0, &cmmdSDGetLogCount);
			printf("SD log count	%d\n", DeviceObj.ep[0].rx_data[5] + 0xFF * DeviceObj.ep[0].rx_data[6]);	
			return 0;
		}

		if (strspn(arg2, "0123456789") == strlen(arg2))
		{
			short logIdx = atoi(arg2);
			log_info info;
			if (LogInfoRXD(&DeviceObj, logIdx, &info) == 0)
			{
				printf("SD log info %i\n", logIdx);

				printdatef("Start time", 5);
				printdatef("End time", 9);
				printf("\t\tBytes\t\t%lu\n", info.bytes);
				printf("\t\tName\t\t%s\n", info.name);
				printf("\t\tStart sector\t%lu\n", info.sector_start);
				printf("\t\tEnd sector\t%lu\n", info.sector_stop);
			}
			else
				printf("Error: Can not load info for log %i\n", logIdx);
			return 0;
		}
		else
			return usageMsg("rexgen SD log count|0..COUNT-1");

		return 0;
	}

	if (strcmp(arg1, "file") == 0)	// just print the full file name by log idx 
	{
		if (arg2 == NULL)
			return 1;

		if (strspn(arg2, "0123456789") == strlen(arg2))
		{
			short logIdx = atoi(arg2);
			log_info info;
			if (LogInfoRXD(&DeviceObj, logIdx, &info) == 0)
				printf("%s\n", info.name_full);

			return 0;
		}
	}

	if (strcmp(arg1, "rxd") == 0) 
	{
		if (arg2 == NULL)
			return usageMsg("rexgen SD rxd all|0..COUNT-1");

		if (strspn(arg2, "0123456789") == strlen(arg2))
		{
			if (arg2 == NULL)
				return usageMsg("rexgen SD rxd LOG_IDX");

			unsigned short logIdx = atoi(arg2);
	    		DownloadRXD(&DeviceObj, logIdx, 1);

			return 0;		
		}

	    	if (strcmp(arg2, "all") == 0)
		{
			ReadConfigFile(&conf);

			CheckFolder(conf.rxd_folder);

			char *cmd[50];
			sprintf(cmd, "rm -f  %s*.rxd", conf.rxd_folder);
			exec(cmd);

			unsigned short beg, end;
			unsigned char res = PrepareDownloadAllRXD(&DeviceObj, &conf);

			WriteConfigFile(&conf);

			ProgressInit(conf.rxd_idx_end - conf.rxd_idx_beg, "Loading RXD files ...");

			for (unsigned short i = conf.rxd_idx_beg; i <= conf.rxd_idx_end; i++)
			{
				ProgressPos(PrgMax - (conf.rxd_idx_end - i));
				DownloadRXD(&DeviceObj, i, 0);
			}
			printf("\n");
			SDCommandFlag = false;
			return 0;
		}

		return usageMsg("rexgen SD rxd all|0..COUNT-1");
	}

	if (strcmp(arg1, "format") == 0) 
	{
		char in;    
		printf(BOLD_YELLOW"This action will format the SD card. Do you want to continue? [y/n]"COLOR_RESET);
		scanf(" %c", &in);

		if ((in == 121) || (in == 89))
		{
			DeviceObj.ep[0].timeout = 5000;
			if (SendCommand(&DeviceObj, 0, &cmmdSDFormat) == 0)
				printf("Format complete.\n");
		}
		SDCommandFlag = false;
		return 0;
	}

	if (strcmp(arg1, "info") == 0) 
	{
		DeviceObj.ep[0].timeout = 5000;
		SendCommand(&DeviceObj, 0, &cmmdSDInfo);

		MMCInfoStruct mmc_info;
		printf("SD info \n");

		printf("\t\tType\t\t");
			for (int i = 5; i < sizeof(mmc_info.Type) + 5; i++)
				printf("%c", DeviceObj.ep[0].rx_data[i]);
		printf("\n");
		printf("\t\tVersion\t\t%i\n", DeviceObj.ep[0].rx_data[11]);
		printf("\t\tBlock size\t%hu\n", ushortFromRXData(&DeviceObj, 12));
		printf("\t\tFull size\t%lu\n", ulongFromRXData(&DeviceObj, 14));
		printf("\t\tFree space\t%lu\n", ulongFromRXData(&DeviceObj, 18));

		return 0;
	}

	return usageMsg("rexgen SD log|rxd|format|info");
}

char GetSerialNumPageIdx(char *arg[])
{
	if  ((arg != NULL) && (strcmp(arg, "short") == 0)) 
		return SN_IDX_SHORT;

	return SN_IDX_LONG;
}

void checkArg(char *arg[])
{	
    int i;
    bool flag;

    void showHelp()
    {
	printf("USAGE: \n");
	printf("	rexgen [COMMAND [PARAMETERS] [COMMAND [PARAMETERS]] ...]\n\n");
	printf("COMMAND: \n");
//	printf("	shutdown	 	Shut down the device\n");
	printf("	version			Show versions of config and stream tools\n");
	printf("	firmware		Show device firmware version\n");
	printf("	serial	[short]		Show device serial number in selected format\n");
	printf("	SD log|rxd|format|info	Support SD card functions  \n");
	printf("	hwinfo                  Show hardware configuration and info for connected devices\n");
	printf("	date	set|get		Set/get system date and time to/from device\n");
	printf("	config			Show configuration name and UUID\n");
	printf("	configure *.rxc		Send configuration <*.rxc file>, created by RexDesk, to the device\n");
	printf("	reflash *.bin		Reflash device with <*.bin file> firmware\n");
    }

    if (arg == NULL)
    {
	showHelp();
	exit(1);
    }

    flag = false;
    if (strcmp(arg, "debug") == 0)
	flag = true;
    else if (strcmp(arg, "version") == 0)
	flag = true;
    else if (strcmp(arg, "firmware") == 0)
	flag = true;
    else if (strcmp(arg, "serial") == 0)
	flag = true;    
    else if (strcmp(arg, "config") == 0)
	flag = true;
    else if (strcmp(arg, "hwinfo") == 0)
	flag = true;
    else if (strcmp(arg, "dontreboot") == 0)
	flag = true;
    else if (strcmp(arg, "reflash") == 0)
	flag = true;
    else if (strcmp(arg, "configure") == 0)
	flag = true;
    else if (strcmp(arg, "shutdown") == 0)
	flag = true;
    else if (strcmp(arg, "date") == 0)
	flag = true;
    else if (strcmp(arg, "test") == 0)
	flag = true;
    else if (strcmp(arg, "cpu") == 0)
	flag = true;
    else if (strcmp(arg, "SD") == 0)
	flag = true;


    if (!flag)
    {
	showHelp();
	exit(1);
    }
}

int main(int argc, char *argv[]) 
{
	int VERSION_MAJOR = 1;
	int VERSION_MINOR = 17;

	void print_versions ()
	{
    		printf("Config tool 	%i.%i \n", VERSION_MAJOR, VERSION_MINOR);
		// uncomment to show stream version
//		system("/home/root/rexusb/rexgen_stream -v");
	}

	if (!InitUsbLibrary())
		return 1;

	signal(SIGINT, sigintHandler);
	checkArg(argv[1]);

	if (strcmp(argv[1], "version") == 0)
        {
		if (argc == 2)
		{
			print_versions();
			return 0;
		}
        }

	if (strcmp(argv[1], "cpu") == 0)
        {
		GetProcessorTypeFromFile(argv[2]);
		return 0;
	}
	
	if (exec("systemctl is-active application.service") == 0)
//	if (exec("systemctl list-unit-files rexgen_data.service | wc -l") > 3)
        	system("systemctl stop rexgen_data.service");

        RebootAfterReflash = 1;
	bool ReflashFlag = false;
	HideRequest = true;
	char *str;
	SDCommandFlag = false;

	if (InitDevice(&DeviceObj, 0x16d0, 0x0f14))//, 2)) 
	{	
		for (int i = 1; i < argc; ++i)
		{
			if (strcmp(argv[i], "debug") == 0)
			{
				CANDebugMode = 1;			
			}	
			else if (strcmp(argv[i], "version") == 0)
    			{	
				print_versions();
    			}
			else if (strcmp(argv[i], "firmware") == 0)
			{
				SendCommand(&DeviceObj, 0,  &cmmdGetFirmwareVersion);
				
				char buff[50];
				GetFirmware(DeviceObj.ep[0].rx_data[5], DeviceObj.ep[0].rx_data[6],
					    DeviceObj.ep[0].rx_data[7], DeviceObj.ep[0].rx_data[8],
					    DeviceObj.ep[0].rx_data[9], buff);
				printf("%s\n", buff);
			}
                        else if (strcmp(argv[i], "dontreboot") == 0)
                        {
                                RebootAfterReflash = 0;
                        }
                        else if (strcmp(argv[i], "reflash") == 0)
                        {
				if (argc < 3)
					printf("Please specify file name\n");
				else
				{
	                                Reflash(&DeviceObj, argv[i+1]);
					ReflashFlag = true;
				}
                        }
			else if (strcmp(argv[i], "serial") == 0)
			{
				unsigned long ser = GetSerialNumber(&DeviceObj, GetSerialNumPageIdx(argv[i + 1]), 0);
			}
			else if (strcmp(argv[i], "config") == 0)
			{
				GetConfig(&DeviceObj);
				printf("Config. name	%s\n", NameStruct);
				PrintUUID();
			}
			else if (strcmp(argv[i], "hwinfo") == 0)
			{
				GetHWSettings(&DeviceObj);
				printf("\n");
				
				SendCommand(&DeviceObj, 0, &cmmd41);
				SendCommand(&DeviceObj, 0, &cmmd43);

				printf("Hardware info	");
				for (int j = 9; j < DeviceObj.ep[0].rx_len -1; j++)
    				    printf("%c", DeviceObj.ep[0].rx_data[j]);
				printf("\n");

				SendCommand(&DeviceObj, 0, &cmmd42);

				printf("		");
				GetProcessorTypeByUSB(&DeviceObj);
			}
			else if (strcmp(argv[i], "date") == 0)
			{
				if (checkDateArg(argv[i + 1]) != 0)
					break;

				if (strcmp(argv[i + 1], "set") == 0)
				{
					time_t seconds = time(NULL);

					cmmdDateSet.cmd_data[5] = (seconds >> 24) & 0xFF;
					cmmdDateSet.cmd_data[4] = (seconds >> 16) & 0xFF;
					cmmdDateSet.cmd_data[3] = (seconds >> 8) & 0xFF;
					cmmdDateSet.cmd_data[2] = seconds & 0xFF;
					SendCommand(&DeviceObj, 0, &cmmdDateSet);
				}
			        else if (strcmp(argv[i + 1], "get") == 0)
				{
					SendCommand(&DeviceObj, 0, &cmmdDateGet);

					time_t seconds = DeviceObj.ep[0].rx_data[8] << 24 | 
						DeviceObj.ep[0].rx_data[7] << 16 | 
						DeviceObj.ep[0].rx_data[6] << 8 | 
						DeviceObj.ep[0].rx_data[5];
				        printf("%s", asctime(gmtime(&seconds)));
				}
			}

			else if (strcmp(argv[i], "SD") == 0)
			{
				if (checkSDArg(argv[i + 1], argv[i + 2]) != 0)
					break;
				
			}

			else if (strcmp(argv[i], "shutdown") == 0)
			{
				SendCommand(&DeviceObj, 0, &cmmdForceGoDeepSleep);
			}
			else if (strcmp(argv[i], "test") == 0)
			{
//				GetProcessorTypeFromFile(argv[i+1]);
			}

			else if (strcmp(argv[i], "configure") == 0)
			{
				if (argc < 3)
					printf("Please specify file name\n");
				else
			    	SendConfig(&DeviceObj, argv[i + 1]);		
			}
		}
	}

	if ( ReflashFlag)
	{
		ApplyReflash(&DeviceObj);
		return 0;
	}

	ReleaseDevice(&DeviceObj);
	ReleaseUsbLibrary();

//	if (exec("systemctl list-unit-files rexgen_data.service | wc -l") > 3)
	if (! SDCommandFlag)
		system("systemctl start rexgen_data.service");
	return 0;
}
