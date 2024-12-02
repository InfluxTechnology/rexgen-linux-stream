#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
#include <sys/statvfs.h>
#include <dirent.h>

unsigned long GetAvailableSpace(const char* path)
{
	struct statvfs stat;

	 if (statvfs(path, &stat) != 0) 
		return -1;
  
	 return stat.f_bsize * stat.f_bavail;
}

int exec(char *command) 
{
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

int CheckConfigFile(conf_file *conf)
{
	if (access(CONFIG_FILE, F_OK) != 0)
	{	
		conf->rxd_idx_beg = 0;
		conf->rxd_idx_end = 0;
		conf->rxd_folder_full = 0;
		sprintf(conf->rex_sn_short, "%s", "SN");
		sprintf(conf->rxd_folder, "%s", CONFIG_RXD_FOLDER);

		WriteConfigFile(conf);
		return 1;
	}
	return 0;
}

void ReadConfigFile(conf_file *conf)
{
	char buff[CONFIG_LINE_LEN];
	char name[CONFIG_NAME_LEN];
	char val[CONFIG_VAL_LEN];

	FILE *confFile;
	confFile = fopen(CONFIG_FILE, "r");

	while(fgets(buff, CONFIG_LINE_LEN, confFile) != NULL)  
	{
		if (buff[0] == '#' || strlen(buff) < 4) 
			continue;

		if (strstr(buff, "RXD_FOLDER "))
		{
			int res = sscanf(buff, "%s = %s\n", name, val);

			if (res == 1)
				sprintf(conf->rxd_folder, "%s", CONFIG_FILE);
			else
				sprintf(conf->rxd_folder, "%s", val);
		}

		if (strstr(buff, "RXD_IDX_BEG"))
		{
			sscanf(buff, "%s = %s\n", name, val);
			conf->rxd_idx_beg = (unsigned short)(atoi(val));
		}

		if (strstr(buff, "RXD_IDX_END"))
		{
			sscanf(buff, "%s = %s\n", name, val);
			conf->rxd_idx_end = (unsigned short)(atoi(val));
		}

		if (strstr(buff, "RXD_PARTITION_FULL"))
		{
			sscanf(buff, "%s = %s\n", name, val);
			conf->rxd_folder_full = (char)(atoi(val));
		}

		if (strstr(buff, "REX_SN_SHORT"))
		{
			sscanf(buff, "%s = %s\n", name, val);
			sprintf(conf->rex_sn_short, "%s", val);
		}

	}

	fclose(confFile);
}


void WriteConfigFile(conf_file *conf)
{
	char buff[CONFIG_LINE_LEN];

	FILE *confFile;
	confFile = fopen(CONFIG_FILE, "w");

	sprintf(buff, "# Folder where rxd files will downloaded from RexGen SD card\n");
	fwrite(buff, strlen(buff), 1, confFile);
	
	sprintf(buff, "RXD_FOLDER = %s\n", conf->rxd_folder);
	fwrite(buff, strlen(buff), 1, confFile);

	sprintf(buff, "\n");
	fwrite(buff, strlen(buff), 1, confFile);

	sprintf(buff, "# rexgen tool use these variables to export rxd files\n");
	fwrite(buff, strlen(buff), 1, confFile);

	sprintf(buff, "# !!! Please do not edit them !!!\n");
	fwrite(buff, strlen(buff), 1, confFile);

	sprintf(buff, "RXD_IDX_BEG = %i\n", conf->rxd_idx_beg);
	fwrite(buff, strlen(buff), 1, confFile);

	sprintf(buff, "RXD_IDX_END = %i\n", conf->rxd_idx_end);
	fwrite(buff, strlen(buff), 1, confFile);

	sprintf(buff, "RXD_PARTITION_FULL = %i\n", conf->rxd_folder_full);
	fwrite(buff, strlen(buff), 1, confFile);

	sprintf(buff, "REX_SN_SHORT = %s\n", conf->rex_sn_short);
	fwrite(buff, strlen(buff), 1, confFile);
	fclose(confFile);
}

void CheckFolder(const char* path)
{
	DIR* dp;
	dp = opendir(path);
	if (dp == NULL)
	{
		char *cmd[50];
		sprintf(cmd, "mkdir -p %s", path);
		exec(cmd);
	}
}

void ProgressPos(unsigned int pos)
{
	unsigned short proc = pos * 100 / PrgMax;
	if (PrgPos == proc) return;

	PrgPos = proc;
	printf("\r%s %d %c ", PrgStr, proc, 37);
	fflush(stdout);
}

void ProgressInit(unsigned int max, char* text)
{
	PrgMax = max;
	PrgPos = 0;
	strcpy(PrgStr, text);
}
