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
 *  linux_nfc_main.h
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
#ifndef _MTK_NFC_MAIN_H_
#define _MTK_NFC_MAIN_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "mtk_nfc_sys.h"
#include "mtk_nfc_adp_sys.h"


/*****************************************************************************
 * Define
 *****************************************************************************/
// option feature

#define USE_SHARE_MEMORY_TO_COMMUNICTION
#define USE_SIGNAL_EVENT_TO_TIMER_CREATE
//#define ADJUST_THREAD_PRIORITY

// software queue
#define MTK_NFCD_2SERVICE              "/tmp/nfc2service"
#define MTK_NFCD_SERVICE2APP           "/tmp/nfcservice2app"
#define MTK_NFCD_2MAIN                 "/tmp/nfc2main"

// handler
#define C_INVALID_TID       (-1)   /*invalid thread id*/
#define C_INVALID_FD        (-1)   /*invalid file handle*/
#define C_INVALID_SOCKET    (-1)
#define THREAD_NUM          (4)    /*MAIN/DATA READ/ADAPT/CLIENT THREAD*/

// timer related constant
#define CLOCKID CLOCK_REALTIME
#define SIG     SIGRTMIN

#define DSP_UART_IN_BUFFER_SIZE (1024)

#define MONITOR_TAG_D "nfcsd(D) : "

// debug
#define DEBUG_LOG
#ifdef DEBUG_LOG
#define LOGD(...)         {printf(MONITOR_TAG_D); printf(__VA_ARGS__);}
#else
#define LOGD(...)
#endif
/*
#define MONITOR_TAG_D "nfcsd(D) : "
#define MONITOR_TAG_W "nfcsd(W) : "
#define MONITOR_TAG_E "nfcsd(E) : "

#define MTK_NFC_LOGD(...)  {printf(MONITOR_TAG_D); printf(__VA_ARGS__); printf("\n");}
#define MTK_NFC_LOGW(...)  {printf(MONITOR_TAG_W); printf(__VA_ARGS__); printf("\n");}
#define MTK_NFC_LOGE(...)  {printf(MONITOR_TAG_E); printf(__VA_ARGS__); printf("\n");}
*/


/*****************************************************************************
 * Data Structure
 *****************************************************************************/

typedef struct MTK_NFC_THREAD_T
{
    int                     snd_fd;
    MTK_NFC_THREAD_E        thread_id;
    pthread_t               thread_handle;
    int (*thread_exit)(struct MTK_NFC_THREAD_T *arg);
    int (*thread_active)(struct MTK_NFC_THREAD_T *arg);
} MTK_NFC_THREAD_T;

/*****************************************************************************
 * Function Prototypes
 *****************************************************************************/

#endif

