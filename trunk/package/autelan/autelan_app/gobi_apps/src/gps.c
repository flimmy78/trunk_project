#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdio.h>
#include <math.h>

#include <fcntl.h>

#include "qmi.h"
#include "qmi_client.h"
#include "qmi_util.h"
#include "qmi_i.h"

#include "location_service_v02.h"

#include "wncqmi.h"

#include "CommonDefs.h"

#ifdef GPS_SUPPORT
extern qmi_client_type gps_handle;

static qmiLocEventPositionReportIndMsgT_v02 g_pos_inf;


//#define GPS_PARSED_POS_OUTPUT

#define GPS_NMEA_OUTPUT


#ifdef GPS_PARSED_POS_OUTPUT
static char sLong[20];
static char sLongInd[1];
static char sLat[20];
static char sLatInd[1];
static char sHeit[20];
static char sHeitWrtElp[20];
static char sVeloc[20];
static char sOrt[20];
static char sUtc[20];
static char sUtcDate[6];
static char sSV[2];
static char sState[10];
static char sHorizon[20];


char* get_str_sect_n(const char *str,char *str_sect,int sect_n)
{
	int i=0,j=0,m=0,n=0;
	int found=0;
	
	if(str==NULL || str_sect==NULL) return;

	memset(str_sect,'\0',sizeof(str_sect));

	for(;i<strlen(str);i++)	{
		if(str[i]==','){
			if(j==sect_n){
				n=i-1;
				found=1;
				break;
			}
			j++;
			m=i+1;
		}
	}

	if(1==found){
		if(n>=m){
			strncpy(str_sect,(str+m),(n-m+1));
		}
	}

	WNC_DEBUG_MSG_3("string=%s section=%s n=%d\n",str,str_sect,sect_n);

	return str_sect;
}

int my_floor(float a)
{
     int b=0;
     if(a>=0)
        {b=(int)a;}
     else
        {b=(int)a-1;}

     return b;
}


char* ddmm2degree(char* ddmm, char* degree)
{
	float d,f;
	if(NULL==ddmm || NULL==degree) return degree;
	
	f=atof(ddmm);
	d=my_floor(f/100)+(f-100*my_floor(f/100))/60;
	sprintf(degree,"%f",d);
	return degree;
}
#endif


void gps_event_pos_report_ind(qmiLocEventPositionReportIndMsgT_v02 *pos)
{
/*	syslog(LOG_USER|LOG_INFO,"%s sessionId=%d,status=%d",__FUNCTION__,pos->sessionId,pos->sessionStatus);

	if(pos->sessionStatus!=eQMI_LOC_SESS_STATUS_SUCCESS_V02)
		return;

	syslog(LOG_USER|LOG_INFO,"latitude=%f",pos->latitude);
	syslog(LOG_USER|LOG_INFO,"longitude=%f",pos->longitude);	
	syslog(LOG_USER|LOG_INFO,"speed=%f m/s",pos->speedHorizontal);
	syslog(LOG_USER|LOG_INFO,"heading=%f",pos->heading);	
	syslog(LOG_USER|LOG_INFO,"altitude=%f m",pos->altitudeWrtMeanSeaLevel);
	syslog(LOG_USER|LOG_INFO,"technology=%x",pos->technologyMask);

	memcpy(&g_pos_inf,pos,sizeof(qmiLocEventPositionReportIndMsgT_v02));
*/
}

void gps_event_nmea_ind(char *nmea)
{
	WNC_DEBUG_MSG_1("=NMEA= %s\n",nmea);
	syslog(LOG_USER|LOG_INFO,"=NMEA= %s",nmea);

#ifdef GPS_NMEA_OUTPUT
	int fd=-1;
	char cmd[128];
	memset(cmd,'\0',sizeof(cmd)); 
	sprintf(cmd,"echo \"\\%s\" >> /var/run/nmea",nmea);
	system(cmd);

	system("ls -l /var/run/nmea|awk '{print $5}' > /var/run/nmeasize");
	fd = open("/var/run/nmeasize",O_RDONLY);
	if(fd != -1)
	{
		memset(cmd,'\0',sizeof(cmd)); 
		read(fd,cmd,20);
		if(atoi(cmd)>2048)
		{
			system("echo > /var/run/nmea");
		}
		close(fd);
	}
	remove("/var/run/nmeasize");
#endif

#ifdef GPS_PARSED_POS_OUTPUT

	if(0==strncmp(nmea,"$GPGGA",6)){
/*
����$GPGGA,092204.999,4250.5589,S,14718.5084,E,1,04,24.4,M,19.7,M,,,0000*1F
�ֶ�0��$GPGGA�����ID�����������ΪGlobal Positioning System Fix Data��GGA��GPS��λ��Ϣ
�ֶ�1��UTC ʱ�䣬hhmmss.sss��ʱ�����ʽ
�ֶ�2��γ��ddmm.mmmm���ȷָ�ʽ��ǰ��λ��������0��
�ֶ�3��γ��N����γ����S����γ��
�ֶ�4������dddmm.mmmm���ȷָ�ʽ��ǰ��λ��������0��
�ֶ�5������E����������W��������
�ֶ�6��GPS״̬��0=������(FIX NOT valid)��1=���㶨λ(GPS FIX)��2=��ֶ�λ(DGPS)��3=��ЧPPS��4=ʵʱ��ֶ�λ��RTK FIX����5=RTK FLOAT��6=���ڹ���
�ֶ�7������ʹ�õ�����������00 - 12����ǰ��λ��������0��
�ֶ�8��HDOPˮƽ�������ӣ�0.5 - 99.9��
�ֶ�9�����θ߶ȣ�-9999.9 - 99999.9��
�ֶ�10����λ��M���ף�
�ֶ�11��������������Դ��ˮ׼��ĸ߶� WGS84ˮ׼�滮��
�ֶ�12��WGS84ˮ׼�滮�ֵ�λ��M���ף�
�ֶ�13�����ʱ�䣨�ӽ��յ�����źſ�ʼ��������������ǲ�ֶ�λ��Ϊ�գ�
�ֶ�14�����վID��0000 - 1023��ǰ��λ��������0��������ǲ�ֶ�λ��Ϊ�գ�
�ֶ�15��У��ֵ
*/
		char tmp[20];

		get_str_sect_n(nmea,&sUtc,1);

		memset(tmp,'\0',sizeof(tmp));
		get_str_sect_n(nmea,tmp,2);
		ddmm2degree(tmp,sLat);

		get_str_sect_n(nmea,&sLatInd,3);

		memset(tmp,'\0',sizeof(tmp));
		get_str_sect_n(nmea,tmp,4);
		ddmm2degree(tmp,sLong);

		get_str_sect_n(nmea,&sLongInd,5);
		
		get_str_sect_n(nmea,&sHorizon,8);

		get_str_sect_n(nmea,&sHeit,9);

		get_str_sect_n(nmea,&sSV,7);

		memset(tmp,'\0',sizeof(tmp));
		//get_str_sect_n(nmea,tmp,11);
		//sprintf(sHeitWrtElp,"%s",atof(tmp)+atof(sHeit));

	}
	else if(0==strncmp(nmea,"$GPRMC",6)){
/*
����$GPRMC,024813.640,A,3158.4608,N,11848.3737,E,10.05,324.27,150706,,,A*50
�ֶ�0��$GPRMC�����ID�����������ΪRecommended Minimum Specific GPS/TRANSIT Data��RMC���Ƽ���С��λ��Ϣ
�ֶ�1��UTCʱ�䣬hhmmss.sss��ʽ
�ֶ�2��״̬��A=��λ��V=δ��λ
�ֶ�3��γ��ddmm.mmmm���ȷָ�ʽ��ǰ��λ��������0��
�ֶ�4��γ��N����γ����S����γ��
�ֶ�5������dddmm.mmmm���ȷָ�ʽ��ǰ��λ��������0��
�ֶ�6������E����������W��������
�ֶ�7���ٶȣ��ڣ�Knots
�ֶ�8����λ�ǣ���
�ֶ�9��UTC���ڣ�DDMMYY��ʽ
�ֶ�10����ƫ�ǣ���000 - 180���ȣ�ǰ��λ��������0��
�ֶ�11����ƫ�Ƿ���E=��W=��
�ֶ�16��У��ֵ
*/
		char tmp[1]="\0";
		get_str_sect_n(nmea,tmp,2);
		
		if(tmp[0]=='A' || tmp[0]=='a')
			strcpy(sState,"fixed");
		else
			strcpy(sState,"unfixed");	

		get_str_sect_n(nmea,&sUtcDate,9);

	}
	else if(0==strncmp(nmea,"$GPVTG",6)){
/*
����$GPVTG,89.68,T,,M,0.00,N,0.0,K*5F
�ֶ�0��$GPVTG�����ID�����������ΪTrack Made Good and Ground Speed��VTG�������ٶ���Ϣ
�ֶ�1���˶��Ƕȣ�000 - 359����ǰ��λ��������0��
�ֶ�2��T=�汱����ϵ
�ֶ�3���˶��Ƕȣ�000 - 359����ǰ��λ��������0��
�ֶ�4��M=�ű�����ϵ
�ֶ�5��ˮƽ�˶��ٶȣ�0.00����ǰ��λ��������0��
�ֶ�6��N=�ڣ�Knots
�ֶ�7��ˮƽ�˶��ٶȣ�0.00����ǰ��λ��������0��
�ֶ�8��K=����/ʱ��km/h
�ֶ�9��У��ֵ
*/
		get_str_sect_n(nmea,&sOrt,1);
		get_str_sect_n(nmea,&sVeloc,7);

	}

	if(strlen(sLong)!=0 ||strlen(sLat)!=0 || strlen(sHeit)!=0 || strlen(sVeloc)!=0){
		printf("GPS POS:%s,%s,%s%s,%s%s,%s,%s,%s,%s,%s\n",
			sUtcDate,sUtc,sLong,sLongInd,sLat,sLatInd,sHeit,sHorizon,sVeloc,sOrt,sSV);	
	}

#endif
}

int total_sv=0;
int total_good_sv=0;

void gps_event_sv_ind(qmiLocEventGnssSvInfoIndMsgT_v02 *ind)
{
	int i,sv=0,good_sv=0;
	
	if(ind==NULL) return;
	if(!ind->svList_valid) return;
			
	for(i=0;i<ind->svList_len;i++){
		if((ind->svList[i].validMask & QMI_LOC_SV_INFO_MASK_VALID_SYSTEM_V02)
			&&(ind->svList[i].validMask & QMI_LOC_SV_INFO_MASK_VALID_GNSS_SVID_V02)
			&&(ind->svList[i].validMask & QMI_LOC_SV_INFO_MASK_VALID_SNR_V02)){
			//GPS system
			if(ind->svList[i].system==1){
#ifdef debug
				printf("SVID %4d, SNR %8f, STAT %s\n",
					ind->svList[i].gnssSvId,
					ind->svList[i].snr,
					(3==ind->svList[i].svStatus)?"track":(2==ind->svList[i].svStatus)?"search":"idle");
#endif
				sv++;
				if(ind->svList[i].snr>30) good_sv++;
			}
		}
	}
	total_sv+=sv;
	total_good_sv+=good_sv;
#ifdef debug
	printf("SVs: %4d, Good SVs(SNR>30): %4d\n",sv,good_sv);
#endif
}

void gps_ind_handler(qmi_client_type user_handle, unsigned long msg_id,
	unsigned char *ind_buf, int ind_buf_len, void *ind_cb_data)
{
	WNC_DEBUG_MSG_2("Incoming Ind MSG %d %s\n",msg_id, __FUNCTION__);

	if (msg_id == QMI_LOC_EVENT_POSITION_REPORT_IND_V02) 
    {
		qmi_client_error_type rc;
		qmiLocEventPositionReportIndMsgT_v02 ind;

		WNC_DEBUG_MSG_0("Ind MSG: QMI_LOC_EVENT_POSITION_REPORT_IND_V02\n");

		memset(&ind, 0, sizeof(qmiLocEventPositionReportIndMsgT_v02));

		rc = qmi_client_message_decode(user_handle,
				QMI_IDL_INDICATION,
				msg_id,
				(void *)ind_buf,
				ind_buf_len,
				(void *)&ind,
				sizeof(qmiLocEventPositionReportIndMsgT_v02));

		if(rc!=0)
		{
			WNC_ERR_MSG_1("Decode Ind Msg ID Error: %d\n", msg_id);	
			syslog(LOG_USER|LOG_INFO,"%s decode msg_id=%d, error=%d",__FUNCTION__,msg_id,rc);
			return;
		}

		gps_event_pos_report_ind(&ind);
		goto done;
	}
	else if(msg_id == QMI_LOC_EVENT_NMEA_IND_V02)
	{
		WNC_DEBUG_MSG_0("Ind MSG: QMI_LOC_EVENT_NMEA_IND_V02\n");

		qmiLocEventNmeaIndMsgT_v02 ind;
		memset(&ind, 0, sizeof(qmiLocEventNmeaIndMsgT_v02));

		qmi_client_message_decode(user_handle,
				QMI_IDL_INDICATION,
				msg_id,
				(void *)ind_buf,
				ind_buf_len,
				(void *)&ind,
				sizeof(qmiLocEventNmeaIndMsgT_v02));

		gps_event_nmea_ind(ind.nmea);
		goto done;
	}
	else if(msg_id == QMI_LOC_EVENT_GNSS_SV_INFO_IND_V02)
	{
		qmi_client_error_type rc;
		qmiLocEventGnssSvInfoIndMsgT_v02 ind;
		WNC_DEBUG_MSG_0("Ind MSG: QMI_LOC_EVENT_GNSS_SV_INFO_IND_V02\n");

		memset(&ind, 0, sizeof(qmiLocEventGnssSvInfoIndMsgT_v02));

		rc = qmi_client_message_decode(user_handle,
				QMI_IDL_INDICATION,
				msg_id,
				(void *)ind_buf,
				ind_buf_len,
				(void *)&ind,
				sizeof(qmiLocEventGnssSvInfoIndMsgT_v02));

		if(rc!=0)
		{
			WNC_ERR_MSG_1("Decode Ind Msg ID Error: %d\n", msg_id);	
			syslog(LOG_USER|LOG_INFO,"%s decode msg_id=%d, error=%d",__FUNCTION__,msg_id,rc);
			return;
		}

		gps_event_sv_ind(&ind);
		goto done;
	}
	else if(msg_id==QMI_LOC_EVENT_ENGINE_STATE_IND_V02)
	{
		WNC_DEBUG_MSG_0("Ind MSG: QMI_LOC_EVENT_ENGINE_STATE_IND_V02\n");	

		qmiLocEventEngineStateIndMsgT_v02 ind;
		memset(&ind, 0, sizeof(qmiLocEventEngineStateIndMsgT_v02));

		qmi_client_message_decode(user_handle,
				QMI_IDL_INDICATION,
				msg_id,
				(void *)ind_buf,
				ind_buf_len,
				(void *)&ind,
				sizeof(qmiLocEventEngineStateIndMsgT_v02));

		if(ind.engineState==1){
			WNC_DEBUG_MSG_0("GPS ON\n");
			syslog(LOG_USER|LOG_INFO,"GPS ON");
		}else{
			WNC_DEBUG_MSG_0("GPS OFF\n");
			syslog(LOG_USER|LOG_INFO,"GPS OFF");	
		}
		goto done;
	}
	else if(msg_id==QMI_LOC_SET_OPERATION_MODE_IND_V02)
	{
		WNC_DEBUG_MSG_0("Ind MSG: QMI_LOC_SET_OPERATION_MODE_IND_V02\n");	

		qmiLocSetOperationModeIndMsgT_v02 ind;
		memset(&ind, 0, sizeof(qmiLocSetOperationModeIndMsgT_v02));

		qmi_client_message_decode(user_handle,
				QMI_IDL_INDICATION,
				msg_id,
				(void *)ind_buf,
				ind_buf_len,
				(void *)&ind,
				sizeof(qmiLocSetOperationModeIndMsgT_v02));

		if(ind.status==eQMI_LOC_SUCCESS_V02){
			WNC_DEBUG_MSG_0("GPS set operation mode ok\n");
		}else{
			WNC_DEBUG_MSG_1("GPS set operation mode fail:%d\n",ind.status);
			syslog(LOG_USER|LOG_INFO,"GPS set operation mode fail:%d\n",ind.status);	
		}
		goto done;
	}
	else if(msg_id==QMI_LOC_EVENT_FIX_SESSION_STATE_IND_V02)
	{
		WNC_DEBUG_MSG_0("Ind MSG: QMI_LOC_EVENT_FIX_SESSION_STATE_IND_V02\n");	

		qmiLocEventFixSessionStateIndMsgT_v02 ind;
		memset(&ind, 0, sizeof(qmiLocEventFixSessionStateIndMsgT_v02));

		qmi_client_message_decode(user_handle,
				QMI_IDL_INDICATION,
				msg_id,
				(void *)ind_buf,
				ind_buf_len,
				(void *)&ind,
				sizeof(qmiLocEventFixSessionStateIndMsgT_v02));

		WNC_DEBUG_MSG_1("//////////ind.sessionState:%d\n",ind.sessionState);
		WNC_DEBUG_MSG_1("//////////ind.sessionId_valid:%d\n",ind.sessionId_valid);
		WNC_DEBUG_MSG_1("//////////ind.sessionId:%d\n",ind.sessionId);

		goto done;
	}
	
	WNC_ERR_MSG_1("Unknow Ind Msg ID: %d\n", msg_id);	
	syslog(LOG_USER|LOG_INFO,"%s unknown msg_id=%d",__FUNCTION__,msg_id);
	return;

done:
	WNC_DEBUG_MSG_2("Ind MSG done. %d %s\n",msg_id, __FUNCTION__);
	return;
}

void gps_event_reg()
{
	qmiLocRegEventsReqMsgT_v02 req;
	qmiLocGenRespMsgT_v02 *res;
	int rc;
	
	res = malloc(sizeof(qmiLocGenRespMsgT_v02));
	memset(&req, 0, sizeof(qmiLocRegEventsReqMsgT_v02));
	memset(res, 0, sizeof(qmiLocGenRespMsgT_v02));
	
	//req.eventRegMask=QMI_LOC_EVENT_MASK_POSITION_REPORT_V02 | 
	//	QMI_LOC_EVENT_MASK_GNSS_SV_INFO_V02;

	req.eventRegMask=0xffffffffffffffff;
	
	rc=qmi_client_send_msg_sync(gps_handle,
		QMI_LOC_REG_EVENTS_REQ_V02,
		&req, 
		sizeof(qmiLocRegEventsReqMsgT_v02),
		res,
		sizeof(qmiLocGenRespMsgT_v02),
		qmi_sync_timeout);

	WNC_DEBUG_MSG_4("%s rc=%x result=%x error=%x\n", 
			__FUNCTION__,rc, res->resp.result, res->resp.error);

	if(rc!=0 || res->resp.result!=0)
	{
		syslog(LOG_USER|LOG_INFO,"%s rc=%x result=%x error=%x", 
			__FUNCTION__,rc, res->resp.result, res->resp.error);
	}
	
	free(res);
}

int initGPSResp()
{
	static int once = 1;
	if(!once) return 1;
	once = 0;

	WNC_DEBUG_MSG_1("%s ----****\n",__FUNCTION__);
	
	syslog(LOG_USER|LOG_INFO,"init Register GPS Event");
	
	gps_event_reg();
	return 0;
}

void gps_start_req()
{
	qmiLocStartReqMsgT_v02 req;
	qmiLocGenRespMsgT_v02 *res;
	int rc;
	
	res = malloc(sizeof(qmiLocGenRespMsgT_v02));
	memset(&req, 0, sizeof(qmiLocStartReqMsgT_v02));
	memset(res, 0, sizeof(qmiLocGenRespMsgT_v02));
	
	req.sessionId=0;
	req.fixRecurrence_valid=1;
	req.fixRecurrence=eQMI_LOC_RECURRENCE_PERIODIC_V02;
	req.horizontalAccuracyLevel_valid=1;
	req.horizontalAccuracyLevel=eQMI_LOC_ACCURACY_HIGH_V02;
	
	rc=qmi_client_send_msg_sync(gps_handle,
		QMI_LOC_START_REQ_V02,
		&req, 
		sizeof(qmiLocStartReqMsgT_v02),
		res,
		sizeof(qmiLocGenRespMsgT_v02),
		qmi_sync_timeout);

	WNC_DEBUG_MSG_4("%s rc=%x result=%x error=%x\n", 
			__FUNCTION__,rc, res->resp.result, res->resp.error);

	if(rc!=0 || res->resp.result!=0)
	{
		syslog(LOG_USER|LOG_INFO,"%s rc=%x result=%x error=%x", 
			__FUNCTION__,rc, res->resp.result, res->resp.error);
	}
	
	free(res);
}

void gps_stop_req()
{
	qmiLocStopReqMsgT_v02 req;
	qmiLocGenRespMsgT_v02 *res;
	int rc;
	
	res = malloc(sizeof(qmiLocGenRespMsgT_v02));
	memset(&req, 0, sizeof(qmiLocStopReqMsgT_v02));
	memset(res, 0, sizeof(qmiLocGenRespMsgT_v02));
	
	req.sessionId=0;
	
	rc=qmi_client_send_msg_sync(gps_handle,
		QMI_LOC_STOP_REQ_V02,
		&req, 
		sizeof(qmiLocStopReqMsgT_v02),
		res,
		sizeof(qmiLocGenRespMsgT_v02),
		qmi_sync_timeout);

	WNC_DEBUG_MSG_4("%s rc=%x result=%x error=%x\n", 
			__FUNCTION__,rc, res->resp.result, res->resp.error);

	if(rc!=0 || res->resp.result!=0)
	{
		syslog(LOG_USER|LOG_INFO,"%s rc=%x result=%x error=%x", 
			__FUNCTION__,rc, res->resp.result, res->resp.error);
	}
	
	free(res);
}

void gps_set_oper_mode_req(int mode)
{
	qmiLocSetOperationModeReqMsgT_v02 req;
	qmiLocGenRespMsgT_v02 *res;
	int rc;
	
	res = malloc(sizeof(qmiLocGenRespMsgT_v02));
	memset(&req, 0, sizeof(qmiLocSetOperationModeReqMsgT_v02));
	memset(res, 0, sizeof(qmiLocGenRespMsgT_v02));
	
//	req.operationMode=eQMI_LOC_OPER_MODE_STANDALONE_V02;
	req.operationMode=(qmiLocOperationModeEnumT_v02)mode;
	
	rc=qmi_client_send_msg_sync(gps_handle,
		QMI_LOC_SET_OPERATION_MODE_REQ_V02,
		&req, 
		sizeof(qmiLocSetOperationModeReqMsgT_v02),
		res,
		sizeof(qmiLocGenRespMsgT_v02),
		qmi_sync_timeout);
	
	WNC_DEBUG_MSG_1("GPS set operation mode:%d\n",req.operationMode);

	WNC_DEBUG_MSG_4("%s rc=%x result=%x error=%x\n", 
			__FUNCTION__,rc, res->resp.result, res->resp.error);

	if(rc!=0 || res->resp.result!=0)
	{
		syslog(LOG_USER|LOG_INFO,"%s rc=%x result=%x error=%x", 
			__FUNCTION__,rc, res->resp.result, res->resp.error);
	}
	
	free(res);	
}

#endif

