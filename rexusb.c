#include <stdio.h>
#include "communication.h"
#include "commands.h"
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

DeviceStruct DeviceObj;

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

void removeMinus(char *arg)
{
    int i, j;
    int len = strlen(arg);
    for(i=0; i<len; i++)
    {
        if(arg[i] == '-')
        {
            for(j=i; j<len; j++)
            {
                arg[j] = arg[j+1];
            }
            len--;
            i--;
        }
    }
}

int checkDateArg(char *arg[])
{
	if ((arg == NULL) || ((strcmp(arg, "set") != 0) && (strcmp(arg, "get") != 0)))
	{
		printf("USAGE: \n");
		printf("	rexgen date set|get\n\n");
		return 1;
	}
	return 0;
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
	printf("	shutdown	 	Shut down the device\n");
	printf("	version			Show versions of config and stream tools\n");
	printf("	firmware		Show device firmware version\n");
	printf("	serial	[short]		Show device serial number in long or short format\n");
	printf("	hwinfo                  Show info for connected hardware\n");
	printf("	date	set|get		Set/get system date and time to/from device\n");
	printf("	config			Show device configuration\n");
	printf("	reflash *.bin		Reflash device with <*.bin file> firmware\n");
	printf("	configure *.rxc		Send configuration <*.rxc file>, created by RexDesk, to the device\n\n");
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

    if (!flag)
    {
	showHelp();
	exit(1);
    }
}

int exec(char *command) {
    char buffer[128];
    char result[2] = "-1";

    // Open pipe to file
    FILE* pipe = popen(command, "r");
    if (!pipe)
	return -1;
   

    // read till end of process:
    while (!feof(pipe)) 
	// use buffer to read and add to result
	if (fgets(buffer, 128, pipe) != NULL)
	    strcpy(result, buffer);
    
   pclose(pipe);
   return atoi(result);
}

int main(int argc, char *argv[]) 
{
	int VERSION_MAJOR = 1;
	int VERSION_MINOR = 14;

	void print_versions ()
	{
    		printf("Config tool	%i.%i \n", VERSION_MAJOR, VERSION_MINOR);
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

	if (exec("systemctl list-unit-files rexgen_data.service | wc -l") > 3)
            system("systemctl stop rexgen_data.service");

        RebootAfterReflash = 1;
	bool ReflashFlag = false;
	HideRequest = true;
	char *str;
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
				printf("Firmware	%d.%d.%d", DeviceObj.ep[0].rx_data[5] + 0xFF * DeviceObj.ep[0].rx_data[6], DeviceObj.ep[0].rx_data[7],  DeviceObj.ep[0].rx_data[8]);

				// interpretation of type 
				char type[3] = "";
    				type[0] = '.';		
				type[1] = ' ';		
				type[2] = ' ';		
    				type[3] = '\0';		
				switch (DeviceObj.ep[0].rx_data[9])
				{
					case 0:
						type[0] = ' ';
						break;
					case 1:
						type[1] = 'A';
						break;
					case 2:
						type[1] = 'B';
						break;
					case 3:
						type[1] = 'R';
						type[2] = 'C';
						break;
				} 
				printf("%s ", type);
				printf("\n");
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
				GetSerialNumber(&DeviceObj, GetSerialNumPageIdx(argv[i + 1]));
			}
			else if (strcmp(argv[i], "config") == 0)
			{
				GetHWSettings(&DeviceObj);
			}
			else if (strcmp(argv[i], "hwinfo") == 0)
			{
				SendCommand(&DeviceObj, 0, &cmmd41);
				SendCommand(&DeviceObj, 0, &cmmd43);
				printf("Hardware info	");
				for (int j = 9; j < DeviceObj.ep[0].rx_len -1; j++)
    				    printf("%c", DeviceObj.ep[0].rx_data[j]);
				printf("\n");

				SendCommand(&DeviceObj, 0, &cmmd42);
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

			else if (strcmp(argv[i], "shutdown") == 0)
			{
				SendCommand(&DeviceObj, 0, &cmmdForceGoDeepSleep);
			}
// ****** obsolete *******
			else if (strcmp(argv[i], "initcan") == 0)
			{
				initStruct cmmdInit = {
					bus: atoi(argv[i+1]),
					fdf: 0,
					brs: 0,
					silent: atoi(argv[i + 3]),
					speed: atoi(argv[i+2]),
					speed_fd: 0,
					rx_id: (int)strtol(argv[i+4], NULL, 0),
				};

				SendInitBus(&DeviceObj, &cmmdInit);
			}
			else if (strcmp(argv[i], "deinitbus") == 0)
			{
				SendCommand(&DeviceObj, 0, &cmmdDeinitBus);
			}
			else if (strcmp(argv[i], "initcanfd") == 0)
			{
				initStruct cmmdInit = {
					bus: atoi(argv[i+1]),
					fdf: atoi(argv[i+6]),
					brs: atoi(argv[i+7]),
					silent: atoi(argv[i + 3]),
					speed: atoi(argv[i+2]),
					speed_fd: atoi(argv[i+5]),
					rx_id: (int)strtol(argv[i+4], NULL, 0),
				};

				SendInitBus(&DeviceObj, &cmmdInit);
			}
			else if (strcmp(argv[i], "cansend") == 0 || strcmp(argv[i], "cansendext") == 0)
			{
				cmdCANstruct cmdCan;
				cmdCan.Extended = 0;
				if (strcmp(argv[i], "cansendext") == 0){
					cmdCan.Extended = 1;
					printf("ext");
				}
				
				cmdCan.Bus = atoi(argv[i+1]);				
				cmdCan.DLC = atoi(argv[i+2]);
				cmdCan.ID = (int)strtol(argv[i+3], NULL, 0);
				cmdCan.TimeStamp = 2000;
				
		        	for (int j = 0; j < 64; ++j)
					cmdCan.Data[j] = 0;
            	
				for (int j = 0; j < cmdCan.DLC; ++j)
				{
					char x;
					if (i+j+4 < argc)
					{
    					sscanf(argv[i+j+4], "%x", &x);
    					cmdCan.Data[j] = x;
					}
					else
						cmdCan.Data[j] = 0;					
				}
				SendCANCmd(&DeviceObj, &cmdCan);			
			}
			else if (strcmp(argv[i], "canread") == 0)
			{
				unsigned long msgCount = 0;
				if (argc > i+2)
					msgCount = atoi(argv[i+2]);
				ReadCANMsg(&DeviceObj, atoi(argv[i+1]), msgCount);				
			}
// ***************
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

	if (exec("systemctl list-unit-files rexgen_data.service | wc -l") > 3)
		system("systemctl start rexgen_data.service");
	return 0;
}
