/*****************************************************************************/
/* File      : third_inverter_SG.h                                           */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2018-04-24 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/
#ifndef __THIRD_INVERTER_SG__
#define __THIRD_INVERTER_SG__
/*****************************************************************************/
/*  Include Files                                                            */
/*****************************************************************************/
#include "variation.h"

/*****************************************************************************/
/*  Definitions                                                              */
/*****************************************************************************/

//�������
typedef enum 
{
    SG60KTL 	= 0x010f,
    SG60KU 		= 0x0136,
    SG33KTL_M = 0x0134,
    SG40KTL_M	= 0x0135,
    SG50KTL_M = 0x011B,
    SG60KTL_M = 0x0131,
    SG60KU_M 	= 0x0132,
    SG49K5J		= 0x0132,
} eSGDeviceType;

/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/
int GetData_ThirdInverter_SG(inverter_third_info *curThirdinverter);
#endif /*__THIRD_INVERTER_SG__*/
