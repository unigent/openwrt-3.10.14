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
 *  linux_nfc_amin.c
 *
 * Project:
 * --------
 *
 * Description:
 * ------------
 *
 * Author:
 * -------
 *  Hiki Chen, ext 25281, hiki.chen@mediatek.com, 2012-10-12
 *
 *******************************************************************************/
/*****************************************************************************
 * Include
 *****************************************************************************/


#include "mtk_nfc_sys_type.h"
#include "mtk_nfc_sys_type_ext.h"

#include "linux_nfc_main.h"

#ifdef ADJUST_THREAD_PRIORITY
#include <pthread.h>
#include <sched.h>
#endif

/*****************************************************************************
 * Define
 *****************************************************************************/

/*****************************************************************************
 * Data Structure
 *****************************************************************************/

/*****************************************************************************
 * Function Prototype
 *****************************************************************************/
void *data_read_thread_func(void * arg);
void *nfc_main_proc_func(void * arg);
void *nfc_adapt_proc_func(void * arg);
void *nfc_client_proc_func(void * arg);

int linux_nfc_exit_thread_normal(MTK_NFC_THREAD_T *arg);
int linux_nfc_thread_active_notify(MTK_NFC_THREAD_T *arg);

/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/
static MTK_NFC_THREAD_T g_mtk_nfc_thread[THREAD_NUM] =
{
    {C_INVALID_FD, MTK_NFC_THREAD_MAIN,     C_INVALID_TID, linux_nfc_exit_thread_normal, linux_nfc_thread_active_notify},
//{C_INVALID_FD, MTK_NFC_THREAD_SERV,  C_INVALID_TID, linux_nfc_exit_thread_normal, linux_nfc_thread_active_notify},  // Use process to handle service messages in linux
    {C_INVALID_FD, MTK_NFC_THREAD_RXHDLR,     C_INVALID_TID, linux_nfc_exit_thread_normal, linux_nfc_thread_active_notify},
    {C_INVALID_FD, MTK_NFC_THREAD_ADAPT,  C_INVALID_TID, linux_nfc_exit_thread_normal, linux_nfc_thread_active_notify},
    {C_INVALID_FD, MTK_NFC_THREAD_CLIENT,  C_INVALID_TID, linux_nfc_exit_thread_normal, linux_nfc_thread_active_notify}
};

// socket fd
int g_nfc_2app_fd       = C_INVALID_FD;
int gconn_fd_tmp        = C_INVALID_FD;

FILE *g_dbg_fp = NULL;

struct sockaddr_un g_nfc_service_socket;
struct sockaddr_un g_nfc_main_socket;
struct sockaddr_un g_nfc_2app_socket;

// physical link handle
int gInterfaceHandle    = C_INVALID_FD;



// thread exit flag
static volatile int  g_ThreadExitNfcAdaptation = FALSE;
static volatile int  g_ThreadExitNfcClient = FALSE;
static volatile int g_ThreadExitReadFunc     = FALSE;
static volatile int g_ThreadExitMainProcFunc = FALSE;
static volatile int g_ThreadExitService      = FALSE;

// Test Mode
unsigned char g_NfcTsetMode = 0;

/*****************************************************************************
 * Function
 *****************************************************************************/
#ifdef ADJUST_THREAD_PRIORITY
void linux_nfc_thread_adjust_priority(char *threadname)
{
    pthread_attr_t my_attr;
    struct sched_param thread_param;
    int rr_min_priority, rr_max_priority, thread_policy;
    int status;

    printf("linux_nfc_thread_adjust_priority (%s)\r\n", threadname);

    status = pthread_attr_init(&my_attr);
    if (status!=0)
        printf("Init attr error\r\n");

    status = pthread_attr_getschedpolicy(&my_attr, &thread_policy);
    if (status!=0)
        printf("Get policy error\r\n");

    status = pthread_attr_getschedparam(&my_attr, &thread_param);
    if (status!=0)
        printf("Get sched param error\r\n");

    status = pthread_attr_setschedpolicy(&my_attr, SCHED_RR);
    if (status!=0)
        printf("Unable to set SCHED_RR policy\r\n");
    else
    {
        rr_min_priority = sched_get_priority_min(SCHED_RR);
        if (rr_min_priority == -1)
            printf("Get SCHED_RR min priority error\r\n");
        else
            printf("Get SCHED_RR min priority : %d\n", rr_min_priority);

        rr_max_priority = sched_get_priority_max(SCHED_RR);
        if (rr_max_priority == -1)
            printf("Get SCHED_RR max priority error\r\n");
        else
            printf("Get SCHED_RR max priority : %d\n", rr_max_priority);

        thread_param.sched_priority = (rr_max_priority);
        printf("SCHED_RR priority range is %d to %d : using %d\r\n", rr_min_priority, rr_max_priority, thread_param.sched_priority);

        pthread_attr_setschedparam(&my_attr, &thread_param);
        if (status != 0)
            printf("Set params error\r\n");

        status = pthread_attr_setinheritsched(&my_attr, PTHREAD_EXPLICIT_SCHED);
        if (status != 0)
            printf("Set inherit error\r\n");
    }

    printf("linux_nfc_thread_adjust_priority (%s) done\r\n", threadname);
}
#endif

int linux_nfc_exit_thread_normal(MTK_NFC_THREAD_T *arg)
{
    int err;
    void *ret;

    LOGD("linux_nfc_exit_thread_normal (taskid: %d)\n", arg->thread_id);

    if (!arg)
    {
        return MTK_NFC_ERROR;
    }

    if (arg->thread_id == MTK_NFC_THREAD_MAIN)
    {
        g_ThreadExitMainProcFunc = TRUE;
    }
    else if (arg->thread_id == MTK_NFC_THREAD_RXHDLR)
    {
        g_ThreadExitReadFunc = TRUE;
    }
    else if (arg->thread_id == MTK_NFC_THREAD_ADAPT)
    {
        g_ThreadExitNfcAdaptation = TRUE;
    }
    else if (arg->thread_id == MTK_NFC_THREAD_CLIENT)
    {
        g_ThreadExitNfcClient = TRUE;
    }

    err = pthread_join(arg->thread_handle, &ret);
    if (err)
    {
        LOGD("(%d)ThreadLeaveErr=%d \r\n",(arg->thread_id), err);
        return err;
    }
    else
    {
        LOGD("(%d)ThreadLeaveOK\r\n", (arg->thread_id));
    }
    arg->thread_handle = C_INVALID_TID;

    return 0;
}

int linux_nfc_thread_active_notify(MTK_NFC_THREAD_T *arg)
{
    LOGD("mtk_nfc_thread_active_notify\n");

    if (!arg)
    {
        // MNL_MSG("fatal error: null pointer!!\n");
        // return -1;
    }
    if (arg->snd_fd != C_INVALID_FD)
    {
        //char buf[] = {MNL_CMD_ACTIVE};
        //return mnl_sig_send_cmd(arg->snd_fd, buf, sizeof(buf));
    }
    return 0;
}

int linux_nfc_threads_create(int threadId)
{
    int result = MTK_NFC_SUCCESS;

    if ( threadId >= MTK_NFC_THREAD_END)
    {
        LOGD("linux_nfc_threads_create ERR1\n");
        result = MTK_NFC_ERROR;
    }

    if(MTK_NFC_THREAD_RXHDLR == threadId)
    {
        if (pthread_create(&g_mtk_nfc_thread[threadId].thread_handle,
                           NULL, data_read_thread_func,
                           (void*)&g_mtk_nfc_thread[threadId]))
        {
            g_ThreadExitReadFunc = TRUE;
            g_ThreadExitMainProcFunc = TRUE;
            g_ThreadExitService = TRUE;
            g_ThreadExitNfcAdaptation = TRUE;
            g_ThreadExitNfcClient = TRUE;
            LOGD("linux_nfc_threads_create ERR2\n");
            result = MTK_NFC_ERROR;
        }
    }

    if(MTK_NFC_THREAD_MAIN == threadId)
    {
        if (pthread_create(&g_mtk_nfc_thread[threadId].thread_handle,
                           NULL, nfc_main_proc_func,
                           (void*)&g_mtk_nfc_thread[threadId]))
        {
            g_ThreadExitReadFunc = TRUE;
            g_ThreadExitMainProcFunc = TRUE;
            g_ThreadExitService = TRUE;
            g_ThreadExitNfcAdaptation = TRUE;
            g_ThreadExitNfcClient = TRUE;
            LOGD("linux_nfc_threads_create ERR2\n");
            result = MTK_NFC_ERROR;
        }
    }


    if(MTK_NFC_THREAD_ADAPT == threadId)
    {
        if (pthread_create(&g_mtk_nfc_thread[threadId].thread_handle,
                           NULL, nfc_adapt_proc_func,
                           (void*)&g_mtk_nfc_thread[threadId]))
        {
            g_ThreadExitReadFunc = TRUE;
            g_ThreadExitMainProcFunc = TRUE;
            g_ThreadExitService = TRUE;
            g_ThreadExitNfcAdaptation = TRUE;
            g_ThreadExitNfcClient = TRUE;
            LOGD("linux_nfc_threads_create ERR2\n");
            result = MTK_NFC_ERROR;
        }
    }


    if(MTK_NFC_THREAD_CLIENT == threadId)
    {
        if (pthread_create(&g_mtk_nfc_thread[threadId].thread_handle,
                           NULL, nfc_client_proc_func,
                           (void*)&g_mtk_nfc_thread[threadId]))
        {
            g_ThreadExitReadFunc = TRUE;
            g_ThreadExitMainProcFunc = TRUE;
            g_ThreadExitService = TRUE;
            g_ThreadExitNfcAdaptation = TRUE;
            g_ThreadExitNfcClient = TRUE;
            LOGD("linux_nfc_threads_create ERR2\n");
            result = MTK_NFC_ERROR;
        }
    }

    return (result);
}

int linux_nfc_threads_release(void)
{
    int result = MTK_NFC_SUCCESS;
    int idx;

    LOGD("linux_nfc_threads_release...\n");

    for (idx = 0; idx < THREAD_NUM; idx++)
    {
        if (g_mtk_nfc_thread[idx].thread_handle == C_INVALID_TID)
        {
            continue;
        }
        if (!g_mtk_nfc_thread[idx].thread_exit)
        {
            continue;
        }
        if ((g_mtk_nfc_thread[idx].thread_exit(&g_mtk_nfc_thread[idx])))
        {
            result = MTK_NFC_ERROR;
            LOGD("linux_nfc_threads_release thread_exit fail\n");
        }
    }

    LOGD("linux_nfc_threads_release done\n");

    return (result);
}

int linux_nfc_software_queue_init (int* fd, unsigned char* filename, struct sockaddr_un* plocal)
{
    int ret = MTK_NFC_SUCCESS;
    int service_sock = -1;

    unlink(filename);

    if ((service_sock = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
    {
        LOGD("socket open failed\n");
        ret = MTK_NFC_ERROR;
    }
    else
    {
        memset(plocal, 0, sizeof(struct sockaddr_un));
        plocal->sun_family = AF_LOCAL;
        strcpy(plocal->sun_path, filename);


        if (bind(service_sock, (struct sockaddr *)plocal, sizeof(struct sockaddr_un)) < 0 )
        {
            LOGD("socket bind failed\n");
            close(service_sock);
            service_sock = -1;
            ret = MTK_NFC_ERROR;
        }
        else
        {
            //    int res;
            //    res = chmod(service_sock, S_IRUSR | S_IXUSR | S_IWUSR | S_IRGRP |S_IWGRP | S_IXGRP | S_IXOTH);
            //    LOGD("%s,%d..\r\n",filename, res);

            (*fd) = service_sock;
        }
    }
    return ret;
}

int linux_nfc_software_queue_deinit (int* fd)
{
    int ret = MTK_NFC_SUCCESS;

    if (fd != NULL)
    {
        if ((*fd) != -1)
        {
            close((*fd));
            (*fd) = -1;
        }
    }
    else
    {
        LOGD("fd is NULL\n");
        ret = MTK_NFC_ERROR;
    }
    return ret;
}

int linux_nfc_sys_init(void)
{
    int result = MTK_NFC_SUCCESS;

    LOGD("linux_nfc_sys_init...\n");

    // Initialize global varialbes
    g_nfc_2app_fd       = C_INVALID_FD;
    gInterfaceHandle = C_INVALID_FD;
    gconn_fd_tmp     = C_INVALID_FD;
    g_ThreadExitReadFunc     = FALSE;
    g_ThreadExitMainProcFunc = FALSE;
    g_ThreadExitService      = FALSE;
    g_ThreadExitNfcAdaptation = FALSE;
    g_ThreadExitNfcClient = FALSE;

    // Initialize system resource - mutex
    result = mtk_nfc_sys_mutex_initialize();
    if (MTK_NFC_SUCCESS != result)
    {
        LOGD("mtk_nfc_sys_mutex_initialize fail...\n");
        return (result);
    }

    // create event resource
    result = mtk_nfc_sys_event_create(MTK_NFC_EVENT_2MAIN);
    if (result != MTK_NFC_SUCCESS)
    {
        LOGD("mtk_nfc_sys_event_create,MTK_NFC_EVENT_2MAIN ERR\n");
        return (result);
    }
    result = mtk_nfc_sys_event_create(MTK_NFC_EVENT_2SERV);
    if (result != MTK_NFC_SUCCESS)
    {
        LOGD("mtk_nfc_sys_event_create,MTK_NFC_EVENT_2SERV ERR\n");
        return (result);
    }

    result = mtk_nfc_sys_event_create(MTK_NFC_EVENT_2SOCKET);
    if (result != MTK_NFC_SUCCESS)
    {
        LOGD("mtk_nfc_sys_event_create,MTK_NFC_EVENT_2SOCKET ERR\n");
        return (result);
    }


    // Initialize system resource - message queue

    result = linux_nfc_software_queue_init(&g_nfc_2app_fd, MTK_NFCD_SERVICE2APP, &g_nfc_2app_socket);
    if (MTK_NFC_SUCCESS != result)
    {
        LOGD("Create software queue for service fail...\r\n");
    }
    result = mtk_nfc_sys_msg_initialize();

    LOGD("Create software queue, %d,%s\r\n" , g_nfc_2app_fd , MTK_NFCD_SERVICE2APP);


    // Initialize physical interface
    result = (int)mtk_nfc_sys_interface_init("/dev/mt6605", 0);
    if (result < 0)
    {
        LOGD("mtk_nfc_sys_interface_init fail, error code %d\n", result);
        return (result);
    }

    // Initialize timer handler
    result = mtk_nfc_sys_timer_init();
    if (result < 0)
    {
        LOGD("mtk_nfc_sys_timer_init fail, error code %d\n", result);
        return (result);
    }

    // Create Thread (NFC RxHdlr)
    result = linux_nfc_threads_create(MTK_NFC_THREAD_RXHDLR);
    if (MTK_NFC_SUCCESS != result)
    {
        LOGD("linux_nfc_threads_create, read thread ERR\n");
        return (result);
    }

    // Create Thread (NFC Main)
    result = linux_nfc_threads_create(MTK_NFC_THREAD_MAIN);
    if (result != MTK_NFC_SUCCESS)
    {
        LOGD("linux_nfc_threads_create, main thread ERR\n");
        return (result);
    }

    // Create Thread (NFC Client)
    result = linux_nfc_threads_create(MTK_NFC_THREAD_CLIENT);
    if (result != MTK_NFC_SUCCESS)
    {
        LOGD("linux_nfc_threads_create, client thread ERR\n");
        return (result);
    }

    // Create Thread (NFC Client)
    result = linux_nfc_threads_create(MTK_NFC_THREAD_ADAPT);
    if (result != MTK_NFC_SUCCESS)
    {
        LOGD("linux_nfc_threads_create, adapt thread ERR\n");
        return (result);
    }


    LOGD("linux_nfc_sys_init done\n");

    return (result);
}

int linux_nfc_sys_deinit(void)
{
    int idx, result = MTK_NFC_SUCCESS;

    LOGD("linux_nfc_sys_deinit...\n");

    // release thread
// result = linux_nfc_threads_release();
//  if (MTK_NFC_SUCCESS != result) {
    //   LOGD("EXIT, linux_nfc_sys_deinit,%d\n",result);
    // }

    // un-initialize physical interface
    mtk_nfc_sys_interface_uninit(NULL);
    gInterfaceHandle = -1;

    // release message queue
    linux_nfc_software_queue_deinit(&gconn_fd_tmp);
    gconn_fd_tmp = -1;
    unlink (MTK_NFCD_SERVICE2APP);
    linux_nfc_software_queue_deinit(&g_nfc_2app_fd);
    g_nfc_2app_fd = -1;

    // release mutex
    for (idx = 0; idx < MTK_NFC_MUTEX_MAX_NUM; idx++)
    {
        mtk_nfc_sys_mutex_destory(idx);
    }
    // delete event resource to avoid blocking call in main & service thread
    result = mtk_nfc_sys_event_delete(MTK_NFC_EVENT_2MAIN);
    LOGD("mtk_nfc_sys_event_delete, MTK_NFC_EVENT_2MAIN,%d\n",result);
    result = mtk_nfc_sys_event_delete(MTK_NFC_EVENT_2SERV);
    LOGD("mtk_nfc_sys_event_delete, MTK_NFC_EVENT_2SERV,%d\n",result);
    result = mtk_nfc_sys_event_delete(MTK_NFC_EVENT_2SOCKET);
    LOGD("mtk_nfc_sys_event_delete, MTK_NFC_EVENT_2SOCKET,%d\n",result);

    // un-initialize global varialbes
    // - TBD
    g_NfcTsetMode = 0;

    LOGD("linux_nfc_sys_deinit done...\n");

    return result;
}

void *data_read_thread_func(void * arg)
{
    UINT32 ret = MTK_NFC_SUCCESS;
#if 1
    UINT32 i =0;
#endif

//    MTK_NFC_THREAD_T *ptr = (MTK_NFC_THREAD_T*)arg;

#ifdef ADJUST_THREAD_PRIORITY
    linux_nfc_thread_adjust_priority("data_read_thread_func");
#endif

    if (!arg)
    {
        pthread_exit(NULL);
        LOGD("data_read_thread_func, Create ERR !arg\n");
        return NULL;
    }

    LOGD("data_read_thread_func incoming...\n");

    while (!g_ThreadExitReadFunc)
    {
        int32_t i4ReadLen = 0;
        char pBuffer[DSP_UART_IN_BUFFER_SIZE];
        int8_t chReadByteOfOnce = 32;

        // blocking read
        if ((i4ReadLen = mtk_nfc_sys_interface_read(pBuffer, chReadByteOfOnce)) == MTK_NFC_ERROR)
        {
            LOGD("data_read_thread_func, read ERR\n");
            //break;
        }

        if (i4ReadLen > 0)
        {

#if 1 // debug by hiki
            {
                char time_tag[32];
                MTK_TIME_T tSysTime;
                mtk_nfc_sys_time_read(&tSysTime);
                sprintf(time_tag, "[%02d:%02d:%02d.%03d] ", tSysTime.hour, tSysTime.min, tSysTime.sec, tSysTime.msec);
                mtk_nfc_sys_dbg_to_file(time_tag);
            }
#endif
#if 1

            mtk_nfc_sys_dbg_to_file("[RX],");
            for( i =0; i < i4ReadLen; i++)
            {
                mtk_nfc_sys_dbg_to_file("%02x,",(unsigned char)pBuffer[i]);
            }
            mtk_nfc_sys_dbg_to_file("\n");
#endif
            ret = mtk_nfc_data_input(pBuffer, i4ReadLen);
            if ( ret != MTK_NFC_SUCCESS)
            {
                mtk_nfc_sys_dbg_to_file("Rx data input: fail\r\n");
            }
        }
        else
        {
            mtk_nfc_sys_sleep(10);  // sleep 10 ms
        }
        //   mtk_nfc_sys_sleep(10);  // sleep 10 ms
    }

    g_ThreadExitReadFunc = TRUE;
    pthread_exit((void *)ret);

    return NULL;
}

void *nfc_main_proc_func(void * arg)
{
    UINT32 ret = MTK_NFC_SUCCESS;
    MTK_NFC_MSG_T *nfc_msg = NULL;

#ifdef ADJUST_THREAD_PRIORITY
    linux_nfc_thread_adjust_priority("nfc_main_proc_func");
#endif

    if (!arg)
    {
        pthread_exit(NULL);
        LOGD("nfc_main_proc_func, Create ERR !arg\n");
        return NULL;
    }

    LOGD("nfc_main_proc_func incoming...\n");

    while (!g_ThreadExitMainProcFunc)
    {
        // - recv msg
        ret = mtk_nfc_sys_msg_recv(MTK_NFC_TASKID_MAIN, &nfc_msg);
        if (g_ThreadExitService == TRUE)
        {
            g_ThreadExitMainProcFunc = TRUE;
        }
        if (ret == MTK_NFC_SUCCESS && (!g_ThreadExitMainProcFunc))
        {
            mtk_nfc_main_proc(nfc_msg);
        }
        else
        {
            if (nfc_msg != NULL)
            {
                // - free msg
                mtk_nfc_sys_msg_free(nfc_msg);
            }
            // - read msg fail...
            mtk_nfc_sys_sleep(1); //add delay for non-blocking read msg

            // - continue read next msg
            continue;
        }

        // - free msg
        mtk_nfc_sys_msg_free(nfc_msg);
    }

    LOGD("nfc_main_proc_func, exit,%d,%d\n", g_ThreadExitMainProcFunc, g_ThreadExitMainProcFunc);

    g_ThreadExitMainProcFunc = TRUE;
    pthread_exit((void *)ret);

    return NULL;
}

int nfc_service_main(void)
{
    int ret = MTK_NFC_SUCCESS;
    MTK_NFC_MSG_T *nfc_msg = NULL;


    LOGD("nfc_service_main incoming...\r\n");

    while(!g_ThreadExitService)
    {
        ret = mtk_nfc_sys_msg_recv(MTK_NFC_TASKID_SERVICE, &nfc_msg);
        if ((ret == MTK_NFC_SUCCESS) && (g_ThreadExitService == FALSE))
        {
            ret  = mtk_nfc_service_proc((unsigned char*)nfc_msg);

            if (ret == MTK_NFC_ERROR)
            {
                g_ThreadExitService = TRUE;
                mtk_nfc_sys_event_set(MTK_NFC_EVENT_2MAIN);
                mtk_nfc_sys_event_set(MTK_NFC_EVENT_2SOCKET);
            }
        }
        else
        {
            if (nfc_msg != NULL)
            {
                // - free msg
                mtk_nfc_sys_msg_free(nfc_msg);
            }
            // - read msg fail...
            mtk_nfc_sys_sleep(1); //add delay for non-blocking read msg

            // - continue read next msg
            continue;
        }
        // - free msg
        mtk_nfc_sys_msg_free(nfc_msg);
    }

    LOGD("nfc_service_main exit...%d\r\n", g_ThreadExitService);

    return ret;
}

int linux_nfc_sys_run (void)
{
    int ret = 0;


    LOGD("linux_nfc_sys_run...\n");

    ret = nfc_service_main();

    LOGD("linux_nfc_sys_run exit...\n");

    return ret;
}

void *nfc_adapt_proc_func(void * arg)
{
    // - Related variables for Accept Socket
    int result = 0, size = 0;
    struct sockaddr_un client_addr;

    LOGD("nfc_adapt_proc_func\r\n");
    //----------------------------------------------------------------
    // Listen for incoming connection requests on the created socket
    //----------------------------------------------------------------
    LOGD("start listen...(g_nfc_2app_fd,%d)\r\n", g_nfc_2app_fd);
    if(listen (g_nfc_2app_fd, 5) == -1)
    {
        LOGD("listent fail\n");
        close(g_nfc_2app_fd);
        g_ThreadExitNfcAdaptation = TRUE;
        pthread_exit((void *)result);
    }

    //----------------------------------------------------------------
    // Waiting for client to connect
    //----------------------------------------------------------------
    LOGD("Waiting for client to connect...\n");
    gconn_fd_tmp = accept(g_nfc_2app_fd, (struct sockaddr*)&client_addr, &size);
    LOGD("socket accept,%d\r\n",g_nfc_2app_fd);

    //----------------------------------------------------------------
    // Servier for client connection
    //----------------------------------------------------------------
    while (!g_ThreadExitNfcAdaptation)
    {
        MTK_NFC_MSG_T *nfc_msg = NULL;
        INT32 ret;

        // - recv msg
        ret = mtk_nfc_adp_sys_msg_recv(MTK_NFC_TASKID_ADAPT, &nfc_msg);
        if (g_ThreadExitService == TRUE)
        {
            g_ThreadExitNfcAdaptation = TRUE;
        }
        if ((ret == MTK_NFC_SUCCESS) && (g_ThreadExitNfcAdaptation == FALSE))
        {
            mtk_nfc_adapt_proc((UINT8 *)nfc_msg);
        }
        else if (ret == (-3))
        {
            LOGD("nfc_adapt_proc_func,socket close\r\n");
            g_ThreadExitNfcAdaptation = TRUE;
            g_ThreadExitService = TRUE;
            g_ThreadExitMainProcFunc = TRUE;
            g_ThreadExitNfcClient = TRUE;
            mtk_nfc_sys_event_set(MTK_NFC_EVENT_2MAIN);
            mtk_nfc_sys_event_set(MTK_NFC_EVENT_2SOCKET);
            mtk_nfc_sys_event_set(MTK_NFC_EVENT_2SERV);
        }
        else
        {
            // read msg fail...Error case
            LOGD("nfc_adapt_proc_func,recv msg fail, ret:%d\r\n", ret);
            if (nfc_msg != NULL)
            {
                // - free msg
                mtk_nfc_sys_msg_free(nfc_msg);
            }
            // - avoid busy loop
            mtk_nfc_sys_sleep(1);

            continue;
        }

        // - free msg
        mtk_nfc_sys_msg_free(nfc_msg);

        // - avoid busy loop
        //  mtk_nfc_sys_sleep(5);
    }

    LOGD("nfc_adapt_proc_func end\r\n");
    g_ThreadExitNfcAdaptation = TRUE;
    pthread_exit((void *)result);

    return NULL;

}


void *nfc_client_proc_func(void * arg)
{
    MTK_NFC_MSG_T *nfc_msg = NULL;

    INT32 ret = 0;

    while (!g_ThreadExitNfcClient)
    {
        // - recv msg
        // LOGD("nfc_client_proc_func,recv,%d\r\n", MTK_NFC_TASKID_SOCKET);

        ret = mtk_nfc_sys_msg_recv(MTK_NFC_TASKID_SOCKET, &nfc_msg);
        if (g_ThreadExitService == TRUE)
        {
            g_ThreadExitNfcClient = TRUE;
        }

        if ((ret == MTK_NFC_SUCCESS)&&(g_ThreadExitNfcClient == FALSE))
        {
            mtk_nfc_client_proc((unsigned char *)nfc_msg);
        }
        else
        {
            // read msg fail...
            LOGD("nfc_client_proc_func,recv msg fail, ret:%d\r\n", ret);
            if (nfc_msg != NULL)
            {
                // - free msg
                mtk_nfc_sys_msg_free(nfc_msg);
            }
            // - avoid busy loop
            mtk_nfc_sys_sleep(1);

            continue;
        }

        // - free msg
        mtk_nfc_sys_msg_free(nfc_msg);

        // - avoid busy loop
        // mtk_nfc_sys_sleep(5);
    }
    LOGD("nfc_client_proc_func end\r\n");
    g_ThreadExitNfcClient = TRUE;
    pthread_exit((void *)ret);
    return NULL;

}

int main (int argc, char** argv)
{
    int result = MTK_NFC_ERROR;

    unsigned char szDbgFile[64] = "";

    struct timeval tv;
    struct tm *p_tm;

    gettimeofday(&tv, NULL);
    p_tm = localtime(&tv.tv_sec);

    memset(szDbgFile, 0, 64);

    sprintf(szDbgFile, "/tmp/nfc_debug-%d-%d-%d_%d-%d-%d",
            (int)p_tm->tm_year,
            (int)p_tm->tm_mon,
            (int)p_tm->tm_mday,
            (int)p_tm->tm_hour,
            (int)p_tm->tm_min,
            (int)p_tm->tm_sec);

    g_dbg_fp = fopen(szDbgFile, "w+b");

#ifdef ADJUST_THREAD_PRIORITY
    linux_nfc_thread_adjust_priority("main");
#endif

    g_NfcTsetMode = 0;
    if(argc == 2)
    {
        if(!strcmp(argv[1],"TEST"))
        {
            LOGD("TEST_MODE");
            g_NfcTsetMode = 1;
        }
    }
    LOGD("TEST,%d,%s,%s,%d\r\n",argc ,argv[0] ,argv[1], g_NfcTsetMode);

    result = linux_nfc_sys_init();

    if (MTK_NFC_SUCCESS == result)
    {
        result = linux_nfc_sys_run();
    }
    else
    {
        LOGD("linux_nfc_sys_init fail...\n");
    }
    LOGD("linux_nfc_sys_init exit,%d\n",result);
    linux_nfc_sys_deinit();

    if (g_dbg_fp  != NULL)
    {
        fclose(g_dbg_fp);
    }

    return MTK_NFC_SUCCESS;
}

