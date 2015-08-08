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
 *  mtk_app_service_type.h
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

#ifndef _MTKNFCAPPSER_TYPE_H_
#define _MTKNFCAPPSER_TYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

// NDEF Data Type
#define MTK_NFC_HO_EMPTY_RTD        (0x00)  ///< Empty Record, no type, ID or payload present.
#define MTK_NFC_HO_NFCWELLKNOWN_RTD (0x01)  ///< NFC well-known type (RTD). 
#define MTK_NFC_HO_MEDIATYPE_RTD    (0x02)  ///< Media Type. 
#define MTK_NFC_HO_ABSURI_RTD       (0x03)  ///< Absolute . 
#define MTK_NFC_HO_NFCEXT_RTD       (0x04)  ///< Nfc Extenal Type (following the RTD format). 
#define MTK_NFC_HO_UNKNOWN_RTD      (0x05)  ///< Unknown type; Contains no Type information. 
#define MTK_NFC_HO_UNCHANGED_RTD    (0x06)  ///< Unchanged: Used for Chunked Records. 
#define MTK_NFC_HO_RESERVED_RTD      (0x07)  ///< RFU, must not be used. 

// HW Test Cmds
#define MTK_NFC_START_HWTEST_DEP    (0xC0)
#define MTK_NFC_START_HWTEST_SWP    (0xC1)
#define MTK_NFC_START_HWTEST_CARD   (0xC2)
#define MTK_NFC_START_HWTEST_VCARD  (0xC3)

// Command Response
#define MTK_NFC_ENTER_RSP          (0xD0)  // 
#define MTK_NFC_START_RSP          (0xD1)  // 
#define MTK_NFC_READ_RSP           (0xD2)  // 
#define MTK_NFC_SELIST_RSP           (0xD3)
#define MTK_NFC_SE_ENABLE_RSP        (0xD4)



// RTD Data Type
#define MTK_NFC_HO_TXT_RTD           (0xE1)  // 
#define MTK_NFC_HO_URI_RTD           (0xE2)  // 

// Handover Radio Carrier Data Type
#define MTK_NFC_HO_WPSCF_TYPE           (0xF1)
#define MTK_NFC_HO_WPSPD_TYPE           (0xF2)
#define MTK_NFC_HO_WIFIDIRECTHOS_TYPE   (0xF3)
#define MTK_NFC_HO_WPSHOS_TYPE          (0xF4)
#define MTK_NFC_HO_WIFIFCS_TYPE         (0xF5)
#define MTK_NFC_HO_BTS_TYPE             (0xF6)
#define MTK_NFC_HO_WIFIDIRECTHOR_TYPE   (0xF7)
#define MTK_NFC_HO_WPSHOR_TYPE          (0xF8)
#define MTK_NFC_HO_WIFIFCR_TYPE         (0xF9)
#define MTK_NFC_HO_BTR_TYPE             (0xFA)


typedef enum MTK_NFC_APP_EVENT_TYPE
{
    MTK_NFC_APP_EVENT_DEFAULT = 0,
    MTK_NFC_APP_EVENT_RESULT    ,
    MTK_NFC_APP_EVENT_READ       ,
    MTK_NFC_APP_EVENT_WRITE      ,
    MTK_NFC_APP_EVENT_FOUNDDEVICE,
    MTK_NFC_APP_EVENT_FOUNDTAG ,
    MTK_NFC_APP_EVENT_DEVICELEFT ,
    MTK_NFC_APP_EVENT_KILLDONE ,
    MTK_NFC_APP_EVENT_END
} MTK_NFC_APP_EVENT_TYPE;


typedef enum MTK_NFCHO_RADIO_TYPE
{
    MTK_NFCHO_RADIO_WIFIWPS     = 0,
    MTK_NFCHO_RADIO_BT          = 1,
    MTK_NFCHO_RADIO_WIFIDIRECT  = 2,
    MTK_NFCHO_RADIO_WIFIFC      = 3,
    MTK_NFCHO_RADIO_END
} MTK_NFCHO_RADIO_TYPE;

typedef struct
{
    unsigned int type;
    unsigned int length;
} MTK_NFC_APP_MSG_T;



typedef struct MTK_NFC_APP_DATA_T
{
    unsigned short lth;
    unsigned char* data;
} MTK_NFC_APP_DATA_T;



typedef struct MTK_NFC_APP_WRITEATA_T
{
    unsigned char  non_ndef; // 1: non-ndef , Wrtie RAW Data , 0: ndef , Write NDEF Data
    unsigned char  negohandover; // 1: for nego handover requester , 2 :  for nego handover selector , 3 :  for static handover token,  0: others
    unsigned char  num_payload;
    unsigned char  radio_type[MTK_NFCHO_RADIO_END];   // For handover records  Refer MTK_NFCHO_RADIO_TYPE definition, If it is not radio carrier data, radio_type will be set to 0xFF
    unsigned char  data_type[MTK_NFCHO_RADIO_END];    //data type array. Refer MTK_NFC_HO_ definition
    MTK_NFC_APP_DATA_T  payload[MTK_NFCHO_RADIO_END];//data buffer array. Each data type has one data buffer ,  //data buffer length. Each data type has one data buffer length
    MTK_NFC_APP_DATA_T payload_type[MTK_NFCHO_RADIO_END];    //NDEF type name for common NDEF usage ,  //NDEF type name length  for common NDEF usage
} MTK_NFC_APP_WRITEATA_T;

typedef void (*mtk_nfc_app_service_sys_event_handle) (unsigned short Event, unsigned char DataType, unsigned short DataStartIdx, unsigned short DataLth, unsigned char* DataBuf);

///mtk_nfc_app_service_sys_event_handle's  MTK_NFC_APP_EVENT_READ event will use DataStartIdx, DataLth, DataBuf to bring read data information
/// form index 0 ~ (DataStartIdx-1) is NDEF type string
/// form index DataStartIdx ~ (DataLth-1) is NDEF payload data


typedef void (*mtk_nfc_app_service_sys_send_handle) (unsigned char* buf, unsigned int len);
typedef void (*mtk_nfc_app_service_sys_debug_handle) (char *data, ...);


typedef struct MTK_NFC_APP_FUNCTION_POINT
{
    mtk_nfc_app_service_sys_event_handle event;
    mtk_nfc_app_service_sys_send_handle  send;
    mtk_nfc_app_service_sys_debug_handle debug;
} MTK_NFC_APP_FUNCTION_POINT;


/// Interfaces of service module & HO management
int mtk_nfc_app_service_start(unsigned char Action);
int mtk_nfc_app_service_stop(unsigned char Action);
int mtk_nfc_app_service_read(unsigned char Action);
int mtk_nfc_app_service_write(MTK_NFC_APP_WRITEATA_T* pData);

int mtk_nfc_app_service_enter(MTK_NFC_APP_FUNCTION_POINT* pFunList, unsigned char TestMode);
int mtk_nfc_app_service_exit(void);

int mtk_nfc_app_service_card_get_selist(void);
int mtk_nfc_app_service_card_enable(unsigned char SEid, unsigned char Action);


/// Nfc service message porting handler function
int mtk_nfc_app_service_proc(unsigned char* Data);

/// Nfc service message  porting functions


#ifdef __cplusplus
}  /* extern "C" */
#endif


#endif // _MTKNFCAPPSER_TYPE_H_

