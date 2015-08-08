/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2012
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
 *  mtk_nfc_adp_sys.h
 *
 * Project:
 * --------
 *  NFC
 *
 * Description:
 * ------------
 *  Operation System Abstration Layer Implementation
 *
 * Author:
 * -------
 *  Hiki Chen, ext 25281, hiki.chen@mediatek.com, 2012-05-10
 *
 *******************************************************************************/

#ifndef MTK_NFC_ADP_SYS_H
#define MTK_NFC_ADP_SYS_H

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************
 * Include
 *****************************************************************************/
#include "mtk_nfc_sys_type.h"
#include "mtk_nfc_sys_type_ext.h"
#include "mtk_nfc_sys.h"

/*****************************************************************************
 * Define
 *****************************************************************************/
#define NFC_MSG_HDR_SIZE    sizeof(MTK_NFC_MSG_T)

/*****************************************************************************
 * Enum
 *****************************************************************************/
typedef enum MTK_NFC_THREAD
{
    MTK_NFC_THREAD_MAIN = 0,
//   MTK_NFC_THREAD_SERV,
    MTK_NFC_THREAD_RXHDLR,
//   MTK_NFC_THREAD_SOCKET,
    MTK_NFC_THREAD_ADAPT,
    MTK_NFC_THREAD_CLIENT,
    MTK_NFC_THREAD_END
} MTK_NFC_THREAD_E;

/*****************************************************************************
 * Data Structure
 *****************************************************************************/

/*****************************************************************************
 * Extern Area
 *****************************************************************************/

/*****************************************************************************
 * Porting Layer APIs
 *****************************************************************************/
INT32 mtk_nfc_adp_sys_socket_read(INT32 csock, UINT8 *pRecvBuff, INT32 i4RecvLen);
INT32 mtk_nfc_adp_sys_socket_write(INT32 csock, UINT8 *pSendBuff, INT32 i4SendLen);
INT32 mtk_nfc_adp_sys_msg_recv (MTK_NFC_TASKID_E task_id, MTK_NFC_MSG_T **msg);
INT32 mtk_nfc_adp_sys_msg_send (MTK_NFC_TASKID_E task_id, const MTK_NFC_MSG_T *msg);

/*****************************************************************************
 * Exported APIs
 *****************************************************************************/
// - nfc adapt proc
INT32 mtk_nfc_adapt_proc (UINT8* pBuf);

// - nfc client proc
INT32 mtk_nfc_client_proc (UINT8* pBuf);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif

