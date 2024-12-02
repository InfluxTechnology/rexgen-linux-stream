#ifndef _inf_common_
#define _inf_common_

#define CONFIG_LINE_LEN 	100
#define CONFIG_NAME_LEN 	20
#define CONFIG_VAL_LEN 		50
#define CONFIG_FILE		"/home/root/rexusb/etc/rexgen.conf"
#define CONFIG_RXD_FOLDER	"/home/root/rexusb/rxd/"

#define BOLD_RED		"\e[1;31m"
#define BOLD_GREEN		"\e[1;32m"
#define BOLD_YELLOW 		"\e[1;33m"
#define COLOR_RESET 		"\x1b[0m"

// progress vars
unsigned int PrgMax;
unsigned int PrgPos;
char PrgStr[50];

typedef struct 
{
	char *rxd_folder[50];
	unsigned short rxd_idx_beg;
	unsigned short rxd_idx_end;
	char rxd_folder_full;
	char *rex_sn_short[10]; 
//	char *rex_sn_Long[30];
}conf_file;

unsigned long GetAvailableSpace(const char* path);
int exec(char *command);
int CheckConfigFile(conf_file *conf);
void ReadConfigFile(conf_file *conf);
void WriteConfigFile(conf_file *conf);
void CheckFolder(const char* path);
void ProgressPos(unsigned int pos);
void ProgressInit(unsigned int max, char* text);


#endif
