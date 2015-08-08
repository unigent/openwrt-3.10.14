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
 *  mtk_nfc_adp_sys.c
 *
 * Project:
 * --------
 *
 * Description:
 * ------------
 *
 * Author:
 * -------
 *  Hiki Chen, ext 25281, Hiki.Chen@mediatek.com, 2013-08-07
 *
 *******************************************************************************/
/*****************************************************************************
 * Include
 *****************************************************************************/
#include "mtk_nfc_sys_type.h"
#include "mtk_nfc_sys_type_ext.h"

#include "linux_nfc_main.h"

/*****************************************************************************
 * Define
 *****************************************************************************/

/*****************************************************************************
 * Data Structure
 *****************************************************************************/

/*****************************************************************************
 * Extern Area
 *****************************************************************************/

extern int gconn_fd_tmp;


/*****************************************************************************
 * GLobal Variable
 *****************************************************************************/


/*****************************************************************************
 * Function Prototype
 *****************************************************************************/


/*****************************************************************************
 * Utilties
 *****************************************************************************/
int nfc_recv_msg(int csock, MTK_NFC_MSG_T **p_msg)
{
    int i4ReadLen = 0;
    MTK_NFC_MSG_T msg_hdr;
    void *p_msg_body = NULL;;
    unsigned char *pBuff = NULL;

    // read msg header (blocking read)
    pBuff = (unsigned char *)&msg_hdr;
    i4ReadLen = mtk_nfc_adp_sys_socket_read(csock, pBuff, NFC_MSG_HDR_SIZE);
    if (i4ReadLen <= 0) // error case
    {
        return (-1);
    }
    else if (NFC_MSG_HDR_SIZE != i4ReadLen)
    {
        LOGD("unexpected length (hdr len: %d, read len: %d)\n", NFC_MSG_HDR_SIZE, i4ReadLen);
        return FALSE;
    }
    else
    {
        //  LOGD("msg hdr (type: %d, len: %d)\n", msg_hdr.type, msg_hdr.length);

        // malloc msg
        *p_msg = (MTK_NFC_MSG_T *)mtk_nfc_sys_mem_alloc(NFC_MSG_HDR_SIZE + msg_hdr.length);
        if (*p_msg == NULL)
        {
            LOGD("malloc fail\n");
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
        i4ReadLen = mtk_nfc_adp_sys_socket_read(csock, pBuff, msg_hdr.length);
        if (i4ReadLen <= 0) // error case
        {
            return (-1);
        }
        else if (msg_hdr.length != i4ReadLen)
        {
            LOGD("unexpected length (body len: %d, read len %d)\n", msg_hdr.length, i4ReadLen);
            mtk_nfc_sys_mem_free(*p_msg);
            *p_msg = NULL;
            return FALSE;
        }
    }

    //LOGD("android_nfc_jni_recv_msg ok\n");

    return TRUE;
}

/*****************************************************************************
 * Function
 *****************************************************************************/
/*****************************************************************************
 * Function
 *  mtk_nfc_adp_sys_socket_read
 * DESCRIPTION
 *  Read socket
 * PARAMETERS
 *  csock       [IN] sockfd
 *  pRecvBuff   [IN] socket recv buffer
 *  i4RecvLen   [IN] socket recv length
 * RETURNS
 *  MTK_NFC_SUCCESS
 *****************************************************************************/
INT32 mtk_nfc_adp_sys_socket_read(INT32 csock, UINT8 *pRecvBuff, INT32 i4RecvLen)
{
    int i4ReadLen = 0;
    int i4TotalReadLen = 0;
    socklen_t remotelen;
    remotelen = sizeof(struct sockaddr_un);

//  LOGD("nfc_socket_read...(pRecvBuff:0x%x, i4RecvLen:%d)\n", (unsigned int)pRecvBuff, i4RecvLen);

    if (csock < 0)
    {
        LOGD("nfc_socket_read fail: due to invalid sockfd: %d\n", csock);
        return -1;
    }

    /* read data to nfc daemon */
    i4TotalReadLen = 0;
    while (i4TotalReadLen < i4RecvLen)
    {
        i4ReadLen = read(csock, pRecvBuff, i4RecvLen - i4TotalReadLen);
        if (i4ReadLen < 0)
        {
            LOGD("nfc_socket_read fail: %d\n", i4ReadLen);
            i4TotalReadLen = i4ReadLen; // keep read fail return value
            break; // exit loop
        }
        else if (i4ReadLen == 0)
        {
            LOGD("nfc_socket_read fail due to socket be closed\n");
            i4TotalReadLen = i4ReadLen; // keep read fail return value
            break; // exit loop
        }
        else
        {
            i4TotalReadLen += i4ReadLen;
        }
        //   LOGD("nfc_socket_read ok (read len: %d, target len: %d)\n", i4TotalReadLen, i4RecvLen);
    }

    return i4TotalReadLen;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_adp_sys_socket_read
 * DESCRIPTION
 *  Write socket
 * PARAMETERS
 *  csock       [IN] sockfd
 *  pSendBuff   [IN] socket send buffer
 *  i4SendLen   [IN] socket send length
 * RETURNS
 *  MTK_NFC_SUCCESS
 *****************************************************************************/
int mtk_nfc_adp_sys_socket_write(int csock, unsigned char *pSendBuff, int i4SendLen)
{
    int ret = 0;

    // LOGD("nfc_socket_write...(pSendBuff:0x%x, i4SendLen:%d)\n", (unsigned int)pSendBuff, i4SendLen);

    if (csock < 0)
    {
        LOGD("nfc_socket_write fail: due to invalid csock: %d\n", csock);
        return -1;
    }

    /* send data to nfc daemon */

    ret = write(csock, pSendBuff, i4SendLen);

    if (ret < 0)
    {
        LOGD("nfc_socket_write fail: ret %d\n", ret);
    }
    else
    {
        //  LOGD("nfc_socket_write ok (send len: %d)\n", ret);
    }

    return ret;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_adp_sys_msg_recv
 * DESCRIPTION
 *  Recv a socket message from a task
 * PARAMETERS
 *  task_id     [IN] target task id
 *  msg         [IN] the receive message pointer
 * RETURNS
 *  MTK_NFC_SUCCESS
 *****************************************************************************/
INT32
mtk_nfc_adp_sys_msg_recv (
    MTK_NFC_TASKID_E task_id,
    MTK_NFC_MSG_T **msg
)
{
    int ret = 0;

    if (msg)
    {
        switch (task_id)
        {
            case MTK_NFC_TASKID_ADAPT:
            {
                ret = nfc_recv_msg(gconn_fd_tmp, msg);
                if (FALSE == ret)
                {
                    return MTK_NFC_ERROR;
                }
                else if (ret == (-1))
                {
                    return (-3);
                }
            }
            break;
            default:
                LOGD("adapt,recv,%d\n",task_id);
                break;
        }
    }
    else
    {
        return  MTK_NFC_ERROR;
    }

    return MTK_NFC_SUCCESS;
}

/*****************************************************************************
 * Function
 *  mtk_nfc_adp_sys_msg_send
 * DESCRIPTION
 *  Send a socket message to a task
 * PARAMETERS
 *  task_id     [IN] target task id
 *  msg         [IN] the send message
 * RETURNS
 *  MTK_NFC_SUCCESS
 *****************************************************************************/
INT32
mtk_nfc_adp_sys_msg_send (
    MTK_NFC_TASKID_E task_id,
    const MTK_NFC_MSG_T *msg
)
{
//    int ret = MTK_NFC_ERROR;

    if (msg)
    {
        switch(task_id)
        {
            case MTK_NFC_TASKID_SERVICE:
            {
                if (mtk_nfc_sys_msg_send(task_id, msg) == 0)
                {
                    return MTK_NFC_SUCCESS;
                }
            }
            break;

            // send msg to nfc demo tool (use socket)
            case MTK_NFC_TASKID_APP:
            {
                if (mtk_nfc_adp_sys_socket_write(gconn_fd_tmp,
                                                 (unsigned char *)msg, (NFC_MSG_HDR_SIZE + msg->length)) > 0)
                {
                    // free msg due to use socket
                    mtk_nfc_sys_mem_free( (VOID*)msg);

                    return MTK_NFC_SUCCESS;
                }

                // free msg due to use socket
                mtk_nfc_sys_mem_free( (VOID*)msg);
            }
            break;

            default:
                LOGD("adapt,send,%d \n", task_id );
                break;
        }
    }

    return MTK_NFC_ERROR;
}


