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
 *  mtk_nfchod_main_sys.h
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

#ifndef _MTKNFCHO_TYPE_H_
#define _MTKNFCHO_TYPE_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/poll.h>
#include <linux/msg.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <mqueue.h>
#include <semaphore.h>
#include <sys/wait.h>

#include <sys/ioctl.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include <sys/types.h>
//#include <linux/in.h>
#include <linux/if_ether.h>
#include <stdarg.h>

#include "mtk_app_service_type.h"


#define RT_PRIV_IOCTL       (SIOCIWFIRSTPRIV + 0x01)


#define C_INVALID_TID                  (-1)

#define MTK_NFCD_2SERVICE              "/tmp/nfc2service"
#define MTK_NFCD_SERVICE2APP           "/tmp/nfcservice2app"

#define MONITOR_TAG_D "nfcho(D)"
#define MONITOR_TAG_W "nfcho(W)"
#define MONITOR_TAG_E "nfcho(E)"
#define MONITOR_TAG_S "nfcho "

#define MTK_NFCAPP_LOGD(...)  {printf(MONITOR_TAG_D); printf(__VA_ARGS__); printf("\n");}
#define MTK_NFCAPP_LOGW(...)  {printf(MONITOR_TAG_W); printf(__VA_ARGS__); printf("\n");}
#define MTK_NFCAPP_LOGE(...)  {printf(MONITOR_TAG_E); printf(__VA_ARGS__); printf("\n");}
#define MTK_NFCAPP_LOGS(...)  {printf(MONITOR_TAG_S); printf(__VA_ARGS__); printf("\n");}

#define DEFAULT_ACCEPTABLE_LENGTH (1024)



#define MTK_WIFINFC_ACT_TONFC_GET   (0x00)
#define MTK_WIFINFC_ACT_TONFC_SET   (0x01)

#define MTK_WIFINFC_ACT_FROMNFC_GET (0x40)
#define MTK_WIFINFC_ACT_FROMNFC_SET (0x41)

#define MTK_WIFINFC_TYPE_CMDRESULT  (0x00)
#define MTK_WIFINFC_TYPE_CONFIG     (0x01)
#define MTK_WIFINFC_TYPE_PASSWD     (0x02)
#define MTK_WIFINFC_TYPE_NFCSTATUS  (0x05)
#define MTK_WIFINFC_TYPE_WIFISTATUS (0x06)
#define MTK_WIFINFC_TYPE_PASSWDHOS  (0x07)
#define MTK_WIFINFC_TYPE_PASSWDHOR  (0x08)
#define MTK_WIFINFC_TYPE_FASTCONNECTS (0x09)
#define MTK_WIFINFC_TYPE_FASTCONNECTR (0x0A)
#define MTK_WIFINFC_TYPE_WIFIDIRECTHOS (0x0B)
#define MTK_WIFINFC_TYPE_WIFIDIRECTHOR (0x0C)

#define MTK_BTNFC_TYPE_CMDRESULT    (0x10)
#define MTK_BTNFC_TYPE_MACADDR      (0x11)

#define MTK_WIFINFC_SET_OFF              (0x00)
#define MTK_WIFINFC_SET_ON               (0x01)

#define MTK_WIFINFC_SET_P2PSERVER        (0x02)
#define MTK_WIFINFC_SET_READER_P2PSERVER (0x03)

#define MTK_WIFINFC_SET_P2PSERVER_T        (0x04)
#define MTK_WIFINFC_SET_READER_P2PSERVER_T (0x05)

#define MTK_WIFINFC_SET_P2PSERVER_I        (0x06)
#define MTK_WIFINFC_SET_READER_P2PSERVER_I (0x07)

#define MTK_WIFINFC_SET_P2PCLIENT          (0x08)
#define MTK_WIFINFC_SET_READERP2PCLIENT    (0x09)

#define MTK_WIFINFC_SET_P2PCLIENT_T        (0x0A)
#define MTK_WIFINFC_SET_READERP2PCLIENT_T  (0x0B)

#define MTK_WIFINFC_SET_P2PCLIENT_I        (0x0C)
#define MTK_WIFINFC_SET_READERP2PCLIENT_I  (0x0D)

#define MTK_WIFINFC_READERTAG              (0x0E)
#define MTK_WIFINFC_FORMATTAG              (0x0F)

#define MTK_WIFINFC_SET_P2PSERVER_A        (0x10)
#define MTK_WIFINFC_SET_P2PCLIENT_A        (0x11)
#define MTK_WIFINFC_SET_P2PSERVER_212_I    (0x12)
#define MTK_WIFINFC_SET_P2PSERVER_212_T    (0x13)
#define MTK_WIFINFC_SET_READER             (0x14)

#define MTK_WIFINFC_SET_CARD_READER_P2P    (0x15)
#define MTK_WIFINFC_SET_END                (0x16)

#define MTK_WIFINFC_HWTEST_DEP             (0xF0)
#define MTK_WIFINFC_HWTEST_SWP             (0xF1)
#define MTK_WIFINFC_HWTEST_CARD            (0xF2)
#define MTK_WIFINFC_HWTEST_VCARD           (0xF3)
#define MTK_WIFINFC_HWTEST_END             (0xF4)

#define MTK_WIFINFC_NFC_ON         (1<<0)
#define MTK_WIFINFC_NFC_DETECTED   (1<<1)


typedef enum MTK_NFCHO_THREAD_ID_ENUM
{
    MTK_NFCHO_THREAD_UNKNOWN      = -1,
    MTK_NFCHO_THREAD_SERVICE      = 0,
    MTK_NFCHO_THREAD_END
} MTK_NFCHO_THREAD_ID_ENUM;


typedef struct _MTK_NFCHO_THREAD_T
{
    MTK_NFCHO_THREAD_ID_ENUM  thread_id;
    pthread_t   thread_handle;
    int (*thread_exit)(struct _MTK_NFCHO_THREAD_T *arg);
} MTK_NFCHO_THREAD_T;


/// Nfc service message  functions
int mtk_nfcho_service_recv_handle(void);


// Handover thread functions
int mtk_nfcho_threads_create(MTK_NFCHO_THREAD_ID_ENUM thread_id);
int mtk_nfcho_threads_release(MTK_NFCHO_THREAD_ID_ENUM thread_id);

#endif

