#ifndef __HTTP_JET_H__
#define __HTTP_JET_H__

#include "http.h"

#define one_ele_json_string(buffer,name,value) sprintf(buffer,"\"%s\":\"%s\"",name,value)
#define one_ele_json_number(buffer,name,value) sprintf(buffer,"\"%s\":%d",name,value)

#define json_string_element(buffer,new_ele) sprintf(buffer,"\"%s\"", new_ele)
#define json_number_element(buffer,new_ele) sprintf(buffer,"%d", new_ele)

#define json_string_array_add(buffer,old_ele,new_ele) sprintf(buffer,"%s,\"%s\"",old_ele, new_ele)
#define json_number_array_add(buffer,old_ele,new_ele) sprintf(buffer,"%s,%d",old_ele, new_ele)

#define json_tag_add_array(buffer,tag,array) sprintf(buffer,"\"%s\":[%s]",tag, array)

#define combine_json_elements(buffer,ele1,ele2) sprintf(buffer,"%s,%s",ele1,ele2)
#define be_json_object(buffer,ele) sprintf(buffer,"{%s}",ele)

#define COPY_TO_TMP(i,pValue,tmp) do{				\
	if('0' > pValue->nextAccessTime[0]){            \
		for(i=0;i<14;i++){							\
			tmp[i] = pValue->nextAccessTime[i]+'0';	\
		}											\
	}else{											\
		for(i=0;i<14;i++){							\
			tmp[i] = pValue->nextAccessTime[i];		\
		}											\
	}												\
}while(0)

#define TEST_REGISTER_FILE_PATH HOME"register.bin"
#define TEST_MONTH_FILE_PATH HOME"month.bin"
//#define TEST_DAY_FILE_PATH "/home/pvicm/today.bin"

#define TEST_DAY_FILE_PATH HOME"files/day/203_0000_09086660000001000000010010_20190102084252.data"

#define TEST_FILE_PATH TEST_DAY_FILE_PATH

#define REGISTER_FILE_PATH HOME"files/register/"
#define MONTH_FILE_PATH    HOME"files/month/"
#define DAY_FILE_PATH      HOME"files/day/"

#define TEST_REG_FILE REGISTER_FILE_PATH"301_8888_09086660000001000000010010_20190103153303.data"
#define TEST_DAY_FILE DAY_FILE_PATH"203_0000_09086660000001000000010010_20190103133934.data"
#define TEST_MONTH_FILE MONTH_FILE_PATH"202_2001_09086660000001000000010010_20190103144334.data"

#define EXTRA_BIT 1

#define TYPE_LEN 6
#define PLANT_ID_LEN 26
#define REG_STATUS_LEN 1
#define SCH_ID_LEN 10
#define TIME_LEN 12
#define DATA_NUM_LEN 5
#define DAY_DATA_LEN 48
#define CHECKSUM_LEN 2
#define NEXT_TIME_LEN 14
#define MONTH_DATA_LEN 1488

struct monthPacketUpper{
	char type[TYPE_LEN];
	char scheduleID[SCH_ID_LEN];
	char plantID[PLANT_ID_LEN];
	char time[TIME_LEN];
	char dataNum[DATA_NUM_LEN];
	char data[];
//	char checksum[CHECKSUM_LEN];
};
typedef struct monthPacketUpper monthPacketUpper_t;

struct monthPacketLower{
	char checksum[CHECKSUM_LEN];
};
typedef struct monthPacketLower monthPacketLower_t;

struct dailyPacket{
	char type[TYPE_LEN];
	char scheduleID[SCH_ID_LEN];
	char plantID[PLANT_ID_LEN];
	char time[TIME_LEN];
	char dataNum[DATA_NUM_LEN];
	char data[DAY_DATA_LEN];
	char extra[EXTRA_BIT];
	char checksum[CHECKSUM_LEN];
	char nextTime[NEXT_TIME_LEN];
};
typedef struct dailyPacket dailyPacket_t;

struct regPacket{
	char type[TYPE_LEN];
	char plantID[PLANT_ID_LEN];
	char success[REG_STATUS_LEN];
};
typedef struct regPacket regPacket_t;




void scheduleSend(httpRequest_T * httpRequest);
void scheduleConfig(httpRequest_T * httpRequest);
void getScheduleConfig(httpRequest_T * httpRequest);
void readRegisterBin(void);
void genRegisterFile(char * plantID, char * fileName);
void genMonthFile(char * plantID, char* fileName, char * YYMM);
void genTodayFile(char * plantID, char* fileName);
void readRegisterFile();
void readTodayFile();
void readMonthFile();

int daysInMonth(char* YYMM);
int getMonthChecksum(char *buffer,int numOfData, char* YYMM);
int getDayChecksum(char *buffer,int numOfData, char* YYYYMMDDhhmm);
void SendJetResponsePacket(char*fileFullPath, char*fileName, char* headBuffer,char* payloadBuffer, httpRequest_T * httpRequest);
void readTestBin();
#endif