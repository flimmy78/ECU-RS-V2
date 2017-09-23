/*****************************************************************************/
/* File      : event.c                                                       */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-06-09 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include "event.h"
#include "RFM300H.h"
#include "file.h"
#include "string.h"
#include "usart5.h"
#include "appcomm.h"
#include "SEGGER_RTT.h"
#include "inverter.h"
#include "rthw.h"
#include "led.h"
#include "serverfile.h"
#include "version.h"
#include "debug.h"
#include "AppFunction.h"

/*****************************************************************************/
/*  Definitions                                                              */
/*****************************************************************************/
#define PERIOD_NUM			3600

/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/
unsigned short comm_failed_Num = 0;
unsigned short pre_heart_rate;
extern rt_mutex_t wifi_uart_lock;
int Data_Len = 0,Command_Id = 0;
int ResolveFlag = 0;


//测试枚举
typedef enum
{ 
    HARDTEST_TEST_ALL    	= 0,		//接收数据头
    HARDTEST_TEST_EEPROM  = 1,	//接收数据长度   其中数据部分的长度为接收到长度减去12个字节
    HARDTEST_TEST_WIFI  	= 2,	//接收数据部分数据
    HARDTEST_TEST_433  		= 3
} eHardWareID;// receive state machin

enum CommandID{
	P00, P01, P02, P03, P04, P05, P06, P07, P08, P09, //0-9
	P10, P11, P12, P13, P14, P15, P16, P17, P18, P19, //10-19
	P20, P21, P22, P23, P24, P25, P26, P27, P28, P29, //20-29
	P30, P31, P32, P33, P34, P35, P36, P37, P38, P39, //30-39
	P40, P41, P42, P43, P44, P45, P46, P47, P48, P49, //40-49
	P50, P51, P52, P53, P54, P55, P56, P57, P58, P59, //50-59
};

void (*pfun_Phone[100])(unsigned char * id,int DataLen,const char *recvbuffer);

void add_APP_functions(void)
{
	pfun_Phone[P01] = App_GetBaseInfo; 				//获取基本信息请求
	pfun_Phone[P02] = App_GetSystemInfo; 				//获取系统信息
	pfun_Phone[P03] = App_SetNetwork; 				//设置组网
	pfun_Phone[P04] = App_SetChannel; 				//设置信道
	pfun_Phone[P05] = App_SetWIFIPasswd; 			//设置WIFI密码
	pfun_Phone[P06] = App_SetIOInitStatus; 			//设置IO初始状态
	pfun_Phone[P07] = APP_GetRSDHistoryInfo; 		//功率电流电压曲线
	pfun_Phone[P08] = App_GetGenerationCurve; 				//发电量曲线请求
	pfun_Phone[P09] = App_SetWiredNetwork; 	//有线网络设置
	pfun_Phone[P10] = App_GetWiredNetwork; 		//获取有线网络设置
	pfun_Phone[P11] = App_GetFlashSize; 			//获取Flash剩余空间
	pfun_Phone[P12] = App_GetPowerCurve; 			//获取Flash剩余空间


}


/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/
//WIFI事件处理
void process_WIFI(unsigned char * ID)
{
	ResolveFlag =  Resolve_RecvData((char *)WIFI_RecvSocketAData,&Data_Len,&Command_Id);
	if(ResolveFlag == 0)
	{
		
		if(pfun_Phone[Command_Id%100])
		{
			printf("pfun_Phone ID:%d\n",Command_Id);
			(*pfun_Phone[Command_Id%100])(ID,Data_Len,(char *)WIFI_RecvSocketAData);
		}
		
	}

}

//硬件测试   返回值是测试的错误码
int HardwareTest(char testItem)
{
	char testWrite[10] = "YUNENG APS";
	char testRead[10] = {'\0'};

	switch(testItem)
	{
		case HARDTEST_TEST_ALL:
			//测试EEPROM读写
			Write_Test(testWrite,10);				//测试
			Read_Test(testRead,10);
			if(memcmp(testWrite,testRead,10))
			{
				printf("EEPROM abnormal\n");
				return 1;
			}
			//测试WIFI模块
			if(0 != WIFI_Test())
			{
				printf("WIFI abnormal\n");
				return 2;
			}
			
			//测试433模块

			
			break;
		
		case HARDTEST_TEST_EEPROM:
			//测试EEPROM读写
			break;
		
		case HARDTEST_TEST_WIFI:
			//测试WIFI模块
			if(0 != WIFI_Test())
			{
				return 2;
			}
			break;
		
		case HARDTEST_TEST_433:
			//测试433模块

			break;
				
	}
	return 0;
}


void process_WIFIEvent(void)
{
	rt_mutex_take(wifi_uart_lock, RT_WAITING_FOREVER);
	//检测WIFI事件
	WIFI_GetEvent();
	rt_mutex_release(wifi_uart_lock);
	//判断是否有WIFI接收事件
	if(WIFI_Recv_SocketA_Event == 1)
	{
		SEGGER_RTT_printf(0,"WIFI_Recv_Event start\n");
		process_WIFI(ID_A);
		WIFI_Recv_SocketA_Event = 0;
		SEGGER_RTT_printf(0,"WIFI_Recv_Event end\n");
	}

}


//心跳事件处理
void process_HeartBeatEvent(void)
{
	int ret = 0;
	//发送心跳包
	if(	ecu.validNum >0	)
	{
		if(ecu.curSequence >= ecu.validNum)		//当轮训的序号大于最后一台时，更换到第0台
		{
			ecu.curSequence = 0;
		}
		
		//先保存上一轮的心跳
		pre_heart_rate = inverterInfo[ecu.curSequence].heart_rate;
		ret = RFM300_Heart_Beat(ecu.ECUID6,&inverterInfo[ecu.curSequence]);
	
		if(ret == 0)	//发送心跳包失败
		{
			//查看绑定标志位，如果绑定未成功，尝试绑定。
			if(inverterInfo[ecu.curSequence].status.bind_status != 1)
			{
				ret = RFM300_Bind_Uid(ecu.ECUID6,(char *)inverterInfo[ecu.curSequence].uid,0,0,&ecu.ver);
				
				if(ret == 1)
				{
					if(Write_UID_Bind(0x01,(ecu.curSequence+1)) == 0)
					{
						inverterInfo[ecu.curSequence].status.bind_status = 1;
						inverterInfo[ecu.curSequence].status.heart_Failed_times = 0;
					}
					
				}
			}
			inverterInfo[ecu.curSequence].status.heart_Failed_times++;
			if(inverterInfo[ecu.curSequence].status.heart_Failed_times >= 3)
			{
				inverterInfo[ecu.curSequence].status.heart_Failed_times = 3;		
				inverterInfo[ecu.curSequence].status.comm_failed3_status = 0;
			}
				
			//通信失败，失败次数++
			comm_failed_Num ++;
		}else	//发送心跳包成功
		{
			//通信失败，失败次数++
			comm_failed_Num  = 0;
			inverterInfo[ecu.curSequence].status.heart_Failed_times = 0;
			//心跳成功  判断当前系统信道和当前RSD2最后一次通信的信道是否不同   不同则更改之
			if(ecu.Channel_char != inverterInfo[ecu.curSequence].channel)
			{
					if(Write_UID_Channel(ecu.Channel_char,(ecu.curSequence+1)) == 0)
					{
						SEGGER_RTT_printf(0, "change Channel %02x%02x%02x%02x%02x%02x  Channel_char:%d   inverterInfo[curSequence].channel:%d\n",inverterInfo[ecu.curSequence].uid[0],inverterInfo[ecu.curSequence].uid[1],inverterInfo[ecu.curSequence].uid[2],inverterInfo[ecu.curSequence].uid[3],inverterInfo[ecu.curSequence].uid[4],inverterInfo[ecu.curSequence].uid[5],ecu.Channel_char,inverterInfo[ecu.curSequence].channel);

						inverterInfo[ecu.curSequence].channel = ecu.Channel_char;
					}
			}

			//如果当前一轮心跳小于上一轮心跳,表示重启
			if(inverterInfo[ecu.curSequence].heart_rate < pre_heart_rate)
			{
				//当前一轮重启次数+1
				if(inverterInfo[ecu.curSequence].restartNum < 255)
					inverterInfo[ecu.curSequence].restartNum++;
			}
			
		}

		
		//在绑定成功的情况下才能改信道
		//if(inverterInfo[curSequence].bind_status == 1)
		{
			//查看是否需要改变信道
			if(ecu.Channel_char != inverterInfo[ecu.curSequence].channel)
			{
				//先变更到RSD2的信道 
				setChannel(inverterInfo[ecu.curSequence].channel);
				
				//发送更改信道报文
				ret = RFM300_Bind_Uid(ecu.ECUID6,(char *)inverterInfo[ecu.curSequence].uid,ecu.Channel_char,0,&ecu.ver);
				if(ret == 1)	//设置信道成功
				{
					if(Write_UID_Channel(ecu.Channel_char,(ecu.curSequence+1)) == 0)
					{
						SEGGER_RTT_printf(0, "change Channel %02x%02x%02x%02x%02x%02x  Channel_char:%d   inverterInfo[curSequence].channel:%d\n",inverterInfo[ecu.curSequence].uid[0],inverterInfo[ecu.curSequence].uid[1],inverterInfo[ecu.curSequence].uid[2],inverterInfo[ecu.curSequence].uid[3],inverterInfo[ecu.curSequence].uid[4],inverterInfo[ecu.curSequence].uid[5],ecu.Channel_char,inverterInfo[ecu.curSequence].channel);
						inverterInfo[ecu.curSequence].channel = ecu.Channel_char;
					}
				}
				
				//更改到系统信道
				setChannel(ecu.Channel_char);
			}	

			//心跳成功 判断是否需要关闭或者打开心跳功能
			if(inverterInfo[ecu.curSequence].status.function_status == 1)	//心跳功能打开
			{
				if(ecu.IO_Init_Status == '0')		//需要关闭心跳功能
				{
					printf("ID: %02x%02x%02x%02x%02x%02x   IO_Init_Status 0\n",inverterInfo[ecu.curSequence].uid[0],inverterInfo[ecu.curSequence].uid[1],inverterInfo[ecu.curSequence].uid[2],inverterInfo[ecu.curSequence].uid[3],inverterInfo[ecu.curSequence].uid[4],inverterInfo[ecu.curSequence].uid[5]);
					RFM300_Status_Init(ecu.ECUID6,(char *)inverterInfo[ecu.curSequence].uid,0x02,0x00,&inverterInfo[ecu.curSequence].status);
				}
			}else					//心跳功能关闭
			{
				if(ecu.IO_Init_Status == '1')		//需要打开心跳功能
				{
					printf("ID: %02x%02x%02x%02x%02x%02x   IO_Init_Status 1\n",inverterInfo[ecu.curSequence].uid[0],inverterInfo[ecu.curSequence].uid[1],inverterInfo[ecu.curSequence].uid[2],inverterInfo[ecu.curSequence].uid[3],inverterInfo[ecu.curSequence].uid[4],inverterInfo[ecu.curSequence].uid[5]);
					RFM300_Status_Init(ecu.ECUID6,(char *)inverterInfo[ecu.curSequence].uid,0x01,0x00,&inverterInfo[ecu.curSequence].status);
				}
			}
		}
		
		ecu.curSequence++;
		
		
		//连续通讯不上1小时   表示关机状态
		if(comm_failed_Num >= PERIOD_NUM)
		{
			//SEGGER_RTT_printf(0, "comm_failed_Num:%d   \n",comm_failed_Num);
			for(ecu.curSequence = 0;ecu.curSequence < ecu.validNum;ecu.curSequence++)
			{
				inverterInfo[ecu.curSequence].restartNum = 0;
			}
			comm_failed_Num = 0;
			ecu.curSequence = 0;
		}
		
				
	}
}

//按键初始化密码事件处理
void process_KEYEvent(void)
{
	int ret =0,i = 0;
	SEGGER_RTT_printf(0, "KEY_FormatWIFI_Event Start\n");
		rt_hw_ms_delay(5000);


	for(i = 0;i<3;i++)
	{
		
		ret = WIFI_Factory(ecu.ECUID12);
		if(ret == 0) break;
	}
	
	if(ret == 0) 	//写入WIFI密码
		Write_WIFI_PW("88888888",8);	//WIFI密码	
	
	SEGGER_RTT_printf(0, "KEY_FormatWIFI_Event End\n");
}

//无线复位处理
int process_WIFI_RST(void)
{
	int ret =1,i = 0;
	SEGGER_RTT_printf(0, "process_WIFI_RST Start\n");
	for(i = 0;i<3;i++)
	{
		ret = WIFI_SoftReset();
		if(ret == 0) break;
	}
	SEGGER_RTT_printf(0, "process_WIFI_RST End\n");
	return ret;
}



#ifdef RT_USING_FINSH
#include <finsh.h>
int setECUID(char *ECUID)
{
	char USART1_ECUID12[13] = {'\0'};
	char USART1_ECUID6[7] = {'\0'};
	char Channel[2] = "01";
	char ret =0;
	if(strlen(ECUID) != 12)
	{
		printf("ECU ID Length misMatching\n");
		return -1;
	}
	memcpy(USART1_ECUID12,ECUID,12);
	USART1_ECUID6[0] = ((USART1_ECUID12[0]-'0')<<4) + (USART1_ECUID12[1]-'0') ;
	USART1_ECUID6[1] = ((USART1_ECUID12[2]-'0')<<4) + (USART1_ECUID12[3]-'0') ;
	USART1_ECUID6[2] = ((USART1_ECUID12[3]-'0')<<4) + (USART1_ECUID12[5]-'0') ;
	USART1_ECUID6[3] = ((USART1_ECUID12[4]-'0')<<4) + (USART1_ECUID12[7]-'0') ;
	USART1_ECUID6[4] = ((USART1_ECUID12[5]-'0')<<4) + (USART1_ECUID12[9]-'0') ;
	USART1_ECUID6[5] = ((USART1_ECUID12[10]-'0')<<4) + (USART1_ECUID12[11]-'0') ;
	ret = Write_ECUID(USART1_ECUID6);													  		//ECU ID
	//设置WIFI密码
	if(ret != 0) 	
	{
		printf("ECU ID Write EEPROM Failed\n");
		return -1;
	}
	Write_CHANNEL(Channel);
	Write_rebootNum(0);
	ecu.IO_Init_Status= '1';
	Write_IO_INIT_STATU(&ecu.IO_Init_Status);
	//设置WIFI密码
	USART1_ECUID12[12] = '\0';
	rt_hw_ms_delay(5000);
	ret = WIFI_Factory(USART1_ECUID12);
	//写入WIFI密码
	Write_WIFI_PW("88888888",8);	//WIFI密码		
	init_ecu();
	return 0;
	
}
FINSH_FUNCTION_EXPORT(setECUID, eg:set ECU ID("247000000001"));


int ReadECUID(void)
{
	char USART1_ECUID12[13] = {'\0'};
	char USART1_ECUID6[7] = {'\0'};
	
	Read_ECUID(USART1_ECUID6);
	transformECUID(USART1_ECUID6,USART1_ECUID12);		//转换ECU ID
	USART1_ECUID12[12] = '\0';
	
	if(USART1_ECUID6[0] == 0xff)
	{
		printf("undesirable ID\n");
		return -1;
	}else
	{
		printf("ID : %s\n",USART1_ECUID12);
		return 0;
	}
}
FINSH_FUNCTION_EXPORT(ReadECUID, eg:Read ECU ID);

int Test(void)
{
	HardwareTest(0);
	return 0;
}
FINSH_FUNCTION_EXPORT(Test, eg:Test Hardware);

int SetNetwork(char *OPT700_RS_ID)
{
	int AddNum = 1;
	char OPT700_RS_ID_list[7] = {'\0'};
	if(strlen(OPT700_RS_ID) != 12)
	{
		printf("OPT700-RS ID Length misMatching\n");
		return -1;
	}
	//将数据写入EEPROM
	OPT700_RS_ID_list[0] = (OPT700_RS_ID[0] - '0') * 0x10 + (OPT700_RS_ID[1] - '0');
	OPT700_RS_ID_list[1] = (OPT700_RS_ID[2] - '0') * 0x10 + (OPT700_RS_ID[3] - '0');
	OPT700_RS_ID_list[2] = (OPT700_RS_ID[4] - '0') * 0x10 + (OPT700_RS_ID[5] - '0');
	OPT700_RS_ID_list[3] = (OPT700_RS_ID[6] - '0') * 0x10 + (OPT700_RS_ID[7] - '0');
	OPT700_RS_ID_list[4] = (OPT700_RS_ID[8] - '0') * 0x10 + (OPT700_RS_ID[9] - '0');
	OPT700_RS_ID_list[5] = (OPT700_RS_ID[10] - '0') * 0x10 + (OPT700_RS_ID[11] - '0');
	add_inverter(inverterInfo,AddNum,(char *)&OPT700_RS_ID_list);
	return 0;
}
FINSH_FUNCTION_EXPORT(SetNetwork, eg:Set Network);

int FactoryStatus(void)
{
	char USART1_UID_NUM[2] = {0x00,0x00};
	Write_UID_NUM(USART1_UID_NUM);
	Write_rebootNum(0);
	init_inverter(inverterInfo);
	return 0;
}
FINSH_FUNCTION_EXPORT(FactoryStatus, eg:recover Factory Status);

#endif



#ifdef RT_USING_FINSH
#include <finsh.h>
void baseInfo(void)
{
	int i = 0 , type = inverterInfo[0].status.device_Type;
	for(i=0; (i<MAXINVERTERCOUNT)&&(i < ecu.validNum); i++)
	{
		if(type != inverterInfo[i].status.device_Type)		//判断是否相等
		{
			type = 2;
			break;
		}
	}
	printf("\n");
	printf("************************************************************\n");
	printf("ECU ID : %s\n",ecu.ECUID12);
	printf("ECU Version :%s\n",VERSION_ECU_RS);
	printf("ECU Channel :%s\n",ecu.Signal_Channel);
	printf("ECU RSSI :%d\n",ecu.Signal_Level);
	printf("RSD Type :%d\n",type);
	printf("ECU software Version : %s_%s_%s\n",ECU_VERSION,MAJORVERSION,MINORVERSION);
	printf("************************************************************\n");
}
FINSH_FUNCTION_EXPORT(baseInfo, eg:baseInfo());

void systemInfo(void)
{
	int i = 0;
	inverter_info *curinverter = inverterInfo;
	printf("\n");
	printf("*****ID*******Type***OnOff**Function**PV1Flag**PV2Flag**HeartTime**Timeout**CloseTime*****PV1********PV2********PI*******P1*******P2********PV1EN*********PV2EN*****RSSI******MOSCL\n");
	for(i=0; (i<MAXINVERTERCOUNT)&&(i < ecu.validNum); i++)	
	{
		printf("%02x%02x%02x%02x%02x%02x ",curinverter->uid[0],curinverter->uid[1],curinverter->uid[2],curinverter->uid[3],curinverter->uid[4],curinverter->uid[5]);
		printf("  %1d ",curinverter->status.device_Type);
		printf("      %1d ",curinverter->status.comm_failed3_status);
		printf("      %1d ",curinverter->status.function_status);
		printf("       %1d ",curinverter->status.pv1_low_voltage_pritection);
		printf("       %1d ",curinverter->status.pv2_low_voltage_pritection);
		printf("       %5d ",curinverter->heart_rate);
		printf("   %5d ",curinverter->off_times);
		printf("  %5d ",curinverter->restartNum);
		printf("       %5d ",curinverter->PV1);
		printf("     %5d ",curinverter->PV2);
		printf("    %5d ",curinverter->PI);
		printf("    %5d ",curinverter->Power1);
		printf("   %5d ",curinverter->Power2);
		printf(" %10d ",curinverter->PV1_Energy);
		printf("   %10d ",curinverter->PV2_Energy);
		printf("   %5d ",curinverter->RSSI);
		printf("   %3d \n",curinverter->Mos_CloseNum);
		curinverter++;
	}
	printf("*****************************************************************************************************************************************\n");
}
FINSH_FUNCTION_EXPORT(systemInfo, eg:systemInfo());

#endif

