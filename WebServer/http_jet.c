#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "http_jet.h"

void SendJetResponsePacket(char*fileFullPath, char*fileName, char* headBuffer,char* payloadBuffer, httpRequest_T * httpRequest){
	//get File
	int fileSize = getFileSize(fileFullPath);
	char *fileBuffer;
	fileBuffer = freadFileToBuffer(fileFullPath, fileSize );
	if(!fileBuffer){
		printf("Read File Fail\n");
	}

	int tailSize = strlen(jetPayloadTail);
	//write payload head
	sprintf(payloadBuffer,jetPayloadHeader, fileName, fileSize );
	int headLen = strlen(payloadBuffer);
	//write file
	memcpy(&payloadBuffer[headLen], fileBuffer, fileSize);
	free(fileBuffer);
	//write tail
	memcpy(&payloadBuffer[headLen+fileSize],jetPayloadTail,tailSize);
	//write head
	sprintf(headBuffer,jetResponseHeader,headLen+fileSize+tailSize);
#ifdef USE_OPENSSL	
	SSL_write(httpRequest->ssl, headBuffer, strlen(headBuffer));
    SSL_write(httpRequest->ssl, payloadBuffer, headLen+fileSize+tailSize);
#else
	write(httpRequest->newfd, headBuffer, strlen(headBuffer));
	write(httpRequest->newfd, payloadBuffer, headLen+fileSize+tailSize);
#endif
}

void scheduleSend(httpRequest_T * httpRequest){
	
	char headBuffer[512],payloadBuffer[2048];
	memset(headBuffer,0,512);
	memset(payloadBuffer,0,2048);

	printf("scheduleSend\n" );

	char  plantID[27],  mac[13], schedule_kbn[5];

	parseItemFromPayload("power_plant_id", httpRequest->payload, plantID);
	parseItemFromPayload("mac_address", httpRequest->payload, mac);
	parseItemFromPayload("schedule_kbn", httpRequest->payload, schedule_kbn);

	printf("pid=%s mac=%s kbn=%s\n",plantID, mac, schedule_kbn );

	char fileName[64],fileFullPath[256];
	memset(fileName,0,64);
	memset(fileFullPath,0,256);
	

	if(!strncmp(schedule_kbn,"8888",4)){
		printf("8888 is register\n");
		readRegisterFile();
		//return;
		genRegisterFile(plantID, fileName);	
		sprintf(fileFullPath,"%s%s", REGISTER_FILE_PATH, fileName);

		SendJetResponsePacket(fileFullPath, fileName, headBuffer, payloadBuffer, httpRequest);
	}else if(!strncmp(schedule_kbn,"0000",4)){//day
		printf("0000 is Today\n");
		//readTodayFile();
		//return;
		genTodayFile(plantID, fileName);	
		sprintf(fileFullPath,"%s%s", DAY_FILE_PATH, fileName);

		SendJetResponsePacket(fileFullPath, fileName, headBuffer, payloadBuffer, httpRequest);
	}else{//YYMM
		printf("%s is Month\n",schedule_kbn);
		//readMonthFile();
		//return;
		genMonthFile(plantID, fileName, schedule_kbn);
		sprintf(fileFullPath,"%s%s", MONTH_FILE_PATH, fileName);
		printf("\nfileFullPath=%s\n", fileFullPath);
		//generate return packet
		SendJetResponsePacket(fileFullPath, fileName, headBuffer, payloadBuffer, httpRequest);
	}	
}

void scheduleConfig(httpRequest_T * httpRequest){

	char headBuffer[512],payloadBuffer[2048];
	memset(headBuffer,0,512);
	memset(payloadBuffer,0,2048);

	printf("scheduleConfig\n" );

	char  monthValue[3],  todayValue[48*4], nextAccessTime[15];
	SUPPRESS_T *pValue = &gValue.suppress;

	parseItemFromPayload("monthValue", httpRequest->payload, monthValue);
	parseItemFromPayload("todayValue", httpRequest->payload, todayValue);
	parseItemFromPayload("nextAccessTime", httpRequest->payload, nextAccessTime);

	printf("monthValue=%s\n", monthValue);
	printf("todayValue=%s\n", todayValue);
	printf("nextAccessTime=%s\n", nextAccessTime);

	pValue->monthValue[0] = atoi(monthValue);

	int i=0, len=0, comma=0, tmp=0;
	for(i=0, len=strlen(todayValue); i<len; i++){
		if(todayValue[i] == ',' ){
			pValue->todayValue[comma++] = tmp;
			tmp = 0;
		}else{
			tmp = tmp*10 + todayValue[i]-'0';
		}
	}

	for(i=0;i<48;i++){
		printf("%d ", pValue->todayValue[i]);
	}

	printf("nextAccessTime=");

	for(i=0; i<14; i++){
		pValue->nextAccessTime[i] = nextAccessTime[i]-'0';
		printf("%d", pValue->nextAccessTime[i]);
	}

	printf("monthValue=%d\n", pValue->monthValue[0]);
	printf("todayValue=%d\n", pValue->todayValue[0]);
	
	sprintf(headBuffer,getResponseHeader,"text/html",(int)strlen("success"));
	sprintf(payloadBuffer,"%s","success");
	
#ifdef USE_OPENSSL
	SSL_write(httpRequest->ssl, headBuffer, strlen(headBuffer));
	SSL_write(httpRequest->ssl, payloadBuffer, strlen(payloadBuffer));
#else
	write(httpRequest->newfd, headBuffer, strlen(headBuffer));
	write(httpRequest->newfd, payloadBuffer, strlen(payloadBuffer));
#endif
}

void getScheduleConfig(httpRequest_T * httpRequest){

	char headBuffer[512],payloadBuffer[2048];
	memset(headBuffer,0,512);
	memset(payloadBuffer,0,2048);

	char monthValue[128];
	memset(monthValue,0,128);
	char todayValue[512];
	char todayValueArr[512];
	memset(monthValue,0,512);
	char nextAccessTime[128];
	memset(nextAccessTime,0,128);
	char combine[512];
	memset(combine,0,512);
	printf("getScheduleConfig\n" );
	char tmp[15];
	memset(tmp,0,15);

	SUPPRESS_T *pValue = &gValue.suppress;

	int i = 0;
	COPY_TO_TMP(i,pValue,tmp);

	printf("nextAccessTime=%s\n", tmp);
	one_ele_json_string(nextAccessTime, "nextAccessTime", tmp);
	one_ele_json_number(monthValue, "monthValue", pValue->monthValue[0]);

	json_number_element(todayValueArr, pValue->todayValue[0]);
	json_number_array_add(todayValueArr, todayValueArr, pValue->todayValue[1]);

	for ( i = 2; i < 48; ++i)
	{
		json_number_array_add(todayValueArr, todayValueArr, pValue->todayValue[i]);
	}
	printf("todayValueArr=%s\n", todayValueArr);
	json_tag_add_array(todayValue,"todayValue",todayValueArr);
	printf("todayValue=%s\n", todayValue);
	combine_json_elements(combine, nextAccessTime, monthValue);
	combine_json_elements(combine, combine, todayValue);
	be_json_object(payloadBuffer,combine);

	printf("monthValue=%s\n", monthValue);
	printf("todayValue=%s\n", todayValue);
	printf("nextAccessTime=%s\n", nextAccessTime);
	printf("payloadBuffer=%s\n", payloadBuffer);


	sprintf(headBuffer,getResponseHeader,"application/json",(int)strlen(payloadBuffer));
	
#ifdef USE_OPENSSL
	SSL_write(httpRequest->ssl, headBuffer, strlen(headBuffer));
	SSL_write(httpRequest->ssl, payloadBuffer, strlen(payloadBuffer));
#else
	write(httpRequest->newfd, headBuffer, strlen(headBuffer));
	write(httpRequest->newfd, payloadBuffer, strlen(payloadBuffer));
#endif
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

	//new method
	regPacket_t packet;
	int i=0;
	
	memset(packet.type ,0 ,TYPE_LEN );
	packet.type[TYPE_LEN-1]=1;

	for(i=0; i<PLANT_ID_LEN; i++){
		packet.plantID[i] = plantID[i]-'0';
	}

	//always success
	packet.success[0]=0;

	FILE *pFile = fopen(fileFullPath,"w");
	if(pFile==NULL){
		printf("open fail\n");
		return ;
	}

	fwrite(&packet,sizeof(char),sizeof(packet),pFile);
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
    char timeBuffer[50], thisYYMMBuffer[4];
    int year, month, today;

    time ( &time_raw_format );
    ptr_time = localtime ( &time_raw_format );
    strftime(timeBuffer,50,"%Y%m%d%H%M%S",ptr_time);
	sprintf(fileName,"202_%s_%s_%s.data",YYMM , plantID, timeBuffer);

	month = ptr_time->tm_mon + 1;
	year = ptr_time->tm_year + 1900 - 2000;
	today = ptr_time->tm_mday;
	sprintf(thisYYMMBuffer,"%d%d",year,month);

	printf("month=%d year=%d\n", month, year);

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

	//replace today to month
	int start = 0;
	if(!strncmp(thisYYMMBuffer,YYMM,4)){
		start = (today-1)*48;
		for(i=0;i<48;i++){
			buffer[59+start+i] = pValue->todayValue[i];
		}
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

	for(i=59; i<59+numOfSuppressData; i++){
		buffer[i] = pValue->todayValue[i-59];
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
		buffer[59+numOfSuppressData+EXTRA_BIT+2+i] = pValue->nextAccessTime[i];
	}

	FILE *pFile = fopen(fileFullPath,"w");
	if(pFile==NULL){
		printf("open fail\n");
		return ;
	}
	fwrite(buffer,sizeof(char),(59 + numOfSuppressData + EXTRA_BIT + 2 + 14),pFile);
	fclose(pFile);
}

void printItem(char* name,char * item,int len){
	int i=0;
	printf("%s ",name );
	while(i<len){
		printf("%d", item[i++] );
	}
	printf("\n");
}

void readRegisterFile(){
	int  lSize;
	char * buffer;

	lSize = getFileSize(TEST_REG_FILE);
	printf("sizeof file %d\n",lSize );

	buffer = freadFileToBuffer(TEST_REG_FILE, lSize);
	if(!buffer){
		printf("Read File Fail\n");
	}
	/* the whole file is now loaded in the memory buffer. */
	regPacket_t * packet = (regPacket_t *)buffer;

	printf("\n\nRead Register File\n");

	printItem("type",packet->type, TYPE_LEN);
	printItem("plantID",packet->plantID, PLANT_ID_LEN);
	printItem("reg",packet->success, REG_STATUS_LEN);

	// terminate
	free (buffer);
	return ;
}

void readTodayFile(){
	int  lSize;
	char * buffer;

	lSize = getFileSize(TEST_DAY_FILE);
	printf("sizeof file %d\n",lSize );

	buffer = freadFileToBuffer(TEST_DAY_FILE, lSize);
	if(!buffer){
		printf("Read File Fail\n");
	}
	/* the whole file is now loaded in the memory buffer. */
	dailyPacket_t * packet = (dailyPacket_t *)buffer;

	printf("\n\nRead Today File\n");

	printItem("type",packet->type, TYPE_LEN);
	printItem("scheduleID",packet->scheduleID, SCH_ID_LEN);
	printItem("plantID",packet->plantID, PLANT_ID_LEN);
	printItem("time",packet->time, TIME_LEN);
	printItem("data num",packet->dataNum, DATA_NUM_LEN);
	printItem("data", packet->data, DAY_DATA_LEN);
	printItem("extra", packet->extra, EXTRA_BIT);
	printItem("checksum", packet->checksum, CHECKSUM_LEN);
	printItem("nextAccessTime", packet->nextTime, NEXT_TIME_LEN);

	// terminate
	free (buffer);
	return ;
}



void readMonthFile(){
	int  lSize;
	char * buffer;

	lSize = getFileSize(TEST_MONTH_FILE);
	printf("sizeof file %d\n",lSize );

	buffer = freadFileToBuffer(TEST_MONTH_FILE, lSize);
	if(!buffer){
		printf("Read File Fail\n");
	}
	/* the whole file is now loaded in the memory buffer. */
	monthPacketUpper_t * packet = (monthPacketUpper_t *)buffer;


	printf("\n\nRead Month File\n");
	printItem("type",packet->type, TYPE_LEN);
	printItem("scheduleID",packet->scheduleID, SCH_ID_LEN);
	printItem("plantID",packet->plantID, PLANT_ID_LEN);
	printItem("time",packet->time, TIME_LEN);
	printItem("data num",packet->dataNum, DATA_NUM_LEN);

	int num = 0;
	int i =0;
	for(i=0; i<DATA_NUM_LEN; i++){
		num = num*10 + packet->dataNum[i];
	}


	printItem("data", packet->data, num);
	printItem("checksum", packet->data + num, CHECKSUM_LEN);

	// terminate
	free (buffer);
}
