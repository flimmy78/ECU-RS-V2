/*****************************************************************************/
/* File      : variation.h                                                   */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2017-06-02 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/
#ifndef __VARIATION_H__
#define __VARIATION_H__

/*****************************************************************************/
/*  Definitions                                                              */
/*****************************************************************************/
#define MAXINVERTERCOUNT 						120	//最大的逆变器数
#define INVERTERLENGTH 							56	//最大的逆变器数  //与手机通讯

#define RECORDLENGTH                            150	//子记录的长度
#define RECORDTAIL                              100	//大记录的结尾，包括发电量、时间等信息

//Client 相关通信参数
#define CLIENT_RECORD_HEAD						20
#define CLIENT_RECORD_ECU_HEAD					78
#define CLIENT_RECORD_INVERTER_LENGTH           104
#define CLIENT_RECORD_OTHER						100
#define CLIENT_RECORD_JSON						10000

#define CONTROL_RECORD_HEAD						18
#define CONTROL_RECORD_ECU_HEAD					33
#define CONTROL_RECORD_INVERTER_LENGTH          41
#define CONTROL_RECORD_OTHER                    100


#define CONTROL_RECORD_ALARM_ECU_HEAD           (14*99+36)

#define JSON_RECORD_HEAD						200
#define JSON_RECORD_PER_INFO					200

#define MAX_THIRD_INVERTER_COUNT                20 //最大第三方逆变器数量
#define MAX_THIRD_INVERTERID_LEN                32 //最大第三方逆变器数量
#define MAX_CHANNEL_NUM 16         //最大通道数
#define MAX_FACTORY_LEN 10         //最大工厂名称长度
#define MAX_TYPE_LEN 10            //最大逆变器型号长度
#define MAX_PV_NUM 	6
#define MAX_AC_NUM	3

#pragma pack(push)  
#pragma pack(1) 

typedef enum
{ 
    DEVICE_UNKNOWN    	= 0,		//未知设备
    DEVICE_OPT700 		= 1,		//优化器
    DEVICE_OPT700_RS  	= 2,		//关断器
    DEVICE_JBOX 		= 3		//Jbox
} eDeviceType;

typedef struct
{
    unsigned short comm_failed3_status:1;			//通讯状态 :  1 正常通讯   0 连续三次通讯不上
    unsigned short function_status:1;				//RSD功能开关状态: 1 RSD功能开    0 RSD功能 关
    unsigned short pv1_low_voltage_pritection:1;	// PV1欠压保护(由优化器自动上报)
    unsigned short pv2_low_voltage_pritection:1;	// PV2欠压保护(由优化器自动上报)
    unsigned short device_Type:2;				//设备类型  0:开关设备 1；监控设备
    unsigned short comm_status:1;				//1表示读到当前数据；0表示读取数据失败
    unsigned short bindflag:1;					//逆变器绑定短地址标志，1表示绑定，0表示未绑定
    unsigned short flag:1;						//id中的flag标志
    unsigned char turn_on_off_flag:1;				//当前读取到的开关机状态  0:关机 1:开机
    unsigned short alarm_flag:1;					//是否需要上报告警标志位
    unsigned short collect_ret:1;
    unsigned short turn_on_collect_data:1;			//开机时，文件中是否有对应RSD数据(数据为上一轮关机前存储数据)
    unsigned char pv2_alarm_status:2	;			//ECU检测到OPT告警状态0: 无报警 1:PV2输入电压大于100V   2:PV2输入电压为0V
    unsigned char Reserve1:1; 
}status_t;

typedef struct
{
    unsigned char rsd_config_status:1;			//RSD配置状态: 1 RSD功能使能   0 RSD功能禁能
    unsigned char Reserve1:7;					//保留
}config_status_t;

typedef struct
{
    unsigned short  Version_Control_flag:1;	//版本控制位
    unsigned short  Reserve1:2;	//保留位
    unsigned short  Energy2_flag:1;	//输入2发电量标志位
    unsigned short  Energy1_flag:1;	//输入1发电量标志位
    unsigned short  EnergyOut_flag:1;	//输出发电量标志位
    unsigned short  Temperature_flag:1;	//温度标志位
    unsigned short  Current2_flag:1;	//PV2输入电流
    unsigned short  Current1_flag:1;	//PV1输入电流
    unsigned short  CurrentOut_flag:1;	//输出电流
    unsigned short  Power2_flag:1;	//PV2输入功率
    unsigned short  Power1_flag:1;	//PV1输入功率
    unsigned short  PowerOut_flag:1;	//输出功率
    unsigned short  Voltage2_flag:1;	//PV2输入电压
    unsigned short  Voltage1_flag:1;	//PV1输入电压
    unsigned short  VoltageOut_flag:1;	//输出电压
}parameter_status_t;

typedef struct inverter_info_t{
    char uid[13];				//逆变器ID（在通讯的时候转换为BCD编码）
    unsigned short shortaddr;		//Zigbee的短地址
    unsigned char model;			//机型：00未知 
    							//                01优化器 
    							//                02 关断器 
    							//                03JBOX
    int zigbee_version;			//zigbee版本号ZK

    unsigned short version;		//软件版本号
    unsigned short heart_rate;	//心跳次数
    unsigned short off_times;		//心跳超时次数
    status_t status;			//部分状态信息
    parameter_status_t parameter_status;	//RSD相关产品传送过来的状态
    config_status_t config_status;	//配置状态信息
    unsigned char restartNum;		//一天内的重启次数

    unsigned char  temperature; 	//设备内部温度 -100度为真实温度

    unsigned short PV1;			//PV1输入电压  精度 0.1V
    unsigned short PV2;			//PV2输入电压  精度 0.1V
    unsigned short PI;			//输入电流 	精度0.1A
    unsigned short PI2;			//输入电流 	精度0.1A

    unsigned short PV_Output; 	//输出电压 精度0.1V
    unsigned short PI_Output; 	//输出电流 	精度0.1A
    unsigned short Power1;		//PV1输入功率  精度0.1W
    unsigned short Power2;		//PV2输入功率  精度0.1W
    unsigned short Power_Output;	//输出功率  精度0.1W
    unsigned char RSSI;			//信号强度
    unsigned int PV1_Energy;		//当前一轮PV1发电量	精度 1焦耳
    unsigned int PV2_Energy;		//当前一轮PV2发电量	精度 1焦耳
    unsigned int PV_Output_Energy;//当前一轮PV2发电量	精度 1焦耳
    unsigned char Mos_CloseNum;	//设备上电后MOS管关断次数
    unsigned char RSDTimeout;			//RSD超时时间
    unsigned short PV1_low_voltageNUM;	//PV1欠压次数
    unsigned short PV2_low_voltageNUM;	//PV2欠压次数
    char LastCommTime[15];			//RSD最后一次通讯上的时间

    //上一轮相关的数据，这里的上一轮指的是5分钟一轮
    char LastCollectTime[15];	//上一轮采集时，最后一次通讯时间
    unsigned int Last_PV1_Energy;//上一轮PV1发电量 指的是5分钟前的一轮
    unsigned int Last_PV2_Energy;//上一轮PV2发电量 指的是5分钟前的一轮
    unsigned int Last_PV_Output_Energy;//上一轮PV2发电量 指的是5分钟前的一轮
    double AveragePower1; 			//5分钟平均功率1
    double AveragePower2; 			//5分钟平均功率2
    double AveragePower_Output; 		//5分钟平均功率2
    unsigned int EnergyPV1;			//当前一轮电量	精度 1焦耳
    unsigned int EnergyPV2;			//当前一轮电量	精度 1焦耳
    unsigned int EnergyPV_Output;		//当前一轮电量	精度 1焦耳
    unsigned char no_getdata_num;	//unsigned char(保持上限255)连续没有获取到逆变器数据的次数

}inverter_info;

typedef struct ecu_info_t{
    char ECUID12[13];
    char ECUID6[7];
    unsigned short panid;				//Zigbee的panid
    char channel;				//Zigbee信道
    char Signal_Level;		//信号强度
    char IO_Init_Status;	//IO初始状态	'1'是使能  '0‘是禁能
    int count;					//系统当前一轮有数据的逆变器数
    int validNum;			//当前有效台数
    int curSequence;		//采集轮训机器号
    int curHeartSequence;   //心跳轮训机器号
    char MacAddress[7];			//ECU  MAC地址
    float life_energy;			//系统历史总电量
    float current_energy;		//系统当前一轮电量
    float today_energy;			//当天的发电量
    int system_power;			//系统总功率
    int lastCommNum;
    char curTime[15];			//最近一次采集的时间
    char JsonTime[15];			//最近一次采集的时间
    unsigned char flag_ten_clock_getshortaddr;	//每天10点有没有重新获取短地址标志
    int polling_total_times;			//ECU一天之中总的轮询次数 ZK
    unsigned char idUpdateFlag;		//id更新标志
    unsigned char ThirdIDUpdateFlag;		//第三方逆变器id更新标志
    int thirdCommNum;
    int thirdCount;		//第三方逆变器总台数
    
    unsigned short abnormalNum;			//出现异常的台数
    unsigned int haveDataTimes;			//有数据的轮数
    unsigned char faulttimes;			//故障次数
    unsigned int nextdetectionTimes;		//下一次检测次数
    unsigned int overdetectionTimes;		//最后一次检测次数
}ecu_info;

typedef struct
{
    unsigned char inverter_addr_flag:1;			//逆变器是否已绑定地址 1：是  0：否
    unsigned char autoget_addr:1;               			//是否自动获取地址  1：是  0：否
    unsigned char communication_flag:1;			//当前一轮通讯标志 0:未通讯上 1已经通讯上
    unsigned char Reserve1:5;				//保留
}inverter_third_status_t;




//第三方逆变器结构题
typedef struct inverter_third_info_t{
    char inverterid[MAX_THIRD_INVERTERID_LEN];//逆变器ID 最大长度是32	OK
    unsigned char  inverter_addr;             //逆变器地址       Modbus从机地址
    char factory[MAX_FACTORY_LEN];            //逆变器厂家名称
    char type[MAX_TYPE_LEN];                  //逆变器型号						OK
    char cBaudrate;                           //波特率       
    inverter_third_status_t third_status;     //各种状态


    float PV_Voltage[MAX_PV_NUM];                       //直流电压		最多6路
    float PV_Current[MAX_PV_NUM];                       //直流电流		最多6路
    float PV_Power[MAX_PV_NUM];                       //直流功率		最多6路
    float AC_Voltage[MAX_AC_NUM];                       //交流电压		3相电压
    float AC_Current[MAX_AC_NUM];                       //交流电流		3相电压
    float Grid_Frequency[MAX_AC_NUM];                      //电网频率
    float Temperature;                           //机内温度
    int Reactive_Power;                        //无功功率
    int Active_Power;                          //有功功率
    float Power_Factor;                        //功率因数
    float Daily_Energy;                        //日发电量
    float Life_Energy;                         //历史发电量
    float Current_Energy;                      //本轮发电量	//日发电量计算  
    int (*GetData_ThirdInverter)(struct inverter_third_info_t *curThirdinverter);
}inverter_third_info;


#pragma pack(pop) 

#endif /*__VARIATION_H__*/
