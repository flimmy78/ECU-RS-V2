#ifndef __ZIGBEE_TRANSMISSION_H__
#define __ZIGBEE_TRANSMISSION_H__
/*****************************************************************************/
/* File      : ZigBeeTransmission.h                                          */
/*****************************************************************************/
/*  History:                                                                 */
/*****************************************************************************/
/*  Date       * Author          * Changes                                   */
/*****************************************************************************/
/*  2018-05-25 * Shengfeng Dong  * Creation of the file                      */
/*             *                 *                                           */
/*****************************************************************************/

/*****************************************************************************/
/*  Function Declarations                                                    */
/*****************************************************************************/
void process_ZigBeeTransimission(void);

int transmission_ZigBeeInfo(const char *recvbuffer, char *sendbuffer);
#endif	/*__ZIGBEE_TRANSMISSION_H__*/
