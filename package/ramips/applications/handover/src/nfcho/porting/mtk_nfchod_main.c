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
 *  nfcd_amin.c
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
 *
 *******************************************************************************/
/*****************************************************************************
 * Include
 *****************************************************************************/

#include "mtk_nfchod_main_sys.h"
#include "mtk_nfc_sys_type.h"


#define NFC_MSG_HDR_SIZE            sizeof(MTK_NFC_APP_MSG_T)


volatile int g_MtkNfchoServiceThreadExit = 1;
volatile int g_MtkNfchoMainProcessExit = 1;


int g_nfc_socket_fd = -1;

int ra_socket_fd = -1;
int br_socket_fd = -1;

pid_t g_pid = -1;

struct ifreq g_ra_ifr;
struct sockaddr_ll g_ra_ll;

struct ifreq g_br_ifr;
struct sockaddr_ll g_br_ll;

struct sockaddr_un g_socket;
//struct sockaddr_un g_service;

char g_listen_if[10];
char g_send_if[10];


unsigned short protocol = 0x6605;


unsigned char NfcStatus = 0;
unsigned char NfcOpCmd = 0;
unsigned char NfcTsetMode = 0;

unsigned char SupportRadioType = (1 << MTK_NFCHO_RADIO_WIFIWPS); //Deafult WIFi WPS

const char* g_HwTest[(MTK_WIFINFC_HWTEST_END-MTK_WIFINFC_HWTEST_DEP)] = { "Reader DEP " ,
                                                                          "SWP " ,
                                                                          "Card " ,
                                                                          "VCrad "
                                                                        };

typedef struct __attribute__ ((packed)) __CMD_INFO
{
    unsigned short VendorId;
    unsigned char Action;
    unsigned char Type;
    unsigned short DataLen;
    unsigned char Data[1];
} CMD_INFO, *PCMD_INFO;


///  Radio command handler functions
int mtk_nfcho_wifidriver_event_send(unsigned char* Sendbuf, unsigned short len,unsigned char Action,unsigned char Type);
int mtk_nfcho_wifidriver_event_read(void);
int mtk_nfcho_exit_thread_normal(MTK_NFCHO_THREAD_T *arg);


static MTK_NFCHO_THREAD_T g_mtk_nfcd_thread[MTK_NFCHO_THREAD_END] =
{
    {MTK_NFCHO_THREAD_SERVICE,    C_INVALID_TID, mtk_nfcho_exit_thread_normal }//,
};



static void mtk_nfcho_service_debug(char *data, ...)
{

    char szBuf[512];
    va_list  args;
    unsigned int length = 0;


    memset(szBuf, 0x00, 512);
    va_start(args, data);
    length = strlen(szBuf);
    vsnprintf((szBuf-length), (512-length) , data, args);
    va_end(args);

    length = strlen(szBuf);

    if ((length != 0) && (length < 512))
    {
        MTK_NFCAPP_LOGS("%s ", szBuf);
    }
    else
    {
        MTK_NFCAPP_LOGE("dbg(%d)", length);
    }
}

int mtk_nfcho_service_recv_check(unsigned char** p_msg)
{
    int i4ReadLen = 0;
    MTK_NFC_APP_MSG_T msg_hdr;
    void *p_msg_body = NULL;
    unsigned char *pBuff= NULL;


    // read msg header (blocking read)
    memset(&msg_hdr, 0x00, sizeof(MTK_NFC_APP_MSG_T));
    pBuff = (unsigned char *)&msg_hdr;
    i4ReadLen = read(g_nfc_socket_fd, pBuff, NFC_MSG_HDR_SIZE);
    if (i4ReadLen <= 0) // error case
    {
        return FALSE;
    }
    else if (NFC_MSG_HDR_SIZE != i4ReadLen)
    {
        MTK_NFCAPP_LOGE("unexpected(l,%d,read l,%d)", NFC_MSG_HDR_SIZE, i4ReadLen);
        return FALSE;
    }
    else
    {
        MTK_NFCAPP_LOGD("M(t,%d,l,%d)", msg_hdr.type, msg_hdr.length);

        // malloc msg
        *p_msg = (unsigned char*)malloc(NFC_MSG_HDR_SIZE + msg_hdr.length);
        if (*p_msg == NULL)
        {
            MTK_NFCAPP_LOGE("malloc fail");
            return FALSE;
        }

        // fill type & length
        memcpy((unsigned char *)*p_msg, (unsigned char *)&msg_hdr, NFC_MSG_HDR_SIZE);
    }

    // read msg body (blocking read)
    if (msg_hdr.length > 0)
    {
        p_msg_body = (unsigned char *)*p_msg + NFC_MSG_HDR_SIZE;
        pBuff = (unsigned char *)p_msg_body;
        i4ReadLen = read(g_nfc_socket_fd, pBuff, msg_hdr.length);
        if (i4ReadLen <= 0) // error case
        {
            MTK_NFCAPP_LOGE("read error(%d)", i4ReadLen);
            free(*p_msg);
            *p_msg = NULL;
            return FALSE;
        }
        else if (msg_hdr.length != (UINT32)i4ReadLen)
        {
            MTK_NFCAPP_LOGE("unexpected(l,%d,read l,%d)", msg_hdr.length, i4ReadLen);
            free(*p_msg);
            *p_msg = NULL;
            return FALSE;
        }
    }

    return TRUE;
}



int mtk_nfcho_service_recv_handle(void)
{
    unsigned char* prmsg = NULL;
    int is_running;
    int retVal;

    MTK_NFCAPP_LOGD("service_recv_handle ..");

    is_running = TRUE;

    while ((is_running == TRUE) && (g_MtkNfchoServiceThreadExit == 0))
    {
        // - recv msg
        retVal = mtk_nfcho_service_recv_check(&prmsg);
        if (retVal == FALSE)
        {
            is_running = FALSE; // exit loop
        }
        else
        {
            // - proc msg
            mtk_nfc_app_service_proc((unsigned char*)prmsg);

            // - free msg
            free(prmsg);
            prmsg = NULL;
        }
    }
    MTK_NFCAPP_LOGD("nfchod service, out ..");
    return 0;
}


static void mtk_nfcho_service_send(unsigned char* msg_ptr,unsigned int len)
{
    // MTK_NFC_APP_MSG_T* pBuf = NULL;
    // pBuf = (MTK_NFC_APP_MSG_T*)msg_ptr;
    // Check meesage is not NULL
    if (msg_ptr != NULL)
    {
        // MTK_NFCAPP_LOGD("send,t,%d,l,%d,%d",pBuf->type, pBuf->length, len);
        if (write(g_nfc_socket_fd, (void *)msg_ptr, len) < 0)
        {
            MTK_NFCAPP_LOGE("service send fail:%s,%s",MTK_NFCD_SERVICE2APP, strerror(errno));
        }
    }
    else
    {
        MTK_NFCAPP_LOGE("send,NULL");
    }
}

int mtk_nfcho_hwtest_read(void)
{
    int rec_bytes = 0, count = 0, ret = 1;
    unsigned char u1MtkNfcdRecvBuf[DEFAULT_ACCEPTABLE_LENGTH];
    socklen_t remotelen;


    while ( count < 800)
    {
        memset(u1MtkNfcdRecvBuf, 0x00, DEFAULT_ACCEPTABLE_LENGTH);
        remotelen = sizeof(g_socket);
        rec_bytes = read(g_nfc_socket_fd, u1MtkNfcdRecvBuf, DEFAULT_ACCEPTABLE_LENGTH);
        if ( rec_bytes > 0)
        {
            ret = mtk_nfc_app_service_proc(u1MtkNfcdRecvBuf);
            if ((ret == 0) || (ret == 1))
            {
                break;
            }
        }
        usleep(10000);  //sleep 10ms for waiting
        count++;
    }
    MTK_NFCAPP_LOGD("hwtest_read,%d,%d",count, ret);
    return ret;
}


void clean_up_child_process (int signal_number)
{
    int status;

    MTK_NFCAPP_LOGD("clean_up_child_process...(sig_num: %d)\n", signal_number);
    /* Clean up the child process. */
    wait (&status);
    MTK_NFCAPP_LOGD("clean_up_child_process status: %d\n", status);
}

void *thread_got_signal(int sig, siginfo_t *info, void *ucontext)
{
    MTK_NFCAPP_LOGE("thread_got_signal,%d\r\n", sig);
    return NULL;
}


int mtk_nfcho_exit_thread_normal(MTK_NFCHO_THREAD_T *arg)
{
    /* exit thread by pthread_kill -> pthread_join*/
    int err;
//   void *ret;

    if (!arg)
    {
        return (-1);
    }
    MTK_NFCAPP_LOGD("mtk_nfcho_exit_thread_normal,%d", arg->thread_id );

    if(arg->thread_id == MTK_NFCHO_THREAD_SERVICE)
    {
        g_MtkNfchoServiceThreadExit = 1;
    }
    MTK_NFCAPP_LOGD("mtk_nfcho_exit_thread_normal,waiting");
//   err = pthread_join(arg->thread_handle, &ret);

    err = pthread_kill(arg->thread_handle, SIGALRM);
    MTK_NFCAPP_LOGD("mtk_nfcho_exit_thread_normal,released");
    if (err)
    {
        MTK_NFCAPP_LOGE("(%d)TLeaveErr=%d",(arg->thread_id), err);
        return err;
    }
    else
    {
        MTK_NFCAPP_LOGD("(%d)TLeaveOK", (arg->thread_id));
    }
    arg->thread_handle = C_INVALID_TID;
    return 0;
}

int mtk_nfcho_threads_release(MTK_NFCHO_THREAD_ID_ENUM idx)
{
    int ret = 0;

    if (g_mtk_nfcd_thread[idx].thread_handle == C_INVALID_TID)
    {
        return ret;
    }
    MTK_NFCAPP_LOGD("Thread Handler,%d,%p", idx, g_mtk_nfcd_thread[idx].thread_exit);

    if (g_mtk_nfcd_thread[idx].thread_exit == NULL)
    {
        MTK_NFCAPP_LOGE("NotReady to release");
        return ret;
    }
    MTK_NFCAPP_LOGD("Ready to release");

    if ((g_mtk_nfcd_thread[idx].thread_exit(&g_mtk_nfcd_thread[idx]))!= 0)
    {
        // Error handler
        MTK_NFCAPP_LOGE("Thread,%d,Exit,Err", idx);
    }
    else
    {
        MTK_NFCAPP_LOGD("Thread,%d,Exit,OK", idx );
    }
    return ret;
}


void * mtk_nfcho_service_thread_handle(void * arg)
{
    int ret = 0;
    MTK_NFCHO_THREAD_T *ptr = (MTK_NFCHO_THREAD_T*)arg;

    int hdlsig = SIGUSR1;
    struct sigaction sa;


    if (!ptr)
    {
        pthread_exit((void*)0);
        return NULL;
    }

    MTK_NFCAPP_LOGD("service_thread...");

    sa.sa_handler = NULL;
    sa.sa_sigaction = thread_got_signal;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    if (sigaction(hdlsig, &sa, NULL) < 0)
    {
        MTK_NFCAPP_LOGE("sigaction error");
    }

    mtk_nfcho_service_recv_handle();

    MTK_NFCAPP_LOGE("service_thread end");
    pthread_exit((void *)ret);
    return NULL;
}



int mtk_nfcho_threads_create(MTK_NFCHO_THREAD_ID_ENUM thread_id)
{
    int ret = 0;

    if (thread_id == MTK_NFCHO_THREAD_SERVICE)
    {
        g_MtkNfchoServiceThreadExit = 0;
        if (pthread_create(&g_mtk_nfcd_thread[thread_id].thread_handle,
                           NULL, mtk_nfcho_service_thread_handle,
                           (void*)&g_mtk_nfcd_thread[thread_id]))
        {
            g_MtkNfchoServiceThreadExit = 1;
            ret = -1;
        }
    }

    return ret;
}

void hex_dump( char *str, unsigned char *pSrcBufVA, unsigned int SrcBufLen)
{
    unsigned char *pt;
    int x;

    pt = pSrcBufVA;
    MTK_NFCAPP_LOGD("len, %d", SrcBufLen);
    for (x = 0; x < SrcBufLen; x++)
    {
        if (x % 16 == 0)
        {
            printf("0x%04x : ", x);
        }
        printf("%02x ", ((unsigned char)pt[x]));
        if (x % 16 == 15)
        {
            printf("\n");
        }
    }
    printf("\n");
}

int mtk_nfchod_init_sw_queue (int* fd, unsigned char* filename, struct sockaddr_un* plocal)
{
    int ret = 0;
    int service_sock = -1;

    if ((service_sock = socket(AF_LOCAL, SOCK_STREAM, 0)) == -1)
    {
        MTK_NFCAPP_LOGE("socket open failed");
        ret =  -1;
    }
    else
    {
        memset(plocal, 0, sizeof(struct sockaddr_un));
        plocal->sun_family = AF_LOCAL;
        strcpy(plocal->sun_path, filename);
        MTK_NFCAPP_LOGD("socket open,%s", filename);
        if (connect(service_sock, (struct sockaddr *)plocal, sizeof(struct sockaddr_un)) < 0 )
        {
            MTK_NFCAPP_LOGE("socket bind failed");
            close(service_sock);
            service_sock = -1;
            ret = -1;
        }
        else
        {
            (*fd) = service_sock;
        }
    }
    return ret;
}

int mtk_nfchod_deinit_sw_queue(int* fd)
{
    int ret = 0;

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
        MTK_NFCAPP_LOGE("fd,NULL");
        ret =  -1;
    }
    return ret;
}



int mtk_nfcho_init (void)
{
    struct sigaction sigchld_action;

    memset (&sigchld_action, 0, sizeof (sigchld_action));
    sigchld_action.sa_handler = &clean_up_child_process;
    sigaction (SIGCHLD, &sigchld_action, NULL);


    // Initialize socket
    g_nfc_socket_fd = -1;

    // Run nfc service process
    if ((g_pid = fork()) < 0)
    {
        MTK_NFCAPP_LOGE("mtk_nfcho_init: fork fails: %d (%s)", errno, strerror(errno));
        return (-2);
    }
    else if (g_pid == 0)  /*child process*/
    {
        int err;

        if (NfcTsetMode == 0)
        {
            MTK_NFCAPP_LOGD("nfc_open: execute: %s", "/bin/nfcsd");
            err = execl("/bin/nfcsd", "nfcsd", NULL);
            if (err == -1)
            {
                MTK_NFCAPP_LOGE("nfcho_init: execl error: %s", strerror(errno));
                return (-3);
            }
        }
        else
        {
            MTK_NFCAPP_LOGD("nfc_open: execute: %s", "/bin/nfcsd TEST");
            err = execl("/bin/nfcsd", "nfcsd", "TEST", NULL);
            if (err == -1)
            {
                MTK_NFCAPP_LOGE("nfcho_init: execl error: %s", strerror(errno));
                return (-3);
            }
        }
    }
    else  /*parent process*/
    {
        MTK_NFCAPP_LOGE("nfcho_init: pid = %d", g_pid);
    }
    usleep(500*1000);

    if (mtk_nfchod_init_sw_queue(&g_nfc_socket_fd, MTK_NFCD_SERVICE2APP, &g_socket) == 0)
    {
        MTK_NFCAPP_LOGD("socket_fd,%d", g_nfc_socket_fd);
    }
    else
    {
        MTK_NFCAPP_LOGD("g_nfc_socket_fd,fail");
    }
    // Create threads

    if (NfcTsetMode == 1)
    {
        int x;
        x=fcntl(g_nfc_socket_fd,F_GETFL,0);
        fcntl(g_nfc_socket_fd, F_SETFL, (x | O_NONBLOCK));
        g_MtkNfchoServiceThreadExit = 0;
    }
    else
    {
        mtk_nfcho_threads_create(MTK_NFCHO_THREAD_SERVICE);
    }
    return 0;
}


int mtk_nfcho_uninit (void)
{
    int idx = 0, err = 0;

// Release threads
    if (NfcTsetMode == 1)
    {
        for ( idx = 0 ; idx < MTK_NFCHO_THREAD_END; idx++)
        {
            mtk_nfcho_threads_release(idx);
        }
    }

    mtk_nfchod_deinit_sw_queue(&g_nfc_socket_fd);

//   err = kill(g_pid, SIGTERM);
    NfcTsetMode = 0;
    if (err != 0)
    {
        MTK_NFCAPP_LOGE("nfcsd kill error: %s\n", strerror(errno));
    }
    else
    {
        MTK_NFCAPP_LOGD("nfcsd kill ok,%d\n", err);
    }
    return 0;
}


static void mtk_nfcho_nfc_event_handle ( unsigned short Event, unsigned char DataType, unsigned short DataStartIdx, unsigned short DataLth, unsigned char* DataBuf)
{
    unsigned char Ret = 0;
    unsigned char CmdType = 0;

    MTK_NFCAPP_LOGD("nfc_event,%d", Event);

    switch (Event)
    {
        case MTK_NFC_APP_EVENT_RESULT:
        {
            if ((DataType == MTK_NFC_ENTER_RSP) && (DataLth == 1))
            {
                if ((*DataBuf) == 0x00 )
                {
                    mtk_nfc_app_service_start(NfcOpCmd);
                }
                else  // init mw fail nfc bye bye
                {
                    mtk_nfc_app_service_exit();
                }
            }
            else if ((DataType == MTK_NFC_START_RSP) && (DataLth == 1))
            {
                Ret = (*DataBuf);
                mtk_nfcho_wifidriver_event_send(&Ret, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
            }
            else if ((DataType == MTK_NFC_READ_RSP) && (DataLth == 1))
            {
                Ret = (*DataBuf);
                mtk_nfcho_wifidriver_event_send(&Ret, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
            }
            else
            {
                MTK_NFCAPP_LOGE("event,result,%d,%d",DataType, DataLth);
            }
        }
        break;

        case MTK_NFC_APP_EVENT_READ:
        {
            unsigned short DLth =  (DataLth - DataStartIdx);
            unsigned short TypeLth = DataStartIdx;

            if (DataType == MTK_NFC_HO_WPSCF_TYPE)
            {
                CmdType = MTK_WIFINFC_TYPE_CONFIG;
            }
            else if (DataType == MTK_NFC_HO_WPSPD_TYPE)
            {
                CmdType = MTK_WIFINFC_TYPE_PASSWD;
            }
            else if (DataType == MTK_NFC_HO_WIFIDIRECTHOS_TYPE)
            {
                CmdType = MTK_WIFINFC_TYPE_WIFIDIRECTHOS;
            }
            else if (DataType == MTK_NFC_HO_WPSHOS_TYPE)
            {
                CmdType = MTK_WIFINFC_TYPE_PASSWDHOS;
            }
            else if (DataType == MTK_NFC_HO_WIFIFCS_TYPE)
            {
                CmdType = MTK_WIFINFC_TYPE_FASTCONNECTS;
            }
            else if (DataType == MTK_NFC_HO_WIFIDIRECTHOR_TYPE)
            {
                CmdType = MTK_WIFINFC_TYPE_WIFIDIRECTHOR;
            }
            else if (DataType == MTK_NFC_HO_WPSHOR_TYPE)
            {
                CmdType = MTK_WIFINFC_TYPE_PASSWDHOR;
            }
            else if (DataType == MTK_NFC_HO_WIFIFCR_TYPE)
            {
                CmdType = MTK_WIFINFC_TYPE_FASTCONNECTR;
            }
            else if (DataType == MTK_NFC_HO_BTS_TYPE)
            {
                CmdType = MTK_BTNFC_TYPE_MACADDR;
            }
            else if (DataType == MTK_NFC_HO_MEDIATYPE_RTD)
            {
                const char type_name[] = "application/tapservice.wfd.v1";
                char type_buf[32];
                // unsigned char data_idx = 0;

                memset(type_buf, 0x00, 32);

                if (TypeLth == strlen(type_name))
                {
                    memcpy(type_buf, DataBuf, TypeLth);
                    if (memcmp(type_buf, type_name, TypeLth) == 0)
                    {
#if 0
                        for (data_idx = 0; data_idx < DLth;  data_idx++)
                        {
                            MTK_NFCAPP_LOGD("%d,0x%x", data_idx, *(DataBuf+TypeLth+data_idx) );
                        }
#endif
                        MTK_NFCAPP_LOGD("Got tapservice.wfd.v1 ");
                    }
                }
            }

            MTK_NFCAPP_LOGD("R,Done,%d,%d,%d,%d", DataType, CmdType, TypeLth, DLth );

            if (CmdType != 0)
            {
                mtk_nfcho_wifidriver_event_send((DataBuf+TypeLth), DLth, MTK_WIFINFC_ACT_FROMNFC_SET, CmdType);
            }
        }
        break;

        case MTK_NFC_APP_EVENT_WRITE:
        {
            if (DataLth == 1)
            {
                MTK_NFCAPP_LOGD("Written,0x%x", (*DataBuf));
                Ret = (*DataBuf);
                mtk_nfcho_wifidriver_event_send(&Ret, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
            }
        }
        break;

        case MTK_NFC_APP_EVENT_FOUNDDEVICE:
        case MTK_NFC_APP_EVENT_FOUNDTAG:
        {

            MTK_NFCAPP_LOGD("Found Device");

            if (Event == MTK_NFC_APP_EVENT_FOUNDDEVICE)
            {
                if ( (SupportRadioType & (1 << MTK_NFCHO_RADIO_BT)) == (1 << MTK_NFCHO_RADIO_BT))
                {
                    mtk_nfcho_wifidriver_event_send(NULL, 0, MTK_WIFINFC_ACT_FROMNFC_GET, MTK_BTNFC_TYPE_MACADDR);    // BT
                }
                if ( (SupportRadioType & (1 << MTK_NFCHO_RADIO_WIFIWPS)) == (1 << MTK_NFCHO_RADIO_WIFIWPS))
                {
                    mtk_nfcho_wifidriver_event_send(NULL, 0, MTK_WIFINFC_ACT_FROMNFC_GET, MTK_WIFINFC_TYPE_PASSWDHOS);    // WiFi
                }
            }
            NfcStatus = (MTK_WIFINFC_NFC_DETECTED|MTK_WIFINFC_NFC_ON);
            mtk_nfcho_wifidriver_event_send(&NfcStatus, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_NFCSTATUS);

        }
        break;

        case MTK_NFC_APP_EVENT_DEVICELEFT:
        {

            MTK_NFCAPP_LOGD("Device Left");
            NfcStatus = (MTK_WIFINFC_NFC_ON);
            mtk_nfcho_wifidriver_event_send(&Ret, 1, MTK_WIFINFC_ACT_FROMNFC_SET, NfcStatus);
        }
        break;

        case MTK_NFC_APP_EVENT_KILLDONE:
        {

            MTK_NFCAPP_LOGD("Nfc killed");
            NfcStatus = 0;

            g_MtkNfchoServiceThreadExit = 1;
            g_MtkNfchoMainProcessExit = 1;
            mtk_nfcho_wifidriver_event_send(&Ret, 1, MTK_WIFINFC_ACT_FROMNFC_SET, NfcStatus);
        }
        break;

        default:
            MTK_NFCAPP_LOGE("Not Support");
            break;

    }

}


int mtk_nfcho_cmd_handler( unsigned char* buf, unsigned int Length, unsigned char Action, unsigned char Type)
{
    int ret = 0;
    unsigned char CmdRet = 0;
    MTK_NFC_APP_WRITEATA_T ReqData;

    MTK_NFCAPP_LOGD("cmd,L,%d,A,%d,T,%d", Length, Action, Type);
    memset(&ReqData, 0x00, sizeof(MTK_NFC_APP_WRITEATA_T));

    if (Action == MTK_WIFINFC_ACT_TONFC_GET)
    {
        switch (Type)
        {
            case MTK_WIFINFC_TYPE_NFCSTATUS:
            {
                // Query NFC middleware status

                mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, NfcStatus);
            }
            break;

            default:
            {
                MTK_NFCAPP_LOGE("Recv,A,%d,T,%d",Action, Type);
            }
            break;
        }
    }
    else if (Action == MTK_WIFINFC_ACT_TONFC_SET)
    {
        switch (Type)
        {
            case MTK_WIFINFC_TYPE_CONFIG:
            {

                // bring buf data array to handover
                // case 1 if determine P2P device

                // to be selector role and trigger handover server mode

                // case 2 if determmine tag
                // to write NDEF data
                if (NfcTsetMode == 0)
                {
                    ReqData.negohandover = 3;
                    ReqData.num_payload = 1;
                    ReqData.data_type[0] = MTK_NFC_HO_WPSCF_TYPE;
                    ReqData.radio_type[0] = MTK_NFCHO_RADIO_WIFIWPS;
                    ReqData.payload[0].data = buf;
                    ReqData.payload[0].lth = Length;

                    ret = mtk_nfc_app_service_write(&ReqData);
                    if (ret == 0)
                    {
                        CmdRet = 0;
                    }
                    else
                    {
                        CmdRet = 0xFF;
                        mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
                    }
                }
            }
            break;
            case MTK_WIFINFC_TYPE_PASSWD:
            {
                // case 1 if determmine tag
                // to write NDEF data
                if (NfcTsetMode == 0)
                {
                    ReqData.negohandover = 3;
                    ReqData.num_payload = 1;
                    ReqData.data_type[0] = MTK_NFC_HO_WPSPD_TYPE;
                    ReqData.radio_type[0] = MTK_NFCHO_RADIO_WIFIWPS;
                    ReqData.payload[0].data = buf;
                    ReqData.payload[0].lth = Length;

                    ret = mtk_nfc_app_service_write(&ReqData);

                    if (ret == 0)
                    {
                        CmdRet = 0;
                    }
                    else
                    {
                        CmdRet = 0xFF;
                        mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
                    }
                }
            }
            break;
            case MTK_WIFINFC_TYPE_PASSWDHOS:
            {
                if (NfcTsetMode == 0)
                {
                    ReqData.negohandover = 2;
                    ReqData.num_payload = 1;
                    ReqData.data_type[0] = MTK_NFC_HO_WPSHOS_TYPE;
                    ReqData.radio_type[0] = MTK_NFCHO_RADIO_WIFIWPS;
                    ReqData.payload[0].data = buf;
                    ReqData.payload[0].lth = Length;
                    ret = mtk_nfc_app_service_write(&ReqData);
                    if (ret == 0)
                    {
                        CmdRet = 0;
                    }
                    else
                    {
                        CmdRet = 0xFF;
                        mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
                    }
                }
            }
            break;

            case MTK_WIFINFC_TYPE_FASTCONNECTS:
            {
                if (NfcTsetMode == 0)
                {
                    ReqData.negohandover = 2;
                    ReqData.num_payload = 1;
                    ReqData.data_type[0] = MTK_NFC_HO_WIFIFCS_TYPE;
                    ReqData.radio_type[0] = MTK_NFCHO_RADIO_WIFIFC;
                    ReqData.payload[0].data = buf;
                    ReqData.payload[0].lth = Length;

                    ret = mtk_nfc_app_service_write(&ReqData);
                    if (ret == 0)
                    {
                        CmdRet = 0;
                    }
                    else
                    {
                        CmdRet = 0xFF;
                        mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
                    }
                }
            }
            break;


            case MTK_WIFINFC_TYPE_WIFIDIRECTHOS:
            {
                if (NfcTsetMode == 0)
                {
                    ReqData.negohandover = 2;
                    ReqData.num_payload = 1;
                    ReqData.data_type[0] = MTK_NFC_HO_WIFIDIRECTHOS_TYPE;
                    ReqData.radio_type[0] = MTK_NFCHO_RADIO_WIFIDIRECT;
                    ReqData.payload[0].data = buf;
                    ReqData.payload[0].lth = Length;
                    ret = mtk_nfc_app_service_write(&ReqData);
                    if (ret == 0)
                    {
                        CmdRet = 0;
                    }
                    else
                    {
                        CmdRet = 0xFF;
                        mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
                    }
                }
            }
            break;

            case MTK_WIFINFC_TYPE_PASSWDHOR:
            {
                if (NfcTsetMode == 0)
                {
                    ReqData.negohandover = 1;
                    ReqData.num_payload = 1;
                    ReqData.data_type[0] = MTK_NFC_HO_WPSHOR_TYPE;
                    ReqData.radio_type[0] = MTK_NFCHO_RADIO_WIFIWPS;
                    ReqData.payload[0].data = buf;
                    ReqData.payload[0].lth = Length;

                    ret = mtk_nfc_app_service_write(&ReqData);
                    if (ret == 0)
                    {
                        CmdRet = 0;
                    }
                    else
                    {
                        CmdRet = 0xFF;
                        mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
                    }
                }
            }
            break;

            case MTK_WIFINFC_TYPE_FASTCONNECTR:
            {
                if (NfcTsetMode == 0)
                {
                    ReqData.negohandover = 1;
                    ReqData.num_payload = 1;
                    ReqData.data_type[0] = MTK_NFC_HO_WIFIFCR_TYPE;
                    ReqData.radio_type[0] = MTK_NFCHO_RADIO_WIFIFC;
                    ReqData.payload[0].data = buf;
                    ReqData.payload[0].lth = Length;

                    ret = mtk_nfc_app_service_write(&ReqData);
                    if (ret == 0)
                    {
                        CmdRet = 0;
                    }
                    else
                    {
                        CmdRet = 0xFF;
                        mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
                    }
                }
            }
            break;

            case MTK_WIFINFC_TYPE_WIFIDIRECTHOR:
            {
                if (NfcTsetMode == 0)
                {
                    ReqData.negohandover = 1;
                    ReqData.num_payload = 1;
                    ReqData.data_type[0] = MTK_NFC_HO_WIFIDIRECTHOR_TYPE;
                    ReqData.radio_type[0] = MTK_NFCHO_RADIO_WIFIDIRECT;
                    ReqData.payload[0].data = buf;
                    ReqData.payload[0].lth = Length;

                    ret = mtk_nfc_app_service_write(&ReqData);

                    if (ret == 0)
                    {
                        CmdRet = 0;
                    }
                    else
                    {
                        CmdRet = 0xFF;
                        mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
                    }
                }
            }
            break;

            case MTK_BTNFC_TYPE_MACADDR:
            {
                if (NfcTsetMode == 0)
                {
                    ReqData.negohandover = 1;
                    ReqData.num_payload = 1;
                    ReqData.data_type[0] = MTK_NFC_HO_BTR_TYPE;
                    ReqData.radio_type[0] = MTK_NFCHO_RADIO_BT;
                    ReqData.payload[0].data = buf;
                    ReqData.payload[0].lth = Length;

                    ret = mtk_nfc_app_service_write(&ReqData);
                    if (ret == 0)
                    {
                        CmdRet = 0;
                    }
                    else
                    {
                        CmdRet = 0xFF;
                        mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_BTNFC_TYPE_CMDRESULT);
                    }
                }
            }
            break;
            case MTK_WIFINFC_TYPE_NFCSTATUS:
            {
                // Enable/Disable NFC
                if ((Length == 1) && (buf != NULL))
                {
                    NfcOpCmd = buf[0];
                    MTK_NFCAPP_LOGD("nfc,status,%d,Cmd,%d",NfcStatus, NfcOpCmd);

                    if ((NfcOpCmd < MTK_WIFINFC_SET_END) && (NfcTsetMode == 0))
                    {
                        if ( NfcOpCmd == MTK_WIFINFC_SET_OFF)
                        {
                            mtk_nfc_app_service_stop(NfcOpCmd);

                        }
                        else if ( NfcOpCmd == MTK_WIFINFC_READERTAG)
                        {
                            if (NfcStatus == (MTK_WIFINFC_NFC_DETECTED|MTK_WIFINFC_NFC_ON))
                            {
                                mtk_nfc_app_service_read(0x01);
                            }
                        }
                        else
                        {
                            MTK_NFC_APP_FUNCTION_POINT AppFun;

                            memset(&AppFun, 0x00, sizeof(MTK_NFC_APP_FUNCTION_POINT));

                            AppFun.event = mtk_nfcho_nfc_event_handle;
                            AppFun.send = mtk_nfcho_service_send;
                            AppFun.debug = mtk_nfcho_service_debug;
                            mtk_nfc_app_service_enter(&AppFun, 0);
                        }
                    }
                    else if ((NfcOpCmd == 30) && (NfcTsetMode == 0)) /////// Test
                    {
                        if (NfcStatus != 0)
                        {
                            mtk_nfcho_wifidriver_event_send(NULL, 0, MTK_WIFINFC_ACT_FROMNFC_GET, MTK_WIFINFC_TYPE_PASSWDHOS);
                        }
                        else
                        {
                            MTK_NFCAPP_LOGE("Nfc is disabled,%d", NfcStatus);
                        }
                    }
                    else if ((NfcOpCmd == 31) && (NfcTsetMode == 0))/////// Test
                    {
                        if (NfcStatus != 0)
                        {
                            mtk_nfcho_wifidriver_event_send(NULL, 0, MTK_WIFINFC_ACT_FROMNFC_GET, MTK_WIFINFC_TYPE_PASSWDHOR);
                        }
                        else
                        {
                            MTK_NFCAPP_LOGE("Nfc is disabled,%d", NfcStatus);
                        }
                    }
                    else if ((NfcOpCmd == 32) && (NfcTsetMode == 0))/////// Test Write  Android phone/BD player Tap Data
                    {
                        unsigned char buffer1[] = {0x08,0x00,0xFF,0xEE,0xDD,0xCC,0xBB,0xAA};
                        unsigned char buf2_type[] = {0x61,0x70,0x70,0x6C,0x69,0x63,0x61,0x74,0x69,
                                                     0x6F,0x6E,0x2F,0x74,0x61,0x70,0x73,0x65,0x72,
                                                     0x76,0x69,0x63,0x65,0x2E,0x77,0x66,0x64,0x2E,
                                                     0x76,0x31
                                                    };

                        unsigned char buffer2[] = {0x61,0x61,0x3A,0x62,0x62,0x3A,0x63,0x63,0x3A,
                                                   0x64,0x64,0x3A,0x65,0x65,0x3A,0x66,0x66
                                                  };
                        ReqData.non_ndef = 0;
                        ReqData.negohandover = 0;
                        ReqData.num_payload = 2;
                        ReqData.data_type[0] = 0xFF;
                        ReqData.radio_type[0] = MTK_NFCHO_RADIO_BT ;
                        ReqData.payload[0].data = buffer1;
                        ReqData.payload[0].lth = 8;

                        ReqData.data_type[1] = MTK_NFC_HO_MEDIATYPE_RTD;
                        ReqData.radio_type[1] = 0xFF;
                        ReqData.payload_type[1].data = buf2_type;
                        ReqData.payload_type[1].lth = 29;
                        ReqData.payload[1].data = buffer2;
                        ReqData.payload[1].lth = 17;

                        ret = mtk_nfc_app_service_write(&ReqData);

                        if (ret == 0)
                        {
                            CmdRet = 0;
                        }
                        else
                        {
                            CmdRet = 0xFF;
                            mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
                        }

                    }
                    else if (NfcOpCmd == 255)  // nfc bye bye
                    {
                        mtk_nfc_app_service_exit();
                    }
                    else if ((NfcTsetMode == 1) && ((NfcOpCmd >= MTK_WIFINFC_HWTEST_DEP) && (NfcOpCmd < MTK_WIFINFC_HWTEST_END)))
                    {
                        MTK_NFC_APP_FUNCTION_POINT AppFun;
                        unsigned char TestMode = 0;

                        memset(&AppFun, 0x00, sizeof(MTK_NFC_APP_FUNCTION_POINT));
                        AppFun.event = mtk_nfcho_nfc_event_handle;
                        AppFun.send = mtk_nfcho_service_send;
                        AppFun.debug = mtk_nfcho_service_debug;
                        if (NfcOpCmd == MTK_WIFINFC_HWTEST_DEP)
                        {
                            TestMode = MTK_NFC_START_HWTEST_DEP;
                        }
                        else if (NfcOpCmd == MTK_WIFINFC_HWTEST_SWP)
                        {
                            TestMode = MTK_NFC_START_HWTEST_SWP;
                        }
                        else if (NfcOpCmd == MTK_WIFINFC_HWTEST_CARD)
                        {
                            TestMode = MTK_NFC_START_HWTEST_CARD;
                        }
                        else if (NfcOpCmd == MTK_WIFINFC_HWTEST_VCARD)
                        {
                            TestMode = MTK_NFC_START_HWTEST_VCARD;
                        }
                        ret = mtk_nfc_app_service_enter(&AppFun, TestMode);
                        MTK_NFCAPP_LOGD("Test,0x%x,%d",TestMode, ret );
                        if (ret == 0)
                        {
                            ret = mtk_nfcho_hwtest_read();
                            if (ret == 0)
                            {
                                MTK_NFCAPP_LOGD("Test,%s,OK",g_HwTest[(NfcOpCmd-MTK_WIFINFC_HWTEST_DEP)]);
                            }
                            else
                            {
                                MTK_NFCAPP_LOGE("Test,%s,NG",g_HwTest[(NfcOpCmd-MTK_WIFINFC_HWTEST_DEP)]);
                            }
                        }
                        else
                        {
                            MTK_NFCAPP_LOGE("Enter,%d,Testmode,Fail",NfcOpCmd);
                        }
                        usleep(2000*1000); // 2000 ms
                        MTK_NFCAPP_LOGD("Nfc killed");
                        NfcStatus = 0;
                        g_MtkNfchoServiceThreadExit = 1;
                        g_MtkNfchoMainProcessExit = 1;
                    }
                    else
                    {
                        // Command invalid
                        MTK_NFCAPP_LOGD("Don't Request Polling,cmd,%d", buf[0]);
                        CmdRet = 0xFF;
                        mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
                    }
                }
                else
                {
                    // Command format error
                    CmdRet = 0xFF;
                    mtk_nfcho_wifidriver_event_send(&CmdRet, 1, MTK_WIFINFC_ACT_FROMNFC_SET, MTK_WIFINFC_TYPE_CMDRESULT);
                }

            }
            break;
            default:
            {
                MTK_NFCAPP_LOGE("Recv,A,%d,Type,%d",Action, Type);
            }
            break;
        }
    }
//    MTK_NFCAPP_LOGD("mtk_nfcho_cmd_handler,%d", ret);

    return ret;
}





int mtk_nfcho_wifidriver_event_send(unsigned char* Sendbuf, unsigned short len,unsigned char Action,unsigned char Type)
{
    int ret = 0, res = 0, total_length = 0;
    char ra_ifname[IFNAMSIZ];
    PCMD_INFO pCmd_info = NULL;

    memset(ra_ifname, 0, IFNAMSIZ);
    memcpy(ra_ifname, g_send_if, strlen(g_send_if));

    memset(&g_ra_ifr, 0, sizeof(g_ra_ifr));
    strncpy(g_ra_ifr.ifr_name, ra_ifname, IF_NAMESIZE);

    if (ioctl(ra_socket_fd, SIOCGIFINDEX, &g_ra_ifr) < 0)
    {
        MTK_NFCAPP_LOGE("ioctl - SIOCGIFINDEX(%s) failed!\n", ra_ifname);
    }

    memset(&g_ra_ll, 0, sizeof(g_ra_ll));
    g_ra_ll.sll_family = PF_PACKET;
    g_ra_ll.sll_ifindex = g_ra_ifr.ifr_ifindex;
    g_ra_ll.sll_protocol = htons(protocol);

    MTK_NFCAPP_LOGD("L,%d,A,%d,T,%d \n", len, Action, Type);
    if (ra_socket_fd != -1)
    {
        if (len != 0)
        {
            total_length = sizeof(CMD_INFO)+len;
        }
        else
        {
            total_length = sizeof(CMD_INFO);
        }
        pCmd_info = malloc(total_length);
        memset(pCmd_info, 0x00, total_length);

        pCmd_info->VendorId = 0x14C3;
        pCmd_info->Action = Action;
        pCmd_info->Type = Type;
        pCmd_info->DataLen = len;

        if ((len != 0)&&(Sendbuf != NULL))
        {
            memcpy(&pCmd_info->Data[0], Sendbuf, len);
        }

        //MTK_NFCAPP_LOGD("vid,0x%x,A,%x,T,%x,datalen,%d,total,%d",
        //                 pCmd_info->VendorId, pCmd_info->Action, pCmd_info->Type, pCmd_info->DataLen, total_length);

        //hex_dump("SendRawData", (void*)pCmd_info, total_length);

        res = sendto(ra_socket_fd, (void*)pCmd_info, total_length, 0, (struct sockaddr *) &g_ra_ll, sizeof(g_ra_ll));

        if (res < 0)
        {
            MTK_NFCAPP_LOGE("ra sendto failure,%d,%s", res,  strerror(errno));
            ret = -1;
        }
        //  MTK_NFCAPP_LOGD("Send Done\n");
        if (pCmd_info != NULL)
        {
            free(pCmd_info);
            pCmd_info = NULL;
        }
    }
    else
    {
        MTK_NFCAPP_LOGE("ra_socket_fd is Not Ready!");
        ret = -1;
    }
    return ret;
}




int mtk_nfcho_wifidriver_event_read()
{
    int res = 0;
    char br_ifname[IFNAMSIZ];
    char ra_ifname[IFNAMSIZ];
    PCMD_INFO pCmd_info = NULL;

    socklen_t fromlen;
    unsigned char buf[2300];

    memset(br_ifname, 0, IFNAMSIZ);
    memcpy(br_ifname, g_listen_if, strlen(g_listen_if));
    memset(ra_ifname, 0, IFNAMSIZ);
    memcpy(ra_ifname, g_send_if, strlen(g_send_if));


    MTK_NFCAPP_LOGD("l_ifname,%s,s_ifname,%s\n", br_ifname, ra_ifname);

    //========== listen_if ==================
    br_socket_fd = socket(PF_PACKET, SOCK_RAW, htons(protocol));

    if (br_socket_fd < 0)
    {
        MTK_NFCAPP_LOGE("listen open socket failed!");
        goto go_out;
    }

    memset(&g_br_ifr, 0, sizeof(g_br_ifr));
    strncpy(g_br_ifr.ifr_name, br_ifname, IF_NAMESIZE);

    if (ioctl(br_socket_fd, SIOCGIFINDEX, &g_br_ifr) < 0)
    {
        MTK_NFCAPP_LOGE("listen ioctl - SIOCGIFINDEX(%s) failed!", br_ifname);
        goto go_out;
    }

    memset(&g_br_ll, 0, sizeof(g_br_ll));
    g_br_ll.sll_family = PF_PACKET;
    g_br_ll.sll_ifindex = g_br_ifr.ifr_ifindex;
    g_br_ll.sll_protocol = htons(protocol);

    if (bind(br_socket_fd, (struct sockaddr *) &g_br_ll, sizeof(g_br_ll)) < 0)
    {
        MTK_NFCAPP_LOGE("listen bind - PF_PACKET failed");
        goto go_out;
    }

    if (ioctl(br_socket_fd, SIOCGIFHWADDR, &g_br_ifr) < 0)
    {
        MTK_NFCAPP_LOGE("listen ioctl - SIOCGIFHWADDR failed");
        goto go_out;
    }
    MTK_NFCAPP_LOGD("listen_hwaddr,%02x:%02x:%02x:%02x:%02x:%02x",
                    (unsigned char)g_br_ifr.ifr_hwaddr.sa_data[0],
                    (unsigned char)g_br_ifr.ifr_hwaddr.sa_data[1],
                    (unsigned char)g_br_ifr.ifr_hwaddr.sa_data[2],
                    (unsigned char)g_br_ifr.ifr_hwaddr.sa_data[3],
                    (unsigned char)g_br_ifr.ifr_hwaddr.sa_data[4],
                    (unsigned char)g_br_ifr.ifr_hwaddr.sa_data[5]);
    //========== listen_if ==================

    //==========send_if ==================
    ra_socket_fd = socket(PF_PACKET, SOCK_RAW, htons(protocol));

    if (ra_socket_fd < 0)
    {
        MTK_NFCAPP_LOGE("send open socket(PF_PACKET) failed!");
        goto go_out;
    }

    memset(&g_ra_ifr, 0, sizeof(g_ra_ifr));
    strncpy(g_ra_ifr.ifr_name, ra_ifname, IF_NAMESIZE);

    if (ioctl(ra_socket_fd, SIOCGIFINDEX, &g_ra_ifr) < 0)
    {
        MTK_NFCAPP_LOGE("send ioctl - SIOCGIFINDEX(%s) failed!", ra_ifname);
        goto go_out;
    }

    memset(&g_ra_ll, 0, sizeof(g_ra_ll));
    g_ra_ll.sll_family = PF_PACKET;
    g_ra_ll.sll_ifindex = g_ra_ifr.ifr_ifindex;
    g_ra_ll.sll_protocol = htons(protocol);

    if (bind(ra_socket_fd, (struct sockaddr *) &g_ra_ll, sizeof(g_ra_ll)) < 0)
    {
        MTK_NFCAPP_LOGE("send bind - PF_PACKET failed");
        goto go_out;
    }
    if (ioctl(ra_socket_fd, SIOCGIFHWADDR, &g_ra_ifr) < 0)
    {
        MTK_NFCAPP_LOGE("send ioctl - SIOCGIFHWADDR failed");
        goto go_out;
    }
    MTK_NFCAPP_LOGD("send_hwaddr,%02x:%02x:%02x:%02x:%02x:%02x",
                    (unsigned char)g_ra_ifr.ifr_hwaddr.sa_data[0],
                    (unsigned char)g_ra_ifr.ifr_hwaddr.sa_data[1],
                    (unsigned char)g_ra_ifr.ifr_hwaddr.sa_data[2],
                    (unsigned char)g_ra_ifr.ifr_hwaddr.sa_data[3],
                    (unsigned char)g_ra_ifr.ifr_hwaddr.sa_data[4],
                    (unsigned char)g_ra_ifr.ifr_hwaddr.sa_data[5]);

    //========== send_if==================

    while(g_MtkNfchoMainProcessExit == 0)
    {
        memset(&g_br_ll, 0, sizeof(g_br_ll));
        fromlen = sizeof(g_br_ll);
        memset(buf, 0x00, 2300);

        //   MTK_NFCAPP_LOGD("try to receive from br0,%d", br_socket_fd);
        res = recvfrom(br_socket_fd, buf, sizeof(buf), 0, (struct sockaddr *)&g_br_ll, &fromlen);
        //   MTK_NFCAPP_LOGD("br0 recvfrom res,%d", res);
        if (res < 0)
        {
            MTK_NFCAPP_LOGE("listen,res,%d", res);
            usleep(1*1000);
        }
        else
        {
            pCmd_info = (PCMD_INFO)(buf+14);
            MTK_NFCAPP_LOGD("cmd,a,0x%x,T,0x%x,L,%d",pCmd_info->Action, pCmd_info->Type, pCmd_info->DataLen);
            //hex_dump("data", pCmd_info->Data, pCmd_info->DataLen);
            mtk_nfcho_cmd_handler(pCmd_info->Data, pCmd_info->DataLen, pCmd_info->Action, pCmd_info->Type );
        }
    }
go_out:
    if (br_socket_fd > 0)
    {
        close(br_socket_fd);
    }

    if (ra_socket_fd > 0)
    {
        close(ra_socket_fd);
    }
    return 0;
}

int main (int argc, char **argv)
{
    int ret = 0;

    NfcTsetMode = 0;
    memset(g_send_if, 0x00, sizeof(g_send_if));
    memset(g_listen_if, 0x00, sizeof(g_listen_if));

    strcpy(g_send_if, "ra0");
    strcpy(g_listen_if, "br-lan");

    if(argc == 4)
    {
#if 0
        if(strcmp(argv[1],"-l") == 0)
        {
            if (strlen(argv[2]) < sizeof(g_listen_if))
            {
                memset(g_listen_if, 0x00, sizeof(g_listen_if));
                memcpy(g_listen_if, argv[2], strlen(argv[2]));
            }
        }
#endif

        if(strcmp(argv[1],"-s") == 0)
        {
            if (strlen(argv[2]) < sizeof(g_send_if))
            {
                memset(g_send_if, 0x00, sizeof(g_send_if));
                memcpy(g_send_if, argv[2], strlen(argv[2]));
            }
        }

        if(strcmp(argv[3],"-t") == 0)
        {
            MTK_NFCAPP_LOGD("HWTEST");
            NfcTsetMode = 1;
        }
    }
    else if(argc == 3)
    {
#if 0
        if(strcmp(argv[1],"-l") == 0)
        {
            if (strlen(argv[2]) < sizeof(g_listen_if))
            {
                memset(g_listen_if, 0x00, sizeof(g_listen_if));
                memcpy(g_listen_if, argv[2], strlen(argv[2]));
            }
        }
#endif
        if(strcmp(argv[1],"-s") == 0)
        {
            if (strlen(argv[2]) < sizeof(g_send_if))
            {
                memset(g_send_if, 0x00, sizeof(g_send_if));
                memcpy(g_send_if, argv[2], strlen(argv[2]));
            }
        }

    }

    ret = mtk_nfcho_init();

    if ( ret == 0 )
    {
        MTK_NFCAPP_LOGD("run nfcho OK");
    }
    else
    {
        MTK_NFCAPP_LOGE("run nfcho NG");
    }
    g_MtkNfchoMainProcessExit = 0;

    while((g_MtkNfchoServiceThreadExit ==0) && (g_MtkNfchoMainProcessExit ==0))
    {
        mtk_nfcho_wifidriver_event_read();
    }

    MTK_NFCAPP_LOGD("nfcho,%d,%d ",g_MtkNfchoServiceThreadExit, g_MtkNfchoMainProcessExit);

    mtk_nfcho_uninit();

    MTK_NFCAPP_LOGD("exit nfcho");
    return 0;
}

