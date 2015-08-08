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
 *  mtk_nfc_osal.c
 *
 * Project:
 * --------
 *
 * Description:
 * ------------
 *
 * Author:
 * -------
 *  Hiki Chen, ext 25281, hiki.chen@mediatek.com, 2012-05-10
 *
 *******************************************************************************/
/*****************************************************************************
 * Include
 *****************************************************************************/
#ifdef WIN32
#include <windows.h>
#include <assert.h>
#include <string.h>
#endif

#include <pthread.h>
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <signal.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
//#include <sys/wait.h>
//#include <sys/ipc.h>
#include <sys/time.h>   // for gettimeofday
//#include <sys/timeb.h>
#include <sys/ioctl.h>

#include "mtk_nfc_sys.h"
#include <stdarg.h>


#include "linux_nfc_main.h" // include option feature

/*****************************************************************************
 * Define
 *****************************************************************************/
#ifdef WIN32
#define WM_MTK_NFC_TASK  (WM_USER)

#define DBG_STRING_MAX_SIZE_RAW_DATA 2048
#define DBG_STRING_MAX_SIZE_CID_INFO   32
#endif

#define DBG_STRING_MAX_SIZE           512

/*****************************************************************************
 * Enum
 *****************************************************************************/

/*****************************************************************************
 * Data Structure
 *****************************************************************************/
#ifdef WIN32
typedef struct
{
    CRITICAL_SECTION    cs;
    BOOL                is_used;    // 1 = used; 0 = unused
    UINT32              timer_id;   //timer's id returned from SetTimer()
    ppCallBck_t         timer_expiry_callback;
    VOID                *timer_expiry_context;
} nfc_timer_table_struct;
#else // for linux
typedef struct
{
    BOOL                is_used;    // 1 = used; 0 = unused
    timer_t             handle;     // system timer handle
    ppCallBck_t         timer_expiry_callback; // timeout callback
    VOID                *timer_expiry_context; // timeout callback context
    BOOL                is_stopped;    // 1 = stopped; 0 = running
} nfc_timer_table_struct;
#endif

/*****************************************************************************
 * Extern Area
 *****************************************************************************/
#ifdef WIN32
extern DWORD g_dwNfcRxHdlrThreadId;
extern DWORD g_dwNfcMainThreadId;
#endif

extern int g_nfc_2app_fd;

extern struct sockaddr_un g_nfc_service_socket;
extern struct sockaddr_un g_nfc_main_socket;
extern struct sockaddr_un g_nfc_2app_socket;


extern int gInterfaceHandle;
extern int gconn_fd_tmp;

extern FILE *g_dbg_fp;
/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/
#ifdef WIN32
// timer pool
static nfc_timer_table_struct nfc_timer_table[MTK_NFC_TIMER_MAX_NUM];

// mutex array
static HANDLE g_hMutex[MTK_NFC_MUTEX_MAX_NUM];

// debug log
HANDLE g_hLogFile = INVALID_HANDLE_VALUE;
HANDLE g_hLogTrxFile = INVALID_HANDLE_VALUE;
#else // for Linux
// timer pool
static nfc_timer_table_struct nfc_timer_table[MTK_NFC_TIMER_MAX_NUM];

// mutex array
static pthread_mutex_t g_hMutex[MTK_NFC_MUTEX_MAX_NUM];

// message queue
#define MTK_NFC_MSG_RING_SIZE 128

MTK_NFC_MSG_RING_BUF nfc_main_msg_ring_body;
MTK_NFC_MSG_RING_BUF * nfc_main_msg_ring = NULL;
MTK_NFC_MSG_T * nfc_main_msg_ring_buffer[MTK_NFC_MSG_RING_SIZE]; //pointer array
INT32 nfc_main_msg_cnt;


MTK_NFC_MSG_RING_BUF nfc_service_msg_ring_body;
MTK_NFC_MSG_RING_BUF * nfc_service_msg_ring = NULL;
MTK_NFC_MSG_T * nfc_service_msg_ring_buffer[MTK_NFC_MSG_RING_SIZE]; //pointer array
INT32 nfc_service_msg_cnt;


MTK_NFC_MSG_RING_BUF nfc_socket_msg_ring_body;
MTK_NFC_MSG_RING_BUF * nfc_socket_msg_ring = NULL;
MTK_NFC_MSG_T * nfc_socket_msg_ring_buffer[MTK_NFC_MSG_RING_SIZE]; //pointer array
INT32 nfc_socket_msg_cnt;

#endif

/*****************************************************************************
 * Function
 *****************************************************************************/
/*****************************************************************************
 * Function
 *  mtk_nfc_sys_mem_alloc
 * DESCRIPTION
 *  Allocate a block of memory
 * PARAMETERS
 *  size [IN] the length of the whole memory to be allocated
 * RETURNS
 *  On success, return the pointer to the allocated memory
 * NULL (0) if failed
 *****************************************************************************/
VOID *
mtk_nfc_sys_mem_alloc (
    UINT32 u4Size
)
{
    void *pMem = NULL;

    if (u4Size != 0)
    {
        pMem = malloc(u4Size);
    }

    return pMem;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_mem_free
 * DESCRIPTION
 *  Release unused memory
 * PARAMETERS
 *  pMem        [IN] the freed memory address
 * RETURNS
 *  NONE
 *****************************************************************************/
VOID
mtk_nfc_sys_mem_free (
    VOID *pMem
)
{
    if (pMem != NULL)
    {
        free(pMem);
    }
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_mutex_initialize
 * DESCRIPTION
 *  Create a mutex object
 * PARAMETERS
 *  mutex_id    [IN] mutex index used by NFC library
 * RETURNS
 *  MTK_NFC_SUCCESS
 *****************************************************************************/
INT32
mtk_nfc_sys_mutex_initialize (
    VOID
)
{
    INT8 index;
    // - TBD
    for (index = 0; index < MTK_NFC_MUTEX_MAX_NUM; index++)
    {
        pthread_mutex_init(&g_hMutex[index], NULL);
    }

    return MTK_NFC_SUCCESS;
}


/*****************************************************************************
 * Function
 *  mtk_nfc_sys_mutex_create
 * DESCRIPTION
 *  Create a mutex object
 * PARAMETERS
 *  mutex_id    [IN] mutex index used by NFC library
 * RETURNS
 *  MTK_NFC_SUCCESS
 *****************************************************************************/
INT32
mtk_nfc_sys_mutex_create (
    MTK_NFC_MUTEX_E mutex_id
)
{
    if (mutex_id >= MTK_NFC_MUTEX_MAX_NUM)
    {
        return MTK_NFC_ERROR;
    }

    pthread_mutex_init(&g_hMutex[mutex_id], NULL);
    return MTK_NFC_SUCCESS;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_mutex_take
 * DESCRIPTION
 *  Request ownership of a mutex and if it's not available now, then block the
 *  thread execution
 * PARAMETERS
 *  mutex_id    [IN] mutex index used by NFC library
 * RETURNS
 *  MTK_NFC_SUCCESS
 *****************************************************************************/
INT32
mtk_nfc_sys_mutex_take (
    MTK_NFC_MUTEX_E mutex_id
)
{
    pthread_mutex_lock(&g_hMutex[mutex_id]);
    return MTK_NFC_SUCCESS;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_mutex_give
 * DESCRIPTION
 *  Release a mutex ownership
 * PARAMETERS
 *  mutex_id    [IN] mutex index used by NFC library
 * RETURNS
 *  MTK_NFC_SUCCESS
 *****************************************************************************/
INT32
mtk_nfc_sys_mutex_give (
    MTK_NFC_MUTEX_E mutex_id
)
{
    pthread_mutex_unlock(&g_hMutex[mutex_id]);
    return MTK_NFC_SUCCESS;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_mutex_destory
 * DESCRIPTION
 *  Destory a mutex object
 * PARAMETERS
 *  mutex_id    [IN] mutex index used by NFC library
 * RETURNS
 *  MTK_NFC_SUCCESS
 *****************************************************************************/
INT32
mtk_nfc_sys_mutex_destory (
    MTK_NFC_MUTEX_E mutex_id
)
{

    if (pthread_mutex_destroy(&g_hMutex[mutex_id]))
    {
        return MTK_NFC_ERROR;
    }
    return MTK_NFC_SUCCESS;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_msg_alloc
 * DESCRIPTION
 *  Allocate a block of memory for message
 * PARAMETERS
 *  u2Size      [IN] the length of the whole MTK_NFC_MSG structure
 * RETURNS
 *  Pinter to the created message if successed
 *  NULL (0) if failed
 *****************************************************************************/
MTK_NFC_MSG_T *
mtk_nfc_sys_msg_alloc (
    UINT16 u2Size
)
{
    return mtk_nfc_sys_mem_alloc(u2Size);
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_msg_initialize
 * DESCRIPTION
 *  Send a message to a task
 * PARAMETERS
 *  task_id     [IN] target task id
 *  msg         [IN] the send message
 * RETURNS
 *  MTK_NFC_SUCCESS
 *****************************************************************************/
INT32
mtk_nfc_sys_msg_initialize (
    VOID
)
{

    //For Main Message
    nfc_main_msg_ring = &nfc_main_msg_ring_body;
    nfc_main_msg_ring->start_buffer = &nfc_main_msg_ring_buffer[0];
    nfc_main_msg_ring->end_buffer = &nfc_main_msg_ring_buffer[MTK_NFC_MSG_RING_SIZE-1];
    nfc_main_msg_ring->next_write = nfc_main_msg_ring->start_buffer;
    nfc_main_msg_ring->next_read = nfc_main_msg_ring->start_buffer;
    nfc_main_msg_cnt = 0;

    //For Service Message
    nfc_service_msg_ring = &nfc_service_msg_ring_body;
    nfc_service_msg_ring->start_buffer = &nfc_service_msg_ring_buffer[0];
    nfc_service_msg_ring->end_buffer = &nfc_service_msg_ring_buffer[MTK_NFC_MSG_RING_SIZE-1];
    nfc_service_msg_ring->next_write = nfc_service_msg_ring->start_buffer;
    nfc_service_msg_ring->next_read = nfc_service_msg_ring->start_buffer;
    nfc_service_msg_cnt = 0;

    //For Socket Message
    nfc_socket_msg_ring = &nfc_socket_msg_ring_body;
    nfc_socket_msg_ring->start_buffer = &nfc_socket_msg_ring_buffer[0];
    nfc_socket_msg_ring->end_buffer = &nfc_socket_msg_ring_buffer[MTK_NFC_MSG_RING_SIZE-1];
    nfc_socket_msg_ring->next_write = nfc_socket_msg_ring->start_buffer;
    nfc_socket_msg_ring->next_read = nfc_socket_msg_ring->start_buffer;
    nfc_socket_msg_cnt = 0;

    return MTK_NFC_SUCCESS;
}


/*****************************************************************************
 * Function
 *  mtk_nfc_sys_msg_send
 * DESCRIPTION
 *  Send a message to a task
 * PARAMETERS
 *  task_id     [IN] target task id
 *  msg         [IN] the send message
 * RETURNS
 *  MTK_NFC_SUCCESS
 *****************************************************************************/
INT32
mtk_nfc_sys_msg_send (
    MTK_NFC_TASKID_E task_id,
    const MTK_NFC_MSG_T *msg
)
{
    int ret = MTK_NFC_SUCCESS;

    if (msg == NULL)
    {
        return MTK_NFC_ERROR;
    }

    if (MTK_NFC_TASKID_MAIN == task_id)
    {
        {
            mtk_nfc_sys_mutex_take(MTK_MUTEX_MSG_Q);

            /* msg queue full check */
            if (nfc_main_msg_cnt == MTK_NFC_MSG_RING_SIZE)
            {
                mtk_nfc_sys_mutex_give(MTK_MUTEX_MSG_Q);
                LOGD("Send message to main, full\n");
                return MTK_NFC_ERROR;
            }

            if ( nfc_main_msg_ring != NULL)
            {
                *(nfc_main_msg_ring->next_write) = (MTK_NFC_MSG_T*)msg;

                nfc_main_msg_ring->next_write++;

                // Wrap check the input circular buffer
                if ( nfc_main_msg_ring->next_write > nfc_main_msg_ring->end_buffer )
                {
                    nfc_main_msg_ring->next_write = nfc_main_msg_ring->start_buffer;
                }

                nfc_main_msg_cnt++;

                // LOGD("mtk_nfc_sys_event_set...2MAIN\n");
                if (MTK_NFC_SUCCESS != mtk_nfc_sys_event_set(MTK_NFC_EVENT_2MAIN))
                {
                    LOGD("mtk_nfc_sys_event_set,2MAIN,fail\n");
                    mtk_nfc_sys_mutex_give(MTK_MUTEX_MSG_Q);
                    return MTK_NFC_ERROR;
                }

                mtk_nfc_sys_mutex_give(MTK_MUTEX_MSG_Q);
                return MTK_NFC_SUCCESS;
            }
            else
            {
                mtk_nfc_sys_mutex_give(MTK_MUTEX_MSG_Q);
                return MTK_NFC_ERROR;
            }
        }
    }
    else if (MTK_NFC_TASKID_SERVICE == task_id)
    {
        {
            mtk_nfc_sys_mutex_take(MTK_MUTEX_SERVICE_MSG_Q);

            /* msg queue full check */
            if (nfc_service_msg_cnt == MTK_NFC_MSG_RING_SIZE)
            {
                mtk_nfc_sys_mutex_give(MTK_MUTEX_SERVICE_MSG_Q);
                LOGD("Send message to service, full\n");
                return MTK_NFC_ERROR;
            }

            if ( nfc_service_msg_ring != NULL)
            {
                *(nfc_service_msg_ring->next_write) = (MTK_NFC_MSG_T*)msg;

                //     LOGD("Send message service,%d ,%d\n", msg->type , msg->length );

                nfc_service_msg_ring->next_write++;

                // Wrap check the input circular buffer
                if ( nfc_service_msg_ring->next_write > nfc_service_msg_ring->end_buffer )
                {
                    nfc_service_msg_ring->next_write = nfc_service_msg_ring->start_buffer;
                }

                nfc_service_msg_cnt++;

                //      LOGD("mtk_nfc_sys_event_set...2SERV\n");
                if (MTK_NFC_SUCCESS != mtk_nfc_sys_event_set(MTK_NFC_EVENT_2SERV))
                {
                    LOGD("mtk_nfc_sys_event_set,2SERV,fail\n");
                    mtk_nfc_sys_mutex_give(MTK_MUTEX_SERVICE_MSG_Q);
                    return MTK_NFC_ERROR;
                }

                mtk_nfc_sys_mutex_give(MTK_MUTEX_SERVICE_MSG_Q);
                return MTK_NFC_SUCCESS;
            }
            else
            {
                mtk_nfc_sys_mutex_give(MTK_MUTEX_SERVICE_MSG_Q);
                LOGD("Send message to service, fail, null\n");
                return MTK_NFC_ERROR;
            }
        }
    }
    else if (MTK_NFC_TASKID_SOCKET == task_id)
    {
        mtk_nfc_sys_mutex_take(MTK_MUTEX_SOCKET_MSG_Q);

        /* msg queue full check */
        if (nfc_socket_msg_cnt == MTK_NFC_MSG_RING_SIZE)
        {
            mtk_nfc_sys_mutex_give(MTK_MUTEX_SOCKET_MSG_Q);
            LOGD("Send message to service, full\n");
            return MTK_NFC_ERROR;
        }

        if ( nfc_socket_msg_ring != NULL)
        {
            *(nfc_socket_msg_ring->next_write) = (MTK_NFC_MSG_T*)msg;

            //   LOGD("Send message socket,%d ,%d\n", msg->type , msg->length );

            nfc_socket_msg_ring->next_write++;

            // Wrap check the input circular buffer
            if ( nfc_socket_msg_ring->next_write > nfc_socket_msg_ring->end_buffer )
            {
                nfc_socket_msg_ring->next_write = nfc_socket_msg_ring->start_buffer;
            }

            nfc_socket_msg_cnt++;

            // LOGD("mtk_nfc_sys_event_set...2SOCK\n");
            if (MTK_NFC_SUCCESS != mtk_nfc_sys_event_set(MTK_NFC_EVENT_2SOCKET))
            {
                LOGD("mtk_nfc_sys_event_set,2SOCK,fail\n");
                mtk_nfc_sys_mutex_give(MTK_MUTEX_SOCKET_MSG_Q);
                return MTK_NFC_ERROR;
            }
            mtk_nfc_sys_mutex_give(MTK_MUTEX_SOCKET_MSG_Q);
            return MTK_NFC_SUCCESS;
        }
        else
        {
            mtk_nfc_sys_mutex_give(MTK_MUTEX_SOCKET_MSG_Q);
            LOGD("Send message to service, fail, null\n");
            return MTK_NFC_ERROR;
        }
        return MTK_NFC_SUCCESS;
    }
    else
    {
        return MTK_NFC_ERROR;
    }

    return ret;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_msg_recv
 * DESCRIPTION
 *  Recv a message from a task
 * PARAMETERS
 *  task_id     [IN] target task id
 *  msg         [IN] the receive message pointer
 * RETURNS
 *  MTK_NFC_SUCCESS
 *****************************************************************************/
INT32
mtk_nfc_sys_msg_recv (
    MTK_NFC_TASKID_E task_id,
    MTK_NFC_MSG_T **msg
)
{
    if (msg != NULL)
    {
        if (MTK_NFC_TASKID_MAIN == task_id)
        {
            mtk_nfc_sys_mutex_take(MTK_MUTEX_MSG_Q);

            /* wait signal if no msg in queue */
            if ( nfc_main_msg_cnt <= 0 )
            {
                // LOGD("mtk_nfc_sys_event_wait...2MAIN\n");
                if (MTK_NFC_SUCCESS != mtk_nfc_sys_event_wait(MTK_NFC_EVENT_2MAIN, MTK_MUTEX_MSG_Q))
                {
                    LOGD("mtk_nfc_sys_event_wait,2MAIN,fail\n");
                    mtk_nfc_sys_mutex_give(MTK_MUTEX_MSG_Q);
                    return MTK_NFC_ERROR;
                }
                else
                {
                    //       LOGD("mtk_nfc_sys_event_wait,ok,2MAIN,cnt,%d\n", nfc_main_msg_cnt);
                }

            }

            if (*(nfc_main_msg_ring->next_read) == NULL)
            {
                mtk_nfc_sys_mutex_give(MTK_MUTEX_MSG_Q);
                return MTK_NFC_ERROR;
            }
            (*msg) = *(nfc_main_msg_ring->next_read);


            nfc_main_msg_ring->next_read++;

            // Wrap check output circular buffer
            if ( nfc_main_msg_ring->next_read > nfc_main_msg_ring->end_buffer )
            {
                nfc_main_msg_ring->next_read = nfc_main_msg_ring->start_buffer;
            }

            nfc_main_msg_cnt--;

            mtk_nfc_sys_mutex_give(MTK_MUTEX_MSG_Q);

            return MTK_NFC_SUCCESS;
        }
        else if(MTK_NFC_TASKID_SERVICE == task_id)
        {
            mtk_nfc_sys_mutex_take(MTK_MUTEX_SERVICE_MSG_Q);

            /* wait signal if no msg in queue */
            if ( nfc_service_msg_cnt <= 0 )
            {
                //   LOGD("mtk_nfc_sys_event_wait...2SERV\n");
                if (MTK_NFC_SUCCESS != mtk_nfc_sys_event_wait(MTK_NFC_EVENT_2SERV, MTK_MUTEX_SERVICE_MSG_Q))
                {
                    LOGD("mtk_nfc_sys_event_wait,2SERV,fail\n");
                    mtk_nfc_sys_mutex_give(MTK_MUTEX_SERVICE_MSG_Q);
                    return MTK_NFC_ERROR;
                }
                else
                {
                    //    LOGD("mtk_nfc_sys_event_wait,ok,2SERV,cnt,%d\n", nfc_service_msg_cnt);
                }
            }

            if (*(nfc_service_msg_ring->next_read) == NULL)
            {
                mtk_nfc_sys_mutex_give(MTK_MUTEX_SERVICE_MSG_Q);
                return MTK_NFC_ERROR;
            }
            (*msg) = *(nfc_service_msg_ring->next_read);

            //      LOGD("Rece message service,%d ,%d\n", (*msg)->type , (*msg)->length );
            nfc_service_msg_ring->next_read++;

            // Wrap check output circular buffer
            if ( nfc_service_msg_ring->next_read > nfc_service_msg_ring->end_buffer )
            {
                nfc_service_msg_ring->next_read = nfc_service_msg_ring->start_buffer;
            }

            nfc_service_msg_cnt--;

            mtk_nfc_sys_mutex_give(MTK_MUTEX_SERVICE_MSG_Q);

            return MTK_NFC_SUCCESS;
        }
        else if(MTK_NFC_TASKID_SOCKET == task_id)
        {
            mtk_nfc_sys_mutex_take(MTK_MUTEX_SOCKET_MSG_Q);

            /* wait signal if no msg in queue */
            if ( nfc_socket_msg_cnt <= 0 )
            {
                //       LOGD("mtk_nfc_sys_event_wait...2SOCK\n");
                if (MTK_NFC_SUCCESS != mtk_nfc_sys_event_wait(MTK_NFC_EVENT_2SOCKET, MTK_MUTEX_SOCKET_MSG_Q))
                {
                    LOGD("mtk_nfc_sys_event_wait,2SOCK,fail\n");
                    mtk_nfc_sys_mutex_give(MTK_MUTEX_SOCKET_MSG_Q);
                    return MTK_NFC_ERROR;
                }
                else
                {
                    // LOGD("mtk_nfc_sys_event_wait,ok,2SOCK,cnt,%d\n", nfc_socket_msg_cnt);
                }
            }

            if (*(nfc_socket_msg_ring->next_read) == NULL)
            {
                mtk_nfc_sys_mutex_give(MTK_MUTEX_SOCKET_MSG_Q);

                LOGD("Rece Err, ring null,%d\n" , nfc_socket_msg_cnt);
                return MTK_NFC_ERROR;
            }
            (*msg) = *(nfc_socket_msg_ring->next_read);

            //  LOGD("Rece message scoket,%d ,%d\n", (*msg)->type , (*msg)->length );
            nfc_socket_msg_ring->next_read++;

            // Wrap check output circular buffer
            if ( nfc_socket_msg_ring->next_read > nfc_socket_msg_ring->end_buffer )
            {
                nfc_socket_msg_ring->next_read = nfc_socket_msg_ring->start_buffer;
            }

            nfc_socket_msg_cnt--;

            mtk_nfc_sys_mutex_give(MTK_MUTEX_SOCKET_MSG_Q);

            return MTK_NFC_SUCCESS;
        }
        else
        {
            LOGD("Rece Err, id,%d\n",task_id );
            return MTK_NFC_ERROR;
        }
    }
    else
    {
        return MTK_NFC_ERROR;
    }
    return MTK_NFC_SUCCESS;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_msg_free
 * DESCRIPTION
 *  Free a block of memory for message
 * PARAMETERS
 *  msg         [IN] the freed message
 * RETURNS
 *  NONE
 *****************************************************************************/
VOID
mtk_nfc_sys_msg_free (
    MTK_NFC_MSG_T *msg
)
{
    if (msg != NULL)
    {
        mtk_nfc_sys_mem_free(msg);
    }

    return;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_dbg_string
 * DESCRIPTION
 *  Output a given string
 * PARAMETERS
 *  pString     [IN] pointer to buffer content to be displayed
 * RETURNS
 *  NONE
 *****************************************************************************/
VOID
mtk_nfc_sys_dbg_string (
    const CH *pString
)
{
    // printf("%s  ", pString);

    mtk_nfc_sys_dbg_to_file("%s  ", pString);

    return;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_dbg_trace
 * DESCRIPTION
 *  Output the traced raw data
 * PARAMETERS
 *  pString     [IN] data Data block
 *  length      [IN] size buffer size of the data block
 * RETURNS
 *  NONE
 *****************************************************************************/
VOID
mtk_nfc_sys_dbg_trace (
    UINT8   pData[],
    UINT32  u4Len
)
{
    UINT32 i;

    for (i = 0; i < u4Len; i++)
    {
        //     printf("%02X,",*(pData+i));
        mtk_nfc_sys_dbg_to_file("%02X,",*(pData+i));
    }

    return;
}

#ifdef WIN32
VOID CALLBACK nfc_timer_expiry_hdlr(
    HWND hwnd,        // handle to window for timer messages
    UINT message,     // WM_TIMER message
    UINT idTimer,     // timer identifier
    DWORD dwTime)     // current system time
{
    INT32 timer_slot;
    ppCallBck_t cb_func;
    void *param;

    printf("[TIMER]nfc_timer_expiry_hdlr(): (timer_id: %d), (timetick: %d)\r\n", idTimer, GetTickCount());

    //look for timer_id of this timeout, range = 0 ~ (MTK_NFC_TIMER_MAX_NUM-1)
    for(timer_slot = 0; timer_slot < MTK_NFC_TIMER_MAX_NUM; timer_slot++)
    {
        if(nfc_timer_table[timer_slot].timer_id == (UINT32)idTimer)
        {
            break;
        }
    }

    if(timer_slot == MTK_NFC_TIMER_MAX_NUM)    //timer not found in table
    {
        printf("[TIMER]timer no found in the table : (idTimer: %d)\r\n", idTimer);
        return;
    }

    //get the cb and param from gps timer pool
    EnterCriticalSection(&(nfc_timer_table[timer_slot].cs));
    cb_func = nfc_timer_table[timer_slot].timer_expiry_callback;
    param = nfc_timer_table[timer_slot].timer_expiry_context;
    LeaveCriticalSection(&(nfc_timer_table[timer_slot].cs));

    //stop time (windows timer is periodic timer)
    mtk_nfc_sys_timer_stop(timer_slot);

    //execute cb
    (*cb_func)(timer_slot, param);

    return;
}
#else
#ifdef USE_SIGNAL_EVENT_TO_TIMER_CREATE
VOID nfc_timer_expiry_hdlr (int sig, siginfo_t *si, void *uc)
{
    INT32 timer_slot;
    timer_t *tidp;
    ppCallBck_t cb_func;
    VOID *param;

    tidp = si->si_value.sival_ptr;

#if 1 // debug by hiki
    {
        char time_tag[32];
        MTK_TIME_T tSysTime;
        mtk_nfc_sys_time_read(&tSysTime);
        sprintf(time_tag, "[%02d:%02d:%02d.%03d] ", tSysTime.hour, tSysTime.min, tSysTime.sec, tSysTime.msec);
        mtk_nfc_sys_dbg_to_file(time_tag);
    }
#endif


    //  mtk_nfc_sys_dbg_to_file("\r\n\n================== Timeout Signal ========================\r\n");
#if 0 // debug by hiki
    mtk_nfc_sys_dbg_to_file("[TIMER]Caugh signal %d, (handle: 0x%x)\n", sig, ((tidp!=NULL) ? (int)*tidp : (int)NULL));
#endif

//    mtk_nfc_sys_mutex_take(MTK_NFC_MUTEX_TIMER);

    /* Look up timer_slot of this timeout, range = 0 ~ (MTK_NFC_TIMER_MAX_NUM-1) */
    for (timer_slot = 0; timer_slot < MTK_NFC_TIMER_MAX_NUM; timer_slot++)
    {
        if ( ( nfc_timer_table[timer_slot].is_used == TRUE ) &&
             ( nfc_timer_table[timer_slot].handle == *tidp ) )
        {
            break;
        }
    }

    if (timer_slot == MTK_NFC_TIMER_MAX_NUM)    //timer not found in table
    {
        mtk_nfc_sys_dbg_to_file("[TIMER]timer no found in the table : (handle: 0x%x)\r\n", nfc_timer_table[timer_slot].handle);
//        mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);
        return;
    }
#if 1 // debug by hiki    
    mtk_nfc_sys_dbg_to_file("[TIMER] expired ID is timer_slot(%d)\n", timer_slot);
#endif
    //get the cb and param from gps timer pool
    cb_func = nfc_timer_table[timer_slot].timer_expiry_callback;
    param = nfc_timer_table[timer_slot].timer_expiry_context;

//    mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);

    //stop time (windows timer is periodic timer)
    mtk_nfc_sys_timer_stop(timer_slot);

    //execute cb
    if (cb_func != NULL)
    {
        (*cb_func)(timer_slot, param);
    }
    else
    {
        mtk_nfc_sys_dbg_to_file("[TIMER] invalid callback\n");
    }
#if 0 // debug by hiki
    mtk_nfc_sys_dbg_to_file("================== Timeout Signal - END ========================\r\n\n");
#endif

}
#else
VOID nfc_timer_expiry_hdlr ( union sigval sv )
{
    UINT32 timer_slot = (UINT32)(sv.sival_int);
    ppCallBck_t cb_func;
    VOID *param;

    if (timer_slot >= MTK_NFC_TIMER_MAX_NUM)
    {
        mtk_nfc_sys_dbg_to_file("nfc_timer_expiry_hdlr : timer_slot(%d) exceed max num of nfc timer\r\n", timer_slot);
        return;
    }

    if (nfc_timer_table[timer_slot].is_stopped == TRUE)
    {
        mtk_nfc_sys_dbg_to_file("nfc_timer_expiry_hdlr : timer_slot(%d) expired but already stopped\r\n", timer_slot);
        return;
    }

    // get the cb and param from nfc timer pool
    cb_func = nfc_timer_table[timer_slot].timer_expiry_callback;
    param = nfc_timer_table[timer_slot].timer_expiry_context;

    // stop timer
    mtk_nfc_sys_timer_stop(timer_slot);

    // execute cb
    (*cb_func)(timer_slot, param);
}
#endif
#endif

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_timer_init
 * DESCRIPTION
 *  Create a new timer
 * PARAMETERS
 *  NONE
 * RETURNS
 *  a valid timer ID or MTK_NFC_TIMER_INVALID_ID if an error occured
 *****************************************************************************/
INT32
mtk_nfc_sys_timer_init (
    VOID
)
{
    int ret;
    int timer_slot;

    struct sigaction sa;

    /* Establish handler for timer signal */
    mtk_nfc_sys_dbg_to_file("Establishing handler for signal %d\n", SIG);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = nfc_timer_expiry_hdlr;
    sigemptyset(&sa.sa_mask);

    ret = sigaction(SIG, &sa, NULL);
    if (ret == -1)
    {
        mtk_nfc_sys_dbg_to_file("sigaction fail\r\n");
    }

    /* Initialize timer pool */
    for (timer_slot = 0; timer_slot < MTK_NFC_TIMER_MAX_NUM; timer_slot++)
    {
        nfc_timer_table[timer_slot].is_used = FALSE;
        nfc_timer_table[timer_slot].handle  = 0;
        nfc_timer_table[timer_slot].is_used = FALSE;
        nfc_timer_table[timer_slot].timer_expiry_callback = NULL;
        nfc_timer_table[timer_slot].timer_expiry_context = NULL;
        nfc_timer_table[timer_slot].is_stopped = TRUE;
    }

    return ret;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_timer_create
 * DESCRIPTION
 *  Create a new timer
 * PARAMETERS
 *  selected_timer  [IN] select the timer slot
 * RETURNS
 *  a valid timer ID or MTK_NFC_TIMER_INVALID_ID if an error occured
 *****************************************************************************/
UINT32
mtk_nfc_sys_timer_create (
    MTK_NFC_TIMER_E selected_timer
)
{
#ifdef USE_SIGNAL_EVENT_TO_TIMER_CREATE
    INT32 ret;
    UINT32 timer_slot;
#if 0
    sigset_t mask;
#endif
    struct sigevent se;

//    mtk_nfc_sys_mutex_take(MTK_NFC_MUTEX_TIMER);

    if (selected_timer >= MTK_NFC_TIMER_MAX_NUM)
    {
#ifdef DEBUG_LOG
        mtk_nfc_sys_dbg_to_file("[TIMER]Invalid timer request %d\r\n", (UINT32)selected_timer);
#endif
//        mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);
        return MTK_NFC_TIMER_INVALID_ID;
    }


    timer_slot = selected_timer;
    if(nfc_timer_table[timer_slot].is_used == TRUE)
    {
#ifdef DEBUG_LOG
        mtk_nfc_sys_dbg_to_file("[TIMER]timer already created\r\n");
#endif
//        mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);
        return timer_slot;
    }

    /* Block timer signal temporarily */
#if 0
    printf("Block signal %d\n", SIG);
    sigemptyset(&mask);
    sigaddset(&mask, SIG);
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
    {
        printf("sigprocmask fail\r\n");
        return;
    }
#endif

    /* Create the timer */
    se.sigev_notify = SIGEV_SIGNAL;
    se.sigev_signo = SIG;
    se.sigev_value.sival_ptr = &nfc_timer_table[timer_slot].handle;

    /* Create a POSIX per-process timer */
    if ((ret = timer_create(CLOCKID, &se, &(nfc_timer_table[timer_slot].handle))) == -1)
    {
        mtk_nfc_sys_dbg_to_file("[TIMER]timer_create fail, slot:%d, ret:%d, errno:%d, %s\r\n", timer_slot, ret, errno, strerror(errno));
//        mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);
        return MTK_NFC_TIMER_INVALID_ID;
    }

    nfc_timer_table[timer_slot].is_used = TRUE;
    mtk_nfc_sys_dbg_to_file("[TIMER]create,time_slot,%d,handle,0x%x\r\n", timer_slot, nfc_timer_table[timer_slot].handle);

//    mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);

    return timer_slot;
#else
    INT32 ret;
    UINT32 timer_slot;
    struct sigevent se;

    se.sigev_notify = SIGEV_THREAD;
    se.sigev_notify_function = nfc_timer_expiry_hdlr;
    se.sigev_notify_attributes = NULL;

    if (selected_timer >= MTK_NFC_TIMER_MAX_NUM)
    {
#ifdef DEBUG_LOG
        mtk_nfc_sys_dbg_to_file("[TIMER]Invalid timer request %d\r\n", (UINT32)selected_timer);
#endif
        return MTK_NFC_TIMER_INVALID_ID;
    }


    timer_slot = selected_timer;
    if(nfc_timer_table[timer_slot].is_used == TRUE)
    {
#ifdef DEBUG_LOG
        mtk_nfc_sys_dbg_to_file("[TIMER]timer already created\r\n");
#endif
        return timer_slot;
    }

    se.sigev_value.sival_int = (int) timer_slot;

    /* Create a POSIX per-process timer */
    mtk_nfc_sys_dbg_to_file("handle1:%x\r\n", nfc_timer_table[timer_slot].handle);
    if ((ret = timer_create(CLOCK_REALTIME, &se, &(nfc_timer_table[timer_slot].handle))) == -1)
    {
        mtk_nfc_sys_dbg_to_file("[TIMER]timer_create fail, ret:%d, errno:%d, %s\r\n", ret, errno, strerror(errno));
        mtk_nfc_sys_dbg_to_file("handle2:%x\r\n", nfc_timer_table[timer_slot].handle);
        return MTK_NFC_TIMER_INVALID_ID;
    }

    nfc_timer_table[timer_slot].is_used = TRUE;
    mtk_nfc_sys_dbg_to_file("[TIMER]create,time_slot,%d\r\n", timer_slot);

    return timer_slot;
#endif
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_timer_start
 * DESCRIPTION
 *  Start a timer
 * PARAMETERS
 *  timer_slot  [IN] a valid timer slot
 *  period      [IN] expiration time in milliseconds
 *  timer_expiry[IN] callback to be called when timer expires
 *  arg         [IN] callback fucntion parameter
 * RETURNS
 *  NONE
 *****************************************************************************/
VOID
mtk_nfc_sys_timer_start (
    UINT32      timer_slot,
    UINT32      period,
    ppCallBck_t timer_expiry,
    VOID        *arg
)
{
    struct itimerspec its;

    if (timer_slot >= MTK_NFC_TIMER_MAX_NUM)
    {
        mtk_nfc_sys_dbg_to_file("[TIMER]timer_slot(%d) exceed max num of nfc timer\r\n", timer_slot);
        return;
    }

    if (timer_expiry == NULL)
    {
        mtk_nfc_sys_dbg_to_file("[TIMER]timer_expiry_callback == NULL\r\n");
        return;
    }

//    mtk_nfc_sys_mutex_take(MTK_NFC_MUTEX_TIMER);

    if (nfc_timer_table[timer_slot].is_used == FALSE)
    {
        mtk_nfc_sys_dbg_to_file("[TIMER]timer_slot(%d) didn't be created\r\n", timer_slot);
//        mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);
        return;
    }
#if 1 // debug by hiki
    {
        char time_tag[32];
        MTK_TIME_T tSysTime;
        mtk_nfc_sys_time_read(&tSysTime);
        sprintf(time_tag, "[%02d:%02d:%02d.%03d] ", tSysTime.hour, tSysTime.min, tSysTime.sec, tSysTime.msec);
        mtk_nfc_sys_dbg_to_file(time_tag);
    }
#endif

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = period / 1000;
    its.it_value.tv_nsec = 1000000 * (period % 1000);
    if ((its.it_value.tv_sec == 0) && (its.it_value.tv_nsec == 0))
    {
        // this would inadvertently stop the timer (TODO: HIKI)
        its.it_value.tv_nsec = 1;
    }

    nfc_timer_table[timer_slot].timer_expiry_callback = timer_expiry;
    nfc_timer_table[timer_slot].timer_expiry_context = arg;
    nfc_timer_table[timer_slot].is_stopped = FALSE;
    timer_settime(nfc_timer_table[timer_slot].handle, 0, &its, NULL);

    mtk_nfc_sys_dbg_to_file("[TIMER]timer_slot(%d) start, handle(0x%x),%d, %lu, %lu\r\n", timer_slot, nfc_timer_table[timer_slot].handle,period, its.it_value.tv_sec, its.it_value.tv_nsec);

//    mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_timer_stop
 * DESCRIPTION
 *  Start a timer
 * PARAMETERS
 *  timer_slot    [IN] a valid timer slot
 * RETURNS
 *  NONE
 *****************************************************************************/
VOID
mtk_nfc_sys_timer_stop (
    MTK_NFC_TIMER_E timer_slot
)
{
    struct itimerspec its = {{0, 0}, {0, 0}};

    if (timer_slot >= MTK_NFC_TIMER_MAX_NUM)
    {
        mtk_nfc_sys_dbg_to_file("[TIMER]timer_slot(%d) exceed max num of nfc timer\r\n", timer_slot);
        return;
    }

//    mtk_nfc_sys_mutex_take(MTK_NFC_MUTEX_TIMER);

    if (nfc_timer_table[timer_slot].is_used == FALSE)
    {
        mtk_nfc_sys_dbg_to_file("[TIMER]timer_slot(%d) already be deleted\r\n", timer_slot);
//        mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);
        return;
    }

#if 1 // debug by hiki
    {
        char time_tag[32];
        MTK_TIME_T tSysTime;
        mtk_nfc_sys_time_read(&tSysTime);
        sprintf(time_tag, "[%02d:%02d:%02d.%03d] ", tSysTime.hour, tSysTime.min, tSysTime.sec, tSysTime.msec);
        mtk_nfc_sys_dbg_to_file(time_tag);
    }
#endif

    if (nfc_timer_table[timer_slot].is_stopped == TRUE)
    {

        timer_settime(nfc_timer_table[timer_slot].handle, 0, &its, NULL);

        mtk_nfc_sys_dbg_to_file("[TIMER]timer_slot(%d) already be stopped\r\n", timer_slot);
//        mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);
        return;
    }

    timer_settime(nfc_timer_table[timer_slot].handle, 0, &its, NULL);
    nfc_timer_table[timer_slot].is_stopped = TRUE;

    mtk_nfc_sys_dbg_to_file("[TIMER]timer_slot(%d) stop, handle(0x%x)\r\n", timer_slot, nfc_timer_table[timer_slot].handle);

//    mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_timer_delete
 * DESCRIPTION
 *  Delete a timer
 * PARAMETERS
 *  timer_slot    [IN] a valid timer slot
 * RETURNS
 *  NONE
 *****************************************************************************/
VOID
mtk_nfc_sys_timer_delete (
    MTK_NFC_TIMER_E timer_slot
)
{
    struct itimerspec its = {{0, 0}, {0, 0}};

    if (timer_slot >= MTK_NFC_TIMER_MAX_NUM)
    {
        mtk_nfc_sys_dbg_to_file("[TIMER]exceed max num of nfc timer,%d\r\n", timer_slot);
        return;
    }

//    mtk_nfc_sys_mutex_take(MTK_NFC_MUTEX_TIMER);

    if (nfc_timer_table[timer_slot].is_used == FALSE)
    {
        mtk_nfc_sys_dbg_to_file("[TIMER]timer_slot(%d) already be deleted\r\n", timer_slot);
//        mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);
        return;
    }
    timer_settime(nfc_timer_table[timer_slot].handle, 0, &its, NULL);
    timer_delete(nfc_timer_table[timer_slot].handle);
    nfc_timer_table[timer_slot].handle = 0;
    nfc_timer_table[timer_slot].timer_expiry_callback = NULL;
    nfc_timer_table[timer_slot].timer_expiry_context = NULL;
    nfc_timer_table[timer_slot].is_used = FALSE; // clear used flag
    nfc_timer_table[timer_slot].is_stopped = TRUE;
    mtk_nfc_sys_dbg_to_file("[TIMER]timer_slot(%d) delete\r\n", timer_slot);

//    mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_TIMER);
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_sleep
 * DESCRIPTION
 *  task sleep funciton
 * PARAMETERS
 *  pString     [IN] data Data block
 *  length      [IN] size buffer size of the data block
 * RETURNS
 *  VOID
 *****************************************************************************/
VOID
mtk_nfc_sys_sleep (
    UINT32 u4MilliSeconds
)
{
#ifdef WIN32
    Sleep(u4MilliSeconds);
#else // for linux
    usleep(u4MilliSeconds*1000);
#endif

    return;
}

VOID
mtk_nfc_sys_assert (
    INT32 value
)
{
#ifdef WIN32
    assert(value);
#else // for linux
    // not implement yet
#endif

    return;
}

/*****************************************************************************
 * FUNCTION
 *  mtk_sys_time_read
 * DESCRIPTION
 *  Read system time
 * PARAMETERS
 *  utctime     [IN/OUT] get the host system time
 * RETURNS
 *  success (MTK_GPS_SUCCESS)
 *  failed (MTK_GPS_ERROR)
 *  system time changed since last call (MTK_GPS_ERROR_TIME_CHANGED)
 *****************************************************************************/
INT32
mtk_nfc_sys_time_read (MTK_TIME_T* utctime)
{
#ifdef WIN32
    SYSTEMTIME systime;

    GetLocalTime(&systime);
    utctime->year = systime.wYear - 1900;
    utctime->month = systime.wMonth - 1;
    utctime->mday = (unsigned char)systime.wDay;
    utctime->hour = (unsigned char)systime.wHour;
    utctime->min = (unsigned char)systime.wMinute;
    utctime->sec = (unsigned char)systime.wSecond;
    utctime->msec = systime.wMilliseconds;

    return  MTK_NFC_SUCCESS;
#else
    struct timeval tv;
    struct tm *p_tm;

    gettimeofday(&tv, NULL);
    p_tm = localtime(&tv.tv_sec);

    utctime->year   = p_tm->tm_year;
    utctime->month  = p_tm->tm_mon;
    utctime->mday   = p_tm->tm_mday;
    utctime->hour   = p_tm->tm_hour;
    utctime->min    = p_tm->tm_min;
    utctime->sec    = p_tm->tm_sec;
    utctime->msec   = tv.tv_usec / 1000;

    return  MTK_NFC_SUCCESS;
#endif
}

#ifdef WIN32 // PHY layer
/* ***************************************************************************
Physical Link Function
    gLinkFunc.init  = mtkNfcDal_uart_init;
    gLinkFunc.open = mtkNfcDal_uart_open;
    gLinkFunc.close = mtkNfcDal_uart_close;
    gLinkFunc.read  = mtkNfcDal_uart_read;
    gLinkFunc.write  = mtkNfcDal_uart_write;
    gLinkFunc.flush  = mtkNfcDal_uart_flush;
    gLinkFunc.reset = mtkNfcDal_chip_reset;    „³ GPIO control for NFC pins
UART
    void mtkNfcDal_uart_init (void);
    NFCSTATUS mtkNfcDal_uart_open (const char* deviceNode, void ** pLinkHandle)
    int mtkNfcDal_uart_read (uint8_t * pBuffer, int nNbBytesToRead);
    int mtkNfcDal_uart_write (uint8_t * pBuffer, int nNbBytesToWrite);
    void mtkNfcDal_uart_flush (void);
    void mtkNfcDal_uart_close (void);
GPIO
    int mtkNfcDal_chip_reset (int level);
** ************************************************************************ */

extern HANDLE g_hUart;

// UART settings for Windows UART driver
#define NFC_UART_BAUD                   (115200)
#define NFC_UART_BUF_TX                 (1024)
#define NFC_UART_BUF_RX                 (1024)

VOID *
mtk_nfc_sys_uart_init (
    const CH* strDevPortName,
    const INT32 i4Baud
)
{
    HANDLE hUARTHandle = INVALID_HANDLE_VALUE;

    hUARTHandle = CreateFile(strDevPortName, GENERIC_READ | GENERIC_WRITE,
                             0, NULL, OPEN_EXISTING, 0, NULL);

    if (INVALID_HANDLE_VALUE != hUARTHandle)
    {
        DCB dcb;
        BOOL fSuccess;

        fSuccess = GetCommState(hUARTHandle, &dcb);
        if (fSuccess)
        {
            dcb.BaudRate = i4Baud;
            dcb.ByteSize = 8;
            dcb.Parity = NOPARITY;
            dcb.StopBits = ONESTOPBIT;
            dcb.fOutxDsrFlow = FALSE;
            dcb.fOutxCtsFlow = FALSE;
            dcb.fDtrControl = DTR_CONTROL_DISABLE;
            dcb.fRtsControl = RTS_CONTROL_ENABLE;
            dcb.fInX = FALSE;           // No Xon/Xoff flow control
            dcb.fOutX = FALSE;
            dcb.fBinary = TRUE;
            dcb.fAbortOnError = FALSE;  // Do not abort reads/writes on error
            dcb.fErrorChar = FALSE;     // Disable error replacement
            dcb.fNull = FALSE;          // Disable null stripping

            fSuccess = SetCommState(hUARTHandle, &dcb);

            if (fSuccess)
            {
                COMMTIMEOUTS timeouts;

                // setup device buffer
                SetupComm(hUARTHandle, NFC_UART_BUF_RX, NFC_UART_BUF_TX);

                // setup timeout
                GetCommTimeouts(hUARTHandle, &timeouts);
                timeouts.ReadIntervalTimeout = MAXDWORD;
                timeouts.ReadTotalTimeoutConstant = 0;
                timeouts.ReadTotalTimeoutMultiplier = 0;
                timeouts.WriteTotalTimeoutConstant = 0;
                timeouts.WriteTotalTimeoutMultiplier = 0;
                SetCommTimeouts(hUARTHandle, &timeouts);
            }
        }

        if (!fSuccess)
        {
            CloseHandle(hUARTHandle);
            hUARTHandle = INVALID_HANDLE_VALUE;
        }
    }

    return hUARTHandle;
}

INT32
mtk_nfc_sys_uart_read (
//    VOID *pLinkHandle,
    UINT8 *pBuffer,
    UINT16 nNbBytesToRead
)
{
    DWORD dwRead = 0;

    if (INVALID_HANDLE_VALUE != g_hUart)
    {
        if (ReadFile(g_hUart, pBuffer, nNbBytesToRead, (LPDWORD)&dwRead, NULL))
        {
            // read success - one shot read and return
        }
        else
        {
            //assert(0);
            dwRead = -1;
        }
    }
    else
    {
        mtk_nfc_sys_dbg_string("UART Handle is invalid\r\n");
        dwRead = -2;
    }

    return dwRead;
}

INT32
mtk_nfc_sys_uart_write (
//    VOID *pLinkHandle,
    UINT8 *pBuffer,
    UINT16 nNbBytesToWrite
)
{
    DWORD dwWritten = 0;
    UINT32 u4Offset = 0;

#ifdef DBG_INTERVAL_BY_TAB
    mtk_nfc_sys_dbg_string("\t\t\t\t\t\t ---> PHY TX: ");
#else
    mtk_nfc_sys_dbg_string("             ---> PHY TX: ");
#endif
    mtk_nfc_sys_dbg_trace(pBuffer, nNbBytesToWrite);
    mtk_nfc_sys_dbg_string("\r\n");

    if (INVALID_HANDLE_VALUE != g_hUart)
    {
        while (u4Offset < nNbBytesToWrite)
        {
            if (WriteFile(g_hUart, &pBuffer[u4Offset], nNbBytesToWrite - u4Offset, &dwWritten, NULL))
            {
                // write success - continuely write if the write data is not completed
                u4Offset += dwWritten;
            }
            else
            {
                //assert(0);
                break;
            }
        }
    }
    else
    {
        mtk_nfc_sys_dbg_string("UART Handle is invalid\r\n");
    }

    return dwWritten;
}

VOID
mtk_nfc_sys_uart_flush (
    VOID *pLinkHandle
)
{
    // purge any information in buffer
    PurgeComm(pLinkHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
}

// uninit UART
VOID
mtk_nfc_sys_uart_uninit (
    VOID *pLinkHandle
)
{
    if (INVALID_HANDLE_VALUE != pLinkHandle)
    {
        CloseHandle(pLinkHandle);
    }
}
#else // for Linux
VOID *
mtk_nfc_sys_i2c_init (
    const CH *strDevPort,
    const INT32 i4Baud  // no used in i2c
)
{
    int ret;

    if ( strDevPort == NULL )
    {
        printf("mtk_nfc_sys_i2c_init fail, strDevPort is null\n");
        ret = -1;
    }
    else
    {
        gInterfaceHandle = open(strDevPort, O_RDWR | O_NOCTTY);
        if (gInterfaceHandle < 0)
        {
            printf("nfc device open fail, fd: %d\n", gInterfaceHandle);
            ret = -1;
        }
        else
        {
            //     printf("nfc device open ok, fd: %d\n", gInterfaceHandle);
            ret = gInterfaceHandle;
        }
    }

    return (VOID *)ret; // cast to void pointer to prevent build error
}

INT32
mtk_nfc_sys_i2c_read (
    UINT8 *pBuffer,
    UINT16 nNbBytesToRead
)
{
    int ret = MTK_NFC_ERROR;

    if (gInterfaceHandle < 0)
    {
        mtk_nfc_sys_dbg_to_file("nfc device handle is invalil, fd: %d\n", gInterfaceHandle);
    }
    else
    {
        ret = read(gInterfaceHandle, pBuffer, nNbBytesToRead);
    }

    return ret;
}

INT32
mtk_nfc_sys_i2c_write (
    UINT8 *pBuffer,
    UINT16 nNbBytesToWrite
)
{
    int ret = MTK_NFC_ERROR;
#if 1 // debug by hiki
    int i, length = 0;
    UINT8 Buf[1024];
#endif

#if 1 // debug by hiki
    {
        char time_tag[32];
        MTK_TIME_T tSysTime;
        mtk_nfc_sys_time_read(&tSysTime);
        sprintf(time_tag, "[%02d:%02d:%02d.%03d] ", tSysTime.hour, tSysTime.min, tSysTime.sec, tSysTime.msec);
        mtk_nfc_sys_dbg_to_file(time_tag);
    }
#endif

#if 1 // debug by hiki
    memset(Buf, 0x00, 1024);
    snprintf(Buf, 1024, "[TX],");
    length = strlen(Buf);
    for ( i = 0 ; (i < nNbBytesToWrite) && (length < 1024); i++)
    {
        snprintf((Buf+length), (1024-length), "%02x,", (unsigned char)pBuffer[i]);
        length = strlen(Buf);
    }
    snprintf((Buf+length), (1024-length), "\n");
    mtk_nfc_sys_dbg_to_file("%s", Buf );
#endif

    if (gInterfaceHandle < 0)
    {
        mtk_nfc_sys_dbg_to_file("nfc device handle is invalil, fd: %d\n", gInterfaceHandle);
    }
    else
    {
        ret = write(gInterfaceHandle, pBuffer, nNbBytesToWrite);
    }

    return ret;
}

VOID
mtk_nfc_sys_i2c_flush (
    VOID *pLinkHandle
)
{
}

VOID
mtk_nfc_sys_i2c_uninit (
    VOID *pLinkHandle
)
{
    if (gInterfaceHandle != -1)
    {
        close(gInterfaceHandle);
        gInterfaceHandle = -1;
    }
}

VOID
mtk_nfc_sys_i2c_gpio_write(
    MTK_NFC_GPIO_E ePin,
    MTK_NFC_PULL_E eHighLow
)
{
    int ret;
    MTK_NFC_IOCTL_ARG_T ioctl_arg;

    if (gInterfaceHandle < 0)
    {
        printf("nfc device handle is invalid, fd: %d\n", gInterfaceHandle);
    }
    else
    {
        ioctl_arg.pin = ePin;
        ioctl_arg.highlow = eHighLow;
        //   printf("mtk_nfc_sys_i2c_gpio_write, ePin: %d, highlow: %d\n", ePin, eHighLow);
        ret = ioctl(gInterfaceHandle, MTK_NFC_IOCTL_WRITE, &ioctl_arg);
        if (MTK_NFC_SUCCESS != ret)
        {
            printf("nfc device ioctl fail, ret: %d\n", ret);
        }
    }
}

MTK_NFC_PULL_E
mtk_nfc_sys_i2c_gpio_read(
    MTK_NFC_GPIO_E ePin
)
{
    int ret;
    MTK_NFC_IOCTL_ARG_T ioctl_arg;

    if (gInterfaceHandle < 0)
    {
        printf("nfc device handle is invalid, fd: %d\n", gInterfaceHandle);
    }
    else
    {
        ioctl_arg.pin = ePin;
        ret = ioctl(gInterfaceHandle, MTK_NFC_IOCTL_READ, &ioctl_arg);
        if (MTK_NFC_SUCCESS != ret)
        {
            printf("nfc device ioctl fail, ret: %d\n", ret);
        }
    }

    return ioctl_arg.highlow;
}
#endif

/*****************************************************************************
 * FUNCTION
 *  mtk_nfc_sys_interface_init
 * DESCRIPTION
 *  Initialize communication interface between DH and NFCC
 * PARAMETERS
 *  strDevPortName      [IN] Device Name
 *  i4Baud              [IN] Baudrate
 * RETURNS
 *  NONE
 *****************************************************************************/
VOID *
mtk_nfc_sys_interface_init (
    const CH* strDevPortName,
    const INT32 i4Baud
)
{
#ifdef WIN32
    return mtk_nfc_sys_uart_init(strDevPortName, i4Baud);
#else // for Linux
    return mtk_nfc_sys_i2c_init(strDevPortName, i4Baud);
#endif
}

/*****************************************************************************
 * FUNCTION
 *  mtk_nfc_sys_interface_read
 * DESCRIPTION
 *  Read data from NFCC
 * PARAMETERS
 *  pBuffer             [IN] read buffer
 *  nNbBytesToRead      [IN] number of bytes to read
 * RETURNS
 *  number of bytes read
 *****************************************************************************/
INT32
mtk_nfc_sys_interface_read (
    UINT8 *pBuffer,
    UINT16 nNbBytesToRead
)
{
#ifdef WIN32
    return mtk_nfc_sys_uart_read(pBuffer, nNbBytesToRead);
#else // for Linux
    return mtk_nfc_sys_i2c_read(pBuffer, nNbBytesToRead);
#endif
}

/*****************************************************************************
 * FUNCTION
 *  mtk_nfc_sys_interface_write
 * DESCRIPTION
 *  Write data to NFCC
 * PARAMETERS
 *  pBuffer             [IN] write buffer
 *  nNbBytesToWrite     [IN] number of bytes to write
 * RETURNS
 *  number of bytes written
 *****************************************************************************/
INT32
mtk_nfc_sys_interface_write (
    UINT8 *pBuffer,
    UINT16 nNbBytesToWrite
)
{
#ifdef WIN32
    return mtk_nfc_sys_uart_write(pBuffer, nNbBytesToWrite);
#else // for Linux
    return mtk_nfc_sys_i2c_write(pBuffer, nNbBytesToWrite);
#endif
}

/*****************************************************************************
 * FUNCTION
 *  mtk_nfc_sys_interface_flush
 * DESCRIPTION
 *  Flush communication interface
 * PARAMETERS
 *  pLinkHandle         [IN] Link Handle
 * RETURNS
 *  NONE
 *****************************************************************************/
VOID
mtk_nfc_sys_interface_flush (
    VOID *pLinkHandle
)
{
#ifdef WIN32
    mtk_nfc_sys_uart_flush(pLinkHandle);
#else // for Linux
    mtk_nfc_sys_i2c_flush(pLinkHandle);
#endif
}

/*****************************************************************************
 * FUNCTION
 *  mtk_nfc_sys_interface_uninit
 * DESCRIPTION
 *  mt6605 gpio config
 * PARAMETERS
 *  pLinkHandle         [IN] Link Handle
 * RETURNS
 *  NONE
 *****************************************************************************/
VOID
mtk_nfc_sys_interface_uninit (
    VOID *pLinkHandle
)
{
#ifdef WIN32
    mtk_nfc_sys_uart_uninit(pLinkHandle);
#else // for Linux
    mtk_nfc_sys_i2c_uninit(pLinkHandle);
#endif
}


/*****************************************************************************
 * FUNCTION
 *  mtk_nfc_sys_gpio_write
 * DESCRIPTION
 *  mt6605 gpio config
 * PARAMETERS
 *  ePin        [IN] GPIO PIN
 *  eHighLow    [IN] High or How
 * RETURNS
 *  NONE
 *****************************************************************************/
VOID
mtk_nfc_sys_gpio_write(
    MTK_NFC_GPIO_E ePin,
    MTK_NFC_PULL_E eHighLow
)
{
#ifdef WIN32
    // Not Support on PC platform
#else // for Linux
    mtk_nfc_sys_i2c_gpio_write(ePin, eHighLow);
#endif
}

/*****************************************************************************
 * FUNCTION
 *  mtk_nfc_sys_gpio_read
 * DESCRIPTION
 *  mt6605 gpio config
 * PARAMETERS
 *  ePin        [IN] GPIO PIN
 * RETURNS
 *  MTK_NFC_PULL_E
 *****************************************************************************/
MTK_NFC_PULL_E
mtk_nfc_sys_gpio_read(
    MTK_NFC_GPIO_E ePin
)
{
#ifdef WIN32
    // Not Support on PC platform
    return MTK_NFC_PULL_HIGH;
#else // for Linux
    return mtk_nfc_sys_i2c_gpio_read(ePin);
#endif
}

/*****************************************************************************
 * FUNCTION
 *  mtk_nfc_sys_dbg_to_file
 * DESCRIPTION
 *  Read system time
 * PARAMETERS
 *  data        [IN] debug output format
 *  ...         [IN] variable parameters
 * RETURNS
 *  NONE
 *****************************************************************************/
VOID
mtk_nfc_sys_dbg_to_file (const CH *data, ...)
{
#ifdef WIN32

    CH szBuf[DBG_STRING_MAX_SIZE];
    DWORD dwWritten = 0;

    va_start(args, data);
    vsnprintf(szBuf, DBG_STRING_MAX_SIZE, data, args);
    va_end(args);

    if (INVALID_HANDLE_VALUE != g_hLogFile)
    {
        sprintf(szBuf, "%s", szBuf);
        WriteFile(g_hLogFile, &szBuf[0], (DWORD)strlen(szBuf), &dwWritten, NULL);
    }

#else
    CH szBuf[DBG_STRING_MAX_SIZE];
    va_list  args;
    unsigned int dwWritten = 0, length = 0;

    mtk_nfc_sys_mutex_take(MTK_NFC_MUTEX_RESERVE_1);


    memset(szBuf, 0x00, DBG_STRING_MAX_SIZE);
    va_start(args, data);
    length = strlen(szBuf);
    vsnprintf((szBuf-length), (DBG_STRING_MAX_SIZE-length), data, args);
    va_end(args);

    length = strlen(szBuf);

    if ((length != 0) && (length < DBG_STRING_MAX_SIZE))
    {
        dwWritten = fwrite ( &szBuf[0], length, 1, g_dbg_fp);
    }
    else
    {
        LOGD("dbg_to_file,(%d)\r\n", length);
    }
    mtk_nfc_sys_mutex_give(MTK_NFC_MUTEX_RESERVE_1);
#endif
}

/*****************************************************************************
 * Function
 *  mtk_nfc_sys_dbg_trx_to_file
 * DESCRIPTION
 *  output debug message for NFC Nurse
 * PARAMETERS
 *  fgIsTx      [IN] Tx or Rx message
 *  pString     [IN] data Data block
 *  length      [IN] size buffer size of the data block
 * RETURNS
 *  VOID
 *

    Format
    #123456 TX XXXXX &CS ? XXXX should be HEX
    #123457 RX XXXXX &CS

    XXXXX ? DDDYYYY

    YYYY¬°¯u¥¿°e¥Xªºraw data
    DDD: ¬°connectionID record format¦p¤U
    CID_count(1B), CID_Info

    Example:
    CID_count = 0x02(1B)
    CID_Info: 0x00(1B),0x01(1B), 0x01(1B),0x00(1B)
*****************************************************************************/
VOID
mtk_nfc_sys_dbg_trx_to_file(
    BOOL    fgIsTx,
    UINT8   pData[],
    UINT32  u4Len
)
{
#ifdef WIN32
    CH      szBufTemp[DBG_STRING_MAX_SIZE_RAW_DATA];
    CH      szBuf[DBG_STRING_MAX_SIZE_RAW_DATA];
    CH      szCidInfo[DBG_STRING_MAX_SIZE_CID_INFO];
    UINT8   u1CidCount = 0;
    UINT8   u1CheckSum = 0;
    UINT32  i;
    DWORD offset = 0, dwWritten = 0;
    MTK_TIME_T tSysTime;

    if (INVALID_HANDLE_VALUE != g_hLogTrxFile)
    {
        // clear memory
        memset(szBufTemp, 0, DBG_STRING_MAX_SIZE_RAW_DATA);
        memset(szBuf    , 0, DBG_STRING_MAX_SIZE_RAW_DATA);
        memset(szCidInfo, 0, DBG_STRING_MAX_SIZE_CID_INFO);

        // step1: get system time
        mtk_nfc_sys_time_read(&tSysTime);
        sprintf(szBufTemp, "\n#%02d%02d%02d.%03d ", tSysTime.hour, tSysTime.min, tSysTime.sec, tSysTime.msec);
        WriteFile(g_hLogTrxFile, &szBufTemp[0], (DWORD)strlen(szBufTemp), &dwWritten, NULL);

        // step2: TX or RX
        sprintf(szBufTemp, "%s", (fgIsTx ? "TX " : "RX "));
        WriteFile(g_hLogTrxFile, &szBufTemp[0], (DWORD)strlen(szBufTemp), &dwWritten, NULL);

        // step3: get connection ID record format (TBD)
        u1CidCount = 1;
        szCidInfo[0] = 0x00;
        szCidInfo[1] = 0x00;
        sprintf(szBufTemp, "%02X%02X%02X", u1CidCount, szCidInfo[0], szCidInfo[1]);
        WriteFile(g_hLogTrxFile, &szBufTemp[0], (DWORD)strlen(szBufTemp), &dwWritten, NULL);

        // step4: prepare HEX raw data
        for (i = 0; i < u4Len; i++)
        {
            sprintf(szBufTemp, "%02X", *(pData+i));
            strcat(szBuf, szBufTemp);
        }
        WriteFile(g_hLogTrxFile, &szBuf[0], (DWORD)strlen(szBuf), &dwWritten, NULL);

        // step5: calculate checksum (TBD)
        sprintf(szBufTemp, " &%02x", u1CheckSum);
        WriteFile(g_hLogTrxFile, &szBufTemp[0], (DWORD)strlen(szBufTemp), &dwWritten, NULL);
    }
#endif
}

//*****************************************************************************
// File Functions
//*****************************************************************************

//*****************************************************************************
// mtk_nfc_sys_file_open : Open a file
//
// PARAMETER : szFileName [IN] - name of the file to be opened
//             i4Mode     [IN] - file access mode (read / write / read + write)
//                               0 -- open file for reading (r)
//                               1 -- create file for writing,
//                                    discard previous contents if any (w)
//                               2 -- open or create file for writing at end of file (a)
//                               3 -- open file for reading and writing (r+)
//                               4 -- create file for reading and writing,
//                                    discard previous contents if any (w+)
//                               5 -- open or create file for reading and writing at end of file (a+)
//
// NOTE : For system which treats binary mode and text mode differently,
//        such as Windows / DOS, please make sure to open file in BINARY mode
//
// RETURN : On success, return the file handle
//          If fail, return 0
NFC_FILE mtk_nfc_sys_file_open(const CHAR *szFileName, UINT32 i4Mode)
{
    FILE *fp;
    char szMode[4];

    // For system which treats binary mode and text mode differently,
    // such as Windows / DOS, please make sure to open file in BINARY mode

    switch (i4Mode)
    {
        case MTK_NFC_FS_READ:       // 0
            sprintf(szMode, "rb");
            break;
        case MTK_NFC_FS_WRITE:      // 1
            sprintf(szMode, "wb");
            break;
        case MTK_NFC_FS_APPEND:     // 2
            sprintf(szMode, "ab");
            break;
        case MTK_NFC_FS_RW:         // 3
            sprintf(szMode, "r+b");
            break;
        case MTK_NFC_FS_RW_DISCARD: // 4
            sprintf(szMode, "w+b");
            break;
        case MTK_NFC_FS_RW_APPEND:  // 5
            sprintf(szMode, "a+b");
            break;
        default:
            return 0;
    }

    fp = fopen(szFileName, szMode);

    if (fp != NULL)
    {
        return (NFC_FILE)fp;
    }

    return 0;
}

//*****************************************************************************
// mtk_nfc_sys_file_close : Close a file
//
// PARAMETER : hFile [IN] - handle of file to be closed
//
// RETURN : void
VOID mtk_nfc_sys_file_close (NFC_FILE hFile)
{
    fclose((FILE *)hFile);
}

//*****************************************************************************
// mtk_nfc_sys_file_read : Read a block of data from file
//
// PARAMETER : hFile    [IN]  - handle of file
//             DstBuf   [OUT] - pointer to data buffer to be read
//             u4Length [IN]  - number of bytes to read
//
// RETURN : Number of bytes read
UINT32 mtk_nfc_sys_file_read (NFC_FILE hFile, void *DstBuf, UINT32 u4Length)
{
    if (hFile != 0)
    {
        return (UINT32)fread(DstBuf, 1, u4Length, (FILE *)hFile);
    }

    return 0;
}

//*****************************************************************************
// mtk_nfc_sys_file_seek : Set the position indicator associated with file handle
//                     to a new position defined by adding offset to a reference
//                     position specified by origin
//
// PARAMETER : hFile    [IN] - handle of file
//             u4OffSet [IN] - number of bytes to offset from origin
//             u4Origin [IN] - position from where offset is added
//                             0 -- seek from beginning of file
//                             1 -- seek from current position
//                             2 -- seek from end of file
//
// RETURN : On success, return a zero value
//          Otherwise, return a non-zero value
UINT32 mtk_nfc_sys_file_seek (NFC_FILE hFile, UINT32 u4OffSet, UINT32 u4Origin)
{
    return fseek((FILE *)hFile, u4OffSet, u4Origin);
}

//*****************************************************************************
// mtk_nfc_sys_file_tell : Returns the current value of the position indicator of the stream.
//
// PARAMETER : hFile    [IN] - handle of file
//
// RETURN : On success, the current value of the position indicator is returned.
//               On failure, -1L is returned, and errno is set to a system-specific positive value.
UINT32 mtk_nfc_sys_file_tell (NFC_FILE hFile)
{
    return (UINT32)ftell((FILE *)hFile);
}

//*****************************************************************************
// mtk_nfc_sys_file_rewind : Set position of stream to the beginning.
//
// PARAMETER : hFile    [IN] - handle of file
//
// RETURN : none
VOID mtk_nfc_sys_file_rewind (NFC_FILE hFile)
{
    rewind((FILE *)hFile);
}


#if 1
static pthread_cond_t g_nfc_event_cond[MTK_NFC_EVENT_END];

INT32 mtk_nfc_sys_event_delete(MTK_NFC_EVENT_E event_idx)
{
    INT32 ret = MTK_NFC_SUCCESS;

    if (pthread_cond_destroy(&g_nfc_event_cond[event_idx]))
    {
        ret = MTK_NFC_ERROR;
    }

    return ret;
}

INT32 mtk_nfc_sys_event_create(MTK_NFC_EVENT_E event_idx)
{
    INT32 ret = MTK_NFC_SUCCESS;

    if (pthread_cond_init(&g_nfc_event_cond[event_idx], NULL))
    {
        ret = MTK_NFC_ERROR;
    }

    return ret;
}

INT32 mtk_nfc_sys_event_set(MTK_NFC_EVENT_E event_idx)
{
    INT32 ret = MTK_NFC_SUCCESS;

    if (pthread_cond_signal(&g_nfc_event_cond[event_idx]))
    {
        ret = MTK_NFC_ERROR;
    }

    return ret;
}

INT32 mtk_nfc_sys_event_wait(MTK_NFC_EVENT_E event_idx, MTK_NFC_MUTEX_E mutex_idx)
{
    INT32 ret = MTK_NFC_SUCCESS;

    if (pthread_cond_wait(&g_nfc_event_cond[event_idx], &g_hMutex[mutex_idx]))
    {
        ret = MTK_NFC_ERROR;
    }

    return ret;
}
#endif

