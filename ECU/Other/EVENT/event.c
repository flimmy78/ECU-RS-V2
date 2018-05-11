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
#include "serverfile.h"
#include "zigbee.h"
#include "rtc.h"
/*****************************************************************************/
/*  Definitions                                                              */
/*****************************************************************************/
#define PERIOD_NUM			3600

/*****************************************************************************/
/*  Variable Declarations                                                    */
/*****************************************************************************/

extern rt_mutex_t wifi_uart_lock;
int Data_Len = 0,Command_Id = 0;
int ResolveFlag = 0;
unsigned char APSTA_Status = 0;		//当前AP,STA模式   0表示STA模式，1表示STA+AP模式
static rt_timer_t APSTAtimer;	//线程重启定时器
unsigned char  switchSTAModeFlag = 0;	//切换为STA模式标志
//测试枚举
typedef enum
{ 
    HARDTEST_TEST_ALL    	= 0,		//接收数据头
    HARDTEST_TEST_EEPROM  = 1,	//接收数据长度   其中数据部分的长度为接收到长度减去12个字节
    HARDTEST_TEST_WIFI  	= 2,	//接收数据部分数据
    HARDTEST_TEST_433  		= 3
} eHardWareID;// receive state machin

enum CommandID{
	P0000, P0001, P0002, P0003, P0004, P0005, P0006, P0007, P0008, P0009, //0-9
	P0010, P0011, P0012, P0013, P0014, P0015, P0016, P0017, P0018, P0019, //10-19
	P0020, P0021, P0022, P0023, P0024, P0025, P0026, P0027, P0028, P0029, //20-29
	P0030, P0031, P0032, P0033, P0034, P0035, P0036, P0037, P0038, P0039, //30-39
	P0040, P0041, P0042, P0043, P0044, P0045, P0046, P0047, P0048, P0049, //40-49
	P0050, P0051, P0052, P0053, P0054, P0055, P0056, P0057, P0058, P0059, //50-59
};

void (*pfun_Phone[100])(int DataLen,const char *recvbuffer);

void add_APP_functions(void)
{
	pfun_Phone[P0001] = App_GetBaseInfo; 				//获取基本信息请求
	pfun_Phone[P0002] = App_GetSystemInfo; 				//获取系统信息
	pfun_Phone[P0003] = App_GetPowerCurve; 			//获取功率曲线
	pfun_Phone[P0004] = App_GetGenerationCurve; 				//发电量曲线请求
	pfun_Phone[P0005] = App_SetNetwork; 				//设置组网
	pfun_Phone[P0006] = App_SetTime; 			//ECU时间设置
	pfun_Phone[P0007] = App_SetWiredNetwork; 	//有线网络设置
	pfun_Phone[P0008] = App_GetECUHardwareStatus; 	//查看当前ECU硬件状态
	pfun_Phone[P0010] = App_SetWIFIPasswd; 			//设置WIFI密码
	pfun_Phone[P0011] = App_GetIDInfo; 			//获取ID信息
	pfun_Phone[P0012] = App_GetTime; 			//获取时间
	pfun_Phone[P0013] = App_GetFlashSize; 			//获取Flash剩余空间
	pfun_Phone[P0014] = App_GetWiredNetwork; 		//获取有线网络设置
	pfun_Phone[P0015] = App_SetChannel; 				//设置信道
	pfun_Phone[P0016] = App_SetIOInitStatus; 			//设置IO初始状态
	pfun_Phone[P0017] = APP_GetRSDHistoryInfo; 		//功率电流电压曲线
	pfun_Phone[P0018] = APP_GetShortAddrInfo;		//功率电流电压曲线
	pfun_Phone[P0020] = APP_GetECUAPInfo;		//功率电流电压曲线
	pfun_Phone[P0021] = APP_SetECUAPInfo;		//功率电流电压曲线
	pfun_Phone[P0022] = APP_ListECUAPInfo;		//功率电流电压曲线
	pfun_Phone[P0023] = APP_GetFunctionStatusInfo;
	pfun_Phone[P0024] = APP_ServerInfo;				//查看和设置相关服务器信息
	pfun_Phone[P0033] = APP_RegisterThirdInverter;	//设置和获取第三方逆变器
	pfun_Phone[P0034] = APP_GetThirdInverter;	//获取第三方逆变器数据
	pfun_Phone[P0035] = APP_TransmissionZigBeeInfo;	//透传ZigBee命令
}


/*****************************************************************************/
/*  Function Implementations                                                 */
/*****************************************************************************/
//WIFI事件处理
void process_WIFI(void)
{
	ResolveFlag =  Resolve_RecvData((char *)WIFI_RecvSocketAData,&Data_Len,&Command_Id);
	if(ResolveFlag == 0)
	{
		add_APP_functions();
		//函数指针不为空，则运行对应的函数
		if(pfun_Phone[Command_Id%100])
		{
			(*pfun_Phone[Command_Id%100])(Data_Len,(char *)WIFI_RecvSocketAData);
		}
		
	}

}


void process_WIFIEvent_ESP07S(void)
{
	rt_mutex_take(wifi_uart_lock, RT_WAITING_FOREVER);
	//检测WIFI事件
	WIFI_GetEvent_ESP07S();
	rt_mutex_release(wifi_uart_lock);
	//判断是否有WIFI接收事件
	if(WIFI_Recv_SocketA_Event == 1)
	{
		process_WIFI();
		WIFI_Recv_SocketA_Event = 0;

	}

}

//按键初始化密码事件处理
void process_KEYEvent(void)
{
	int ret =0,i = 0;
	
	for(i = 0;i<3;i++)
	{
		
		ret = WIFI_Factory_Passwd();
		if(ret == 0) break;
	}
	
	if(ret == 0) 	//写入WIFI密码
	{
		key_init();
		Write_WIFI_PW("88888888",8);	//WIFI密码	
	}
		
	
	SEGGER_RTT_printf(0, "KEY_FormatWIFI_Event End\n");
}

static void APSTATimeout(void* parameter)
{
	//切换为AP+STA模式
	APSTA_Status = 0;
	switchSTAModeFlag = 1;
}

void process_switchSTAMode(void)
{
	if(switchSTAModeFlag == 1)
	{
		printf("STA\n");
		AT_CWMODE3(1);
		AT_CIPMUX1();
		switchSTAModeFlag = 0;
	}
}
//切换为AP+STA模式，1小时后切换为STA模式
void process_APKEYEvent(void)
{
	//切换到AP+STA模式
	AT_CWMODE3(3);
	AT_CIPMUX1();
	AT_CIPSERVER();
	AT_CIPSTO();
	printf("AP\n");
	APSTA_Status = 1;

	 if(APSTAtimer == RT_NULL)
	 {
		//定时器1小时后，切换为STA模式
		APSTAtimer = rt_timer_create("APSTA",
			APSTATimeout, 
			RT_NULL, 
			3600*RT_TICK_PER_SECOND,
			RT_TIMER_FLAG_ONE_SHOT); 
	 }
	
	if (APSTAtimer != RT_NULL) 
	{
		rt_timer_stop(APSTAtimer);
		rt_timer_start(APSTAtimer);
	}
	
}

//无线复位处理,心跳失败才复位
int process_WIFI_RST(void)
{
	if(AT_RST() != 0)
	{
		WIFI_Reset();
	}

	if(APSTA_Status == 1)
	{
		AT_CWMODE3(3);
		AT_CIPMUX1();
		AT_CIPSERVER();
		AT_CIPSTO();
	}else
	{
		AT_CWMODE3(1);
		AT_CIPMUX1();
	}
	return 0;
}

int setECUID(char *ECUID)
{
	char ret =0;
	if(strlen(ECUID) != 12)
	{
		printf("ECU ID Length misMatching\n");
		return -1;
	}

	ret = Write_ECUID(ECUID);													  		//ECU ID
	//设置WIFI密码
	if(ret != 0) 	
	{
		printf("ECU ID Write EEPROM Failed\n");
		return -1;
	}
	echo("/config/channel.con","0x17");
	ecu.IO_Init_Status= '1';
	Write_IO_INIT_STATU(&ecu.IO_Init_Status);
	//设置WIFI密码
	ret = WIFI_Factory(ECUID);
	//写入WIFI密码
	Write_WIFI_PW("88888888",8);	//WIFI密码	
	
	init_ecu();
	return 0;
	
}


#ifdef RT_USING_FINSH
#include <finsh.h>

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
	printf("ECU Channel :%02x\n",ecu.channel);
	printf("ECU RSSI :%d\n",ecu.Signal_Level);
	printf("RSD Type :%d\n",type);
	printf("ECU software Version : %s_%s.%s\n",ECU_VERSION,MAJORVERSION,MINORVERSION);
	printf("************************************************************\n");
}
FINSH_FUNCTION_EXPORT(baseInfo, eg:baseInfo());

void systemInfo(void)
{
	int i = 0;
	inverter_info *curinverter = inverterInfo;
	printf("\n");
	printf("*****ID*****Type*OnOff*Function*PV1Flag*PV2Flag*HeartTime*Timeout*CloseTime***PV1******PV2******PI******P1*****P2******PV1EN*******PV2EN***RSSI****MOSCL*****EnergyPV1******EnergyPV2\n");
	for(i=0; (i<MAXINVERTERCOUNT)&&(i < ecu.validNum); i++)	
	{
		printf("%s ",curinverter->uid);
		printf("%1d ",curinverter->status.device_Type);
		printf("    %1d ",curinverter->status.comm_failed3_status);
		printf("     %1d ",curinverter->status.function_status);
		printf("      %1d ",curinverter->status.pv1_low_voltage_pritection);
		printf("      %1d ",curinverter->status.pv2_low_voltage_pritection);
		printf("      %5d ",curinverter->heart_rate);
		printf("  %5d ",curinverter->off_times);
		printf(" %5d ",curinverter->restartNum);
		printf("     %5u ",curinverter->PV1);
		printf("   %5u ",curinverter->PV2);
		printf("  %5u ",curinverter->PI);
		printf("  %5u ",curinverter->Power1);
		printf(" %5u ",curinverter->Power2);
		printf("%10u ",curinverter->PV1_Energy);
		printf(" %10u ",curinverter->PV2_Energy);
		printf(" %5d ",curinverter->RSSI);
		printf(" %10u ",curinverter->EnergyPV1);
		printf(" %10u ",curinverter->EnergyPV2);
		printf("     %3d \n",curinverter->Mos_CloseNum);
		curinverter++;
	}
	printf("*****************************************************************************************************************************************\n");
}
FINSH_FUNCTION_EXPORT(systemInfo, eg:systemInfo());

#endif


