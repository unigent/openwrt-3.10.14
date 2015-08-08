/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2007
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/

/*******************************************************************************
 * Filename:
 * ---------
 *  mtk_app_service.h
 *
 * Project:
 * --------
 *
 * Description:
 * ------------
 *
 * Author:
 * -------
 *
 *==============================================================================
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *==============================================================================
 *******************************************************************************/
/*****************************************************************************
 * Include
 *****************************************************************************/

#ifndef _MTKNFCHO_SER_H_
#define _MTKNFCHO_SER_H_

#include "mtk_nfchod_main_sys.h"
#include "mtk_nfc_sys_type_ext.h"
#include "mtk_nfc_sys.h"
#include "mtk_app_service_type.h"

//#define SUPPORT_EM_CMD
#define DEFAULT_SUPPORT_HO
#define SUPPORT_I2C_IF


#define MTK_NFC_NDEFRECORD_TNF_EMPTY        (0x00)  /**< Empty Record, no type, ID or payload present. */
#define MTK_NFC_NDEFRECORD_TNF_NFCWELLKNOWN (0x01)  /**< NFC well-known type (RTD). */
#define MTK_NFC_NDEFRECORD_TNF_MEDIATYPE    (0x02)  /**< Media Type. */
#define MTK_NFC_NDEFRECORD_TNF_ABSURI       (0x03)  /**< Absolute URI. */
#define MTK_NFC_NDEFRECORD_TNF_NFCEXT       (0x04)  /**< Nfc Extenal Type (following the RTD format). */
#define MTK_NFC_NDEFRECORD_TNF_UNKNOWN      (0x05)  /**< Unknown type; Contains no Type information. */
#define MTK_NFC_NDEFRECORD_TNF_UNCHANGED    (0x06)  /**< Unchanged: Used for Chunked Records. */
#define MTK_NFC_NDEFRECORD_TNF_RESERVED     (0x07)  /**< RFU, must not be used. */

//#define PHFRINFCNDEFRECORD_NORMAL_RECORD_BYTE   4               /** \internal Normal record. */
#define MTK_NFC_NDEFRECORD_TNFBYTE_MASK       (0x07) /** \internal For masking */
#define MTK_NFC_NDEFRECORD_BUF_INC1           1               /** \internal Increment Buffer Address by 1 */
#define MTK_NFC_NDEFRECORD_BUF_INC2           2               /** \internal Increment Buffer Address by 2 */
#define MTK_NFC_NDEFRECORD_BUF_INC3           3               /** \internal Increment Buffer Address by 3 */
#define MTK_NFC_NDEFRECORD_BUF_INC4           4               /** \internal Increment Buffer Address by 4 */
#define MTK_NFC_NDEFRECORD_BUF_INC5           5               /** \internal Increment Buffer Address by 5 */
#define MTK_NFC_NDEFRECORD_BUF_TNF_VALUE      (0x00) /** \internal If TNF = Empty, Unknown and Unchanged, the id, type and payload length is ZERO  */
#define MTK_NFC_NDEFRECORD_FLAG_MASK          (0xF8) /** \internal To Mask the Flag Byte */

#define MTK_NFC_NDEFRECORD_FLAGS_MB       (0x80)  /**< This marks the begin of a NDEF Message. */
#define MTK_NFC_NDEFRECORD_FLAGS_ME       (0x40)  /**< Set if the record is at the Message End. */
#define MTK_NFC_NDEFRECORD_FLAGS_CF       (0x20)  /**< Chunk Flag: The record is a record chunk only. */
#define MTK_NFC_NDEFRECORD_FLAGS_SR       (0x10)  /**< Short Record: Payload Length is encoded in ONE byte only. */
#define MTK_NFC_NDEFRECORD_FLAGS_IL       (0x08)  /**< The ID Length Field is present. */

#define PAYLOAD_LENGTH (512)

#define MTK_NFC_WIFIWPS_TYPE_LENGTH    (23)
#define MTK_NFC_BT_TYPE_LENGTH         (32)
#define MTK_NFC_WIFIDIRECT_TYPE_LENGTH (23)
#define MTK_NFC_WIFIFC_TYPE_LENGTH     (23)



#define SWAP16(x) \
    ((UINT16)( \
    (((UINT16)(x) & (UINT16) 0x00ffU) << 8) | \
    (((UINT16)(x) & (UINT16) 0xff00U) >> 8) ))

#define SWAP32(x) \
    ((UINT32)( \
    (((UINT32)(x) & (UINT32) 0x000000ffUL) << 24) | \
    (((UINT32)(x) & (UINT32) 0x0000ff00UL) <<  8) | \
    (((UINT32)(x) & (UINT32) 0x00ff0000UL) >>  8) | \
    (((UINT32)(x) & (UINT32) 0xff000000UL) >> 24) ))




typedef enum MTK_NFCHO_RADIODTAT_TYPE
{
    MTK_NFCHO_RADIODATA_DEFAULT = 0,
    MTK_NFCHO_RADIODATA_CONFIG  = 1,
    MTK_NFCHO_RADIODATA_PASSWD  = 2,
    MTK_NFCHO_RADIODATA_BEAMPLUS = 3,
    MTK_NFCHO_RADIODATA_END
} MTK_NFCHO_RADIODTAT_TYPE;



typedef struct mtk_nfcho_ndefrecord
{
    UINT8   Flags;
    UINT8   Tnf;
    UINT8   TypeLength;
    UINT8   *Type;
    UINT8   IdLength;
    UINT8   *Id;
    UINT32    PayloadLength;
    UINT8   *PayloadData;
} mtk_nfcho_ndefrecord_t;




VOID mtk_nfcho_service_send_handle(UINT8* buf, UINT32 len);
VOID mtk_nfcho_service_event_handle(UINT16 Event, UINT8 DataType, UINT16 DataStartIdx, UINT16 DataLth, UINT8* DataBuf);
VOID mtk_nfcho_service_dbg_output (CH *data, ...);


/// Nfc service action functions

INT32 mtk_nfcho_service_send_msg(UINT32 type, UINT8* payload, UINT32 len);
INT32 mtk_nfcho_service_nfc_start(UINT8 Action);
INT32 mtk_nfcho_service_nfc_hwtest(UINT8 TestMode);
INT32 mtk_nfcho_service_nfc_deactive(void);
INT32 mtk_nfcho_service_nfc_op(UINT8 Action);

#ifndef SUPPORT_EM_CMD
INT32 mtk_nfcho_service_deinit(void);
INT32 mtk_nfcho_service_exit(void);
#endif

INT32 mtk_nfcho_service_p2p_create_server(UINT8 SerName[], UINT32 Length, UINT8 uSap);
INT32 mtk_nfcho_service_p2p_accpet_server(UINT32 Handle);
INT32 mtk_nfcho_service_p2p_create_client(void);
INT32 mtk_nfcho_service_p2p_connect_client(UINT32 Handle);
INT32 mtk_nfcho_service_p2p_send_data(UINT8* DataBuf, UINT32 DataLth);
INT32 mtk_nfcho_service_p2p_receive_data(void);
INT32 mtk_nfcho_service_p2p_close_service(UINT32 handle);

INT32 mtk_nfcho_service_tag_write(UINT8* DataBuf, UINT32 DataLth);
INT32 mtk_nfcho_service_tag_read(void);
INT32 mtk_nfcho_service_tag_format(void);


// SNEP functions
INT32 mtk_nfcho_snep_recv(void);
void mtk_nfcho_snep_stop(UINT32 handle);
INT32 mtk_nfcho_snep_parse(UINT8* u1Data, UINT16 u2Length, UINT8 u1ConnType, UINT8* pReadStatus, UINT8 EnSnep);
INT32 mtk_nfcho_snep_client_get( UINT8 SnepData[], UINT32 Length);
INT32 mtk_nfcho_snep_client_put( UINT8 SnepData[], UINT32 Length);


// handover management functions
INT32 mtk_nfcho_handover_nego_mgt(UINT8 InData[], UINT32 InLength, UINT8 OutData[], UINT32* pOutLength);
INT32 mtk_nfcho_handover_static_mgt( UINT8 u1Data[], UINT32 Length);
INT32 mtk_nfcho_handover_client_run(UINT8 IsRadio, UINT8 NumRadio, UINT8 RadioList[], UINT8* DataBuf[], UINT32 DataLth[], UINT8 Sap);



// Handover RTD parser/generator

INT32 mtk_nfcho_handover_radio_payload_parse(UINT8 RadioType, UINT8 Data[], UINT16 Length);

INT32 mtk_nfcho_handover_select_parse(UINT8 Data[], UINT32 Length, UINT8 MaxNum,
                                      UINT8** OutData, UINT16* OutLength,
                                      UINT8* pNum, UINT8 Power[], UINT8 Radio[],
                                      UINT8 CarrierID[]);
INT32 mtk_nfcho_handover_request_parse(UINT8 Data[], UINT32 Length, UINT8 MaxNum,
                                       UINT8** OutData, UINT16* OutLength,
                                       UINT16* pReceivedRandom, UINT8* pNum, UINT8 Power[], UINT8 Radio[],
                                       UINT8 CarrierID[] );
INT32 mtk_nfcho_handover_select_build( UINT8 Num, UINT8 Power[], UINT8 Radio[], MTK_NFC_APP_DATA_T* pData,
                                       UINT32* pOutLength, UINT8 OutData[] );
INT32 mtk_nfcho_handover_request_build(UINT8 Num, UINT8 Power[], UINT8 Radio[], MTK_NFC_APP_DATA_T* pData,
                                       UINT32* pOutLength, UINT8 OutData[], UINT16* pRequestRandom);

INT32 mtk_nfcho_handover_token_parse(UINT8 Data[], UINT32 Length);
INT32 mtk_nfcho_handover_token_build(UINT8 TokenData[],UINT16 TokenLength, UINT8 OutData[],UINT32* pOutLength, UINT8 Radio, UINT8 Idx, UINT8 TotalNum);

// NDEF parser/generator
INT32 mtk_nfchod_NdefRecord_Generate(mtk_nfcho_ndefrecord_t *Record, UINT8* Buffer, UINT32 MaxBufferSize, UINT32* BytesWritten);
INT32 mtk_nfchod_NdefRecord_Parse(mtk_nfcho_ndefrecord_t* Record,UINT8*   RawRecord);


/// Porting  functions
extern mtk_nfc_app_service_sys_event_handle g_event_handle;
extern mtk_nfc_app_service_sys_send_handle g_send_handle;
extern mtk_nfc_app_service_sys_debug_handle g_dbg_output;



#define mtk_nfcho_service_event_handle g_event_handle
#define mtk_nfcho_service_send_handle g_send_handle
#define mtk_nfcho_service_dbg_output g_dbg_output

#endif // _MTKNFCHO_HO_H_

