#ifndef __PARSEBINARY_H__
#define __PARSEBINARY_H__

#define TEST_REGISTER_FILE_PATH "/home/pvicm/register.bin"
#define TEST_MONTH_FILE_PATH "/home/pvicm/month.bin"
#define TEST_DAY_FILE_PATH "/home/pvicm/today.bin"

#define TEST_FILE_PATH TEST_DAY_FILE_PATH

#define REGISTER_FILE_PATH "/home/pvicm/files/register/"
#define MONTH_FILE_PATH    "/home/pvicm/files/month/"
#define DAY_FILE_PATH      "/home/pvicm/files/day/"

#define EXTRA_BIT 1

void readRegisterBin(void);
void genRegisterFile(char * plantID, char * fileName);
void genMonthFile(char * plantID, char* fileName, char * YYMM);
void genTodayFile(char * plantID, char* fileName);
int getFileSize(char *path);
void freadFileToBuffer(char * file, char *buffer, int num );
int daysInMonth(char* YYMM);
int getMonthChecksum(char *buffer,int numOfData, char* YYMM);
int getDayChecksum(char *buffer,int numOfData, char* YYYYMMDDhhmm);

#endif