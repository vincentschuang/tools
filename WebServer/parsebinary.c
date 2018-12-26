#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <time.h>

#include "parsebinary.h"
#include "web.h"

void freadFileToBuffer(char * file, char *buffer, int num ){
	FILE *fp;
    fp = fopen (file, "r");
    fread(buffer,sizeof(char),num,fp);
    fclose(fp);
}
//only for small file
int getFileSize(char *path){
	struct stat fileStat;
	int file_fd;
	file_fd = open(path, O_RDONLY, 0644);
	//printf("path=%s fd =%d\n", path, file_fd);
	if(file_fd > -1 && fstat(file_fd, &fileStat) > -1){
		close(file_fd);
		return (int)fileStat.st_size;
	}else{
		printf("FILE Not Exist\n");
		return 0;
	}
}

int daysInMonth(char* YYMM){
	printf("\n\nYYMM=%s\n",YYMM );
	const int MONTHS[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int month = (YYMM[2]-'0')*10 + YYMM[3]-'0';
	printf("\n\nmonth=%d\n",month );
	if(month == 2){
		int year = (YYMM[0]-'0')*10 + YYMM[1]-'0';
		if(year%4 == 0){
			if(year%100 ==0 ){
				if(year%400 == 0){
					if(year%4000 == 0){
						return MONTHS[2-1];
					}else{
						return 29;
					}
				}else{
					return MONTHS[2-1];
				}
			}else{
				return 29;
			}
		}else{
			return MONTHS[2-1];
		}


	}else{
		return MONTHS[month-1];
	}

	
}

void genRegisterFile(char * plantID, char* fileName){


	time_t time_raw_format;
    struct tm * ptr_time;
    char tbuffer[50];

    time ( &time_raw_format );
    ptr_time = localtime ( &time_raw_format );
    strftime(tbuffer,50,"%Y%m%d%H%M%S",ptr_time);

	sprintf(fileName,"301_8888_%s_%s.data",plantID,tbuffer);

	printf("fileName=%s\n", fileName );

	char fileFullPath[256];
	memset(fileFullPath,0,256);
	sprintf(fileFullPath,"%s%s", REGISTER_FILE_PATH, fileName);
	printf("fileFullPath=%s\n", fileFullPath);
	
	//6+26+1=33
	char buffer[33];
	memset(buffer,0,33);

	//head  6
	buffer[5]=1;

	//plantID 26
	int i=0;
	for(i=0; i<26; i++){
		buffer[i+6] = plantID[i]-'0';
		//printf("%d", buffer[i+6]);
	}

	//reg 1
	buffer[6+26+1-1] = 0;
	
	i=0;
/*
	printf("buffer\n");
	while(i<33){
		printf("%d",buffer[i] );
		i++;
	}
	printf("\n");
*/
	FILE *pFile = fopen(fileFullPath,"w");
	if(pFile==NULL){
		printf("open fail\n");
		return ;
	}

	fwrite(buffer,sizeof(char),33,pFile);
	fclose(pFile);

}

int getMonthChecksum(char *buffer,int numOfData, char* YYMM){
	int sum=0;
	int i=0;
	for(i=0;i<numOfData;i++){
		sum += buffer[i];
	}

	int divider = (YYMM[2]-'0')*10 + YYMM[3]-'0' + 1;

	printf("\n\nsum=%d divider=%d\n", sum, divider);

	return sum%divider;
}

int getDayChecksum(char *buffer,int numOfData, char* YYYYMMDDhhmm){
	int sum=0;
	int i=0;
	for(i=0;i<numOfData;i++){
		sum += buffer[i];
	}

	int divider = (YYYYMMDDhhmm[4]-'0')*10 + YYYYMMDDhhmm[5]-'0' + (YYYYMMDDhhmm[6]-'0')*10 + YYYYMMDDhhmm[7]-'0';

	printf("\n\nsum=%d divider=%d\n", sum, divider);

	return sum%divider;
}

void genMonthFile(char * plantID, char* fileName, char * YYMM){
	time_t time_raw_format;
    struct tm * ptr_time;
    char timeBuffer[50];

    time ( &time_raw_format );
    ptr_time = localtime ( &time_raw_format );
    strftime(timeBuffer,50,"%Y%m%d%H%M%S",ptr_time);
	sprintf(fileName,"202_%s_%s_%s.data",YYMM , plantID, timeBuffer);

	printf("fileName=%s\n", fileName );

	char fileFullPath[256];
	memset(fileFullPath,0,256);
	sprintf(fileFullPath,"%s%s", MONTH_FILE_PATH, fileName);
	printf("fileFullPath=%s\n", fileFullPath);
	
	//6+10+26+12+5+1488+2=1549
	char buffer[1550];
	memset(buffer,0,1550);

	//files  6
	buffer[5]=1;

	//dayly ID
	int i=0;
	char dailyID[]={'4','0','0','0','0','9','9','0','0','0'};
	printf("\n\ndailyID\n");
	for(i=6; i<16; i++){
		buffer[i] = dailyID[i-6]-'0';
		printf("%d", buffer[i] );
	}

	//plantID 26
	printf("\n\nplantID\n");
	for(i=16; i<42; i++){
		buffer[i] = plantID[i-16]-'0';
		printf("%d",buffer[i] );
	}

	//YYYYMMDDhhmm 12
	memset(timeBuffer,0,50);
	sprintf(timeBuffer,"20%s010000",YYMM);
	printf("\n\nYYYYMMDDhhmm\n");
	for(i=42; i<54;i++){
		buffer[i] = timeBuffer[i-42]-'0';
		printf("%d",buffer[i] );
	}

	//num of suppress data  5
	int days = daysInMonth(YYMM);
	printf("\n\ndays\n");
	printf("%d\n", days);
	int numOfSuppressData = 24*2*days;

	printf("\n\nnumber of suppress data\n");
	printf("%d", numOfSuppressData);
	buffer[54]=0;
	buffer[55] = numOfSuppressData/1000;
	buffer[56] = (numOfSuppressData%1000)/100;
	buffer[57] = (numOfSuppressData%100)/10;
	buffer[58] = (numOfSuppressData%10)/1;

	//suppress data   num of suppress data
	//month value =100;
	SUPPRESS_T *pValue = &gValue.suppress;

	printf("\n\nsuppress data\n");
	for(i=59; i<59+numOfSuppressData; i++){
		buffer[i] = pValue->monthValue[0];
		//printf("%d", buffer[i]);
	}

	//calculate checksum
	printf("\n\nchecksum\n");
	int checksum=0;
	checksum = getMonthChecksum(&buffer[59],numOfSuppressData, YYMM );
	printf("%d\n",checksum );
	buffer[59+numOfSuppressData+0]= (checksum%100)>0?(checksum%100)/10:0;
	buffer[59+numOfSuppressData+1]= (checksum%10)/1;

	FILE *pFile = fopen(fileFullPath,"w");
	if(pFile==NULL){
		printf("open fail\n");
		return ;
	}
	fwrite(buffer,sizeof(char),(59+numOfSuppressData+2),pFile);
	fclose(pFile);
}

void genTodayFile(char * plantID, char* fileName){
	time_t time_raw_format;
    struct tm * ptr_time;
    char timeBuffer[50];

    time ( &time_raw_format );
    ptr_time = localtime ( &time_raw_format );
    strftime(timeBuffer,50,"%Y%m%d%H%M%S",ptr_time);
	sprintf(fileName,"203_0000_%s_%s.data" , plantID, timeBuffer);

	printf("fileName=%s\n", fileName );

	char fileFullPath[256];
	memset(fileFullPath,0,256);
	sprintf(fileFullPath,"%s%s", DAY_FILE_PATH, fileName);
	printf("fileFullPath=%s\n", fileFullPath);
	
	//6+10+26+12+5+1488+2=1549
	char buffer[1550];
	memset(buffer,0,1550);

	//files  6
	buffer[5]=1;

	//daily ID
	int i=0;
	char dailyID[]={'4','0','4','0','0','0','0','0','0','0'};
	printf("\n\ndailyID\n");
	for(i=6; i<16; i++){
		buffer[i] = dailyID[i-6]-'0';
		printf("%d", buffer[i] );
	}

	//plantID 26
	printf("\n\nplantID\n");
	for(i=16; i<42; i++){
		buffer[i] = plantID[i-16]-'0';
		printf("%d",buffer[i] );
	}

	//YYYYMMDDhhmm 12
	memset(timeBuffer,0,50);
	strftime(timeBuffer,50,"%Y%m%d0000",ptr_time);
	printf("\n\nYYYYMMDDhhmm\n");
	for(i=42; i<54;i++){
		buffer[i] = timeBuffer[i-42]-'0';
		printf("%d",buffer[i] );
	}

	//num of suppress data  5
	int days = 1;
	printf("\n\ndays\n");
	printf("%d\n", days);
	int numOfSuppressData = 24*2*days;

	printf("\n\nnumber of suppress data\n");
	printf("%d", numOfSuppressData);
	buffer[54]=0;
	buffer[55] = numOfSuppressData/1000;
	buffer[56] = (numOfSuppressData%1000)/100;
	buffer[57] = (numOfSuppressData%100)/10;
	buffer[58] = (numOfSuppressData%10)/1;

	//suppress data   num of suppress data
	//day value =100;
	SUPPRESS_T *pValue = &gValue.suppress;

	printf("\n\nsuppress data\n");
	printf("%d\n", pValue->todayValue[0]);
	for(i=59; i<59+numOfSuppressData; i++){
		buffer[i] = pValue->todayValue[0];
		//printf("%d", buffer[i]);
	}

	//an extra bit 6 between suppress data and checksum
	buffer[59 + numOfSuppressData ]=6;

	//calculate checksum  2
	printf("\n\nchecksum\n");
	int checksum=0;
	checksum = getDayChecksum(&buffer[59],numOfSuppressData, timeBuffer );
	printf("%d\n",checksum );
	buffer[59 + numOfSuppressData + EXTRA_BIT + 0]= (checksum%100)>0 ? (checksum%100)/10 : 0;
	buffer[59 + numOfSuppressData + EXTRA_BIT + 1]= (checksum%10)/1;

	//next access time   14
	//char nextAccexxTime[14] = {'2','0','1','8','1','2','2','1','1','8','3','0','5','5'};
	for(i=0; i<14;i++){
		//buffer[59+numOfSuppressData+EXTRA_BIT+2+i] = nextAccexxTime[i]-'0';
		buffer[59+numOfSuppressData+EXTRA_BIT+2+i] = pValue->nextAccessTime[i]-'0';
	}

	FILE *pFile = fopen(fileFullPath,"w");
	if(pFile==NULL){
		printf("open fail\n");
		return ;
	}
	fwrite(buffer,sizeof(char),(59 + numOfSuppressData + EXTRA_BIT + 2 + 14),pFile);
	fclose(pFile);
}

void readRegisterBin(){
	int  lSize;
	char * buffer;
	char * payloadptr;
	char * payloadend;

	lSize = getFileSize(TEST_FILE_PATH);
	printf("sizeof file %d\n",lSize );

	// allocate memory to contain the whole file:
	buffer = (char*) malloc (sizeof(char)*lSize);
	if (buffer == NULL) {
		fputs ("Memory error",stderr);
		exit (2);
	}

	freadFileToBuffer(TEST_FILE_PATH, buffer, lSize);
	/* the whole file is now loaded in the memory buffer. */

	payloadptr = strstr(buffer,"\r\n\r\n");
	payloadptr = strstr(&payloadptr[4],"\r\n\r\n");
	if(!payloadptr){
		printf("payload ptr is NULL\n");
	}

	payloadend = strstr(&payloadptr[4],"\r\n");
	if(!payloadend){
		printf("payload end is NULL\n");
	}

	int ps, pe;
	ps = payloadptr+4-buffer;
	//pe = payloadend-2 - buffer;

	printf("ps=%d \n",ps );

	int i=-1;
	while(i++ < lSize){
		//printf("i=%d ", i);
		if(i >= ps ){
			printf("%d ", buffer[i]);
		}else{
			printf("%c", buffer[i]);
		}
	}
	printf("EXIT LOOP\n");
	// terminate
	free (buffer);
	return ;


}