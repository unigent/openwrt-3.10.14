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
 *  nfcd_service.c
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

#include "mtk_app_service.h"


/// Porting  functions
mtk_nfc_app_service_sys_event_handle g_event_handle;
mtk_nfc_app_service_sys_send_handle g_send_handle;
mtk_nfc_app_service_sys_debug_handle g_dbg_output;

/// Status variables

UINT8 g_nfc_status = 0;// 0: nfc stop , 1: nfc start
UINT8 g_nfc_opt = 0;// Nfc Command:  Setuped by WiFi's  Iwpriv ra0 set NfcStatus=g_nfc_opt, 0xC0~0xCF is in test mode;

UINT8 g_nfc_device = 0; // 1: reader, 2:p2p
UINT8 g_nfc_restart = 0; // Restart flag
UINT8 ReaderOpt = 0; // 1: read , 2:write , 3:format
INT32 p2p_working_handle = 0;  // From client RSP , Client's handle
INT32 p2p_server_handle = 0;

UINT32 p2p_sending = 0;
UINT8 p2p_mode = MTK_NFC_LLCP_SERVICEROLE_SERVER;  //1: server mode, 2 :client mode

UINT32 p2p_remote_miu = 128;   // From server RSP

UINT32 p2p_other_length = 0;
UINT8 u1SendOtherData[PAYLOAD_LENGTH];

UINT8 u1RecvData[PAYLOAD_LENGTH];  // Reader / P2P receive  buffer
UINT32 RecvLength = 0;             // Reader / P2P receive  buffer  length

UINT8 doneReading = 1;             // SNEP Reading fragmentation packet flag, 0 : Try to Receive the next PDU , 1 :Receive Done , 2 : Receive SNEP format error 3 :Wait SNEP_REQUEST_CONTINUE

#ifdef DEFAULT_SUPPORT_HO
UINT8 SerNameH[] = "urn:nfc:sn:handover";  // Default LLCP service name , Default Handover
UINT8 u1SAP = 0x10;                        // Default LLCP  SAP, Default Handover
#else
UINT8 SerNameH[] = "urn:nfc:sn:snep";  // Default LLCP service name , Default SNEP
UINT8 u1SAP = 0x04;                        // Default LLCP  SAP, Default SNEP
#endif

UINT16 RequestRandom = 0;
UINT8 LocalPower[MTK_NFCHO_RADIO_END] = {0x01,0x01,0x01,0x01};

UINT8 RemoteCarrierData[MTK_NFCHO_RADIO_END];

UINT8 RemoteRadioType[MTK_NFCHO_RADIO_END]; // MTK_NFCHO_RADIO_TYPE
UINT8 LocalRadioType[MTK_NFCHO_RADIO_END]; // MTK_NFCHO_RADIO_TYPE
UINT8 TotalRadioSize = 0;
UINT8 IsNegoHandover = 1;

UINT8* SelectData[MTK_NFCHO_RADIO_END];    // Radio's Carrier data buffer
UINT32 SelectDataLength[MTK_NFCHO_RADIO_END];         // Radio's Carrier data buffer length

s_mtk_nfc_get_selist_rsp grSeListInfor;


INT32 mtk_nfc_app_service_stop(UINT8 Action)
{
#ifdef SUPPORT_EM_CMD
    return mtk_nfcho_service_nfc_start(Action);
#else
    Action = 0;
    return mtk_nfcho_service_nfc_deactive();
#endif
}


INT32 mtk_nfc_app_service_start(UINT8 Action)
{


#ifdef SUPPORT_EM_CMD
    return mtk_nfcho_service_nfc_start(Action);

#else
    g_nfc_opt = Action;
    return mtk_nfcho_service_nfc_op(Action);
#endif
}


INT32 mtk_nfc_app_service_read(UINT8 Action)
{

    mtk_nfcho_service_dbg_output("read,%d,%d", g_nfc_device, Action);

    if (Action == 0x01)
    {
        if (g_nfc_device == 1)
        {
            mtk_nfcho_service_tag_read();
        }
    }
    return 0;

}



INT32 mtk_nfc_app_service_enter(MTK_NFC_APP_FUNCTION_POINT* pFunList, UINT8 TestMode)
{

    UINT8  Ret = 0x00;

#ifdef SUPPORT_EM_CMD

    if (pFunList != NULL)
    {
        g_event_handle = pFunList->event;
        g_send_handle = pFunList->send;
        g_dbg_output = pFunList->debug;

        mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_RESULT, MTK_NFC_ENTER_RSP,0 , 1, &Ret );
    }
    else
    {
        Ret = 0xFF;
    }
#else
    UINT8 Action = 1;

    if (pFunList != NULL)
    {
        g_event_handle = pFunList->event;
        g_send_handle = pFunList->send;
        g_dbg_output = pFunList->debug;
        mtk_nfcho_service_dbg_output("nfc_enter");
        if (TestMode == 0)
        {
            mtk_nfcho_service_nfc_start(Action);
        }
        else
        {
            g_nfc_opt = TestMode; // Test Mode
            Ret = (UINT8)mtk_nfcho_service_nfc_hwtest(TestMode);
            if (Ret != 0)
            {
                Ret = 0xFF;
            }
        }
    }
    else
    {
        Ret = 0xFF;
    }
#endif
    return (INT32)Ret;
}



#ifdef SUPPORT_EM_CMD
INT32 mtk_nfc_app_service_exit(void)
{
    UINT32 u4Length = 0;

    mtk_nfcho_service_dbg_output("nfc_exit");

    mtk_nfcho_service_send_msg(MTK_NFC_EXIT_REQ, NULL, u4Length);

    return 0;
}


#else

INT32 mtk_nfcho_service_deinit(void)
{
    s_mtk_nfc_dev_conn_req ReqData;
    UINT32 u4Length = 0;

    u4Length = sizeof(s_mtk_nfc_dev_conn_req);
    memset(&ReqData, 0x00, u4Length);
    ReqData.isconnect = 0;
    mtk_nfcho_service_dbg_output("nfc_de-init");

    mtk_nfcho_service_send_msg(MTK_NFC_CHIP_CONNECT_REQ, (UINT8*)&ReqData, u4Length);

    return 0;
}


INT32 mtk_nfcho_service_exit(void)
{
    UINT32 u4Length = 0;

    mtk_nfcho_service_dbg_output("nfc_exit,0x%x\n", g_nfc_opt);
    if ((g_nfc_opt == MTK_NFC_START_HWTEST_DEP) ||
        (g_nfc_opt == MTK_NFC_START_HWTEST_SWP) ||
        (g_nfc_opt == MTK_NFC_START_HWTEST_CARD) ||
        (g_nfc_opt == MTK_NFC_START_HWTEST_VCARD))
    {
        u4Length = 0;
        mtk_nfcho_service_send_msg(MTK_NFC_FM_STOP_CMD, NULL, u4Length);
        g_nfc_opt = 0;

    }
    else
    {
        u4Length = 0;
        mtk_nfcho_service_send_msg(MTK_NFC_STOP_REQ, NULL, u4Length);
        g_nfc_opt = 0;
    }


    return 0;
}


INT32 mtk_nfc_app_service_exit(void)
{
    mtk_nfcho_service_deinit();
    return 0;
}

#endif


INT32 mtk_nfc_app_service_write(MTK_NFC_APP_WRITEATA_T* pData)
{

    UINT8* pOutData = NULL;
    UINT32 OutLength = 0, OutIdx = 0 ;
    INT32 ret = 0;
    UINT16 u2TestTemp = 0;
    mtk_nfcho_ndefrecord_t Record;
    UINT8* TmpBuf = NULL;
    UINT8 RadioIdx = 0;


    if ((pData != NULL) && (pData->non_ndef == 0) && (pData->num_payload > 0))
    {
        mtk_nfcho_service_dbg_output("write ndef,%d", pData->num_payload);
        pOutData = (UINT8*)malloc(DEFAULT_ACCEPTABLE_LENGTH);
        if ( pOutData != NULL)
        {
            memset(pOutData, 0x00, DEFAULT_ACCEPTABLE_LENGTH);
            OutLength = 0;

            if (g_nfc_device == 1) // Write Tag
            {
                ret = (-1);

                if (pData->negohandover == 2)
                {
                    ret = mtk_nfcho_handover_select_build(pData->num_payload, LocalPower, pData->radio_type, pData->payload, &OutLength, pOutData);
                }
                else if (pData->negohandover == 1) // For Test request records
                {
                    ret = mtk_nfcho_handover_request_build(pData->num_payload, LocalPower, pData->radio_type, pData->payload, &OutLength, pOutData, &u2TestTemp);
                }
                else if ((pData->negohandover == 0) || (pData->negohandover == 3))
                {
                    for (RadioIdx = 0 ; RadioIdx < pData->num_payload; RadioIdx++)
                    {
                        if (pData->radio_type[RadioIdx] < MTK_NFCHO_RADIO_END)
                        {
                            if ((pData->payload[RadioIdx].data != NULL) && (pData->payload[RadioIdx].lth != 0))
                            {
                                OutLength = 0;
                                ret = mtk_nfcho_handover_token_build(pData->payload[RadioIdx].data, pData->payload[RadioIdx].lth, (pOutData+OutIdx), &OutLength, pData->radio_type[RadioIdx], RadioIdx, pData->num_payload);
                                OutIdx = OutIdx + OutLength;
                            }
                            else
                            {
                                ret = (-2);
                                break;
                            }
                        }
                        else if ((pData->data_type[RadioIdx] == MTK_NFC_HO_TXT_RTD) || (pData->data_type[RadioIdx]  == MTK_NFC_HO_URI_RTD))
                        {

                            UINT8 TypeT[] = "T";
                            UINT8 TypeU[] = "U";
                            UINT32 TmpL = 0;

                            mtk_nfcho_service_dbg_output("TXT URI,%d,%d",RadioIdx, pData->payload[RadioIdx].lth);
                            if (pData->data_type[RadioIdx] == MTK_NFC_HO_TXT_RTD)
                            {
                                TmpL = ( pData->payload[RadioIdx].lth+3);
                            }
                            else
                            {
                                TmpL = ( pData->payload[RadioIdx].lth+1);
                            }

                            TmpBuf = (UINT8*)malloc(TmpL);

                            if (TmpBuf != NULL)
                            {
                                memset(TmpBuf, 0x00, TmpL);
                                memset(&Record, 0x00, sizeof(mtk_nfcho_ndefrecord_t));

                                Record.Flags = 0;
                                if (pData->payload[RadioIdx].lth < 256)
                                {
                                    Record.Flags =  (Record.Flags | (MTK_NFC_NDEFRECORD_FLAGS_SR));
                                }
                                if (RadioIdx == 0)
                                {
                                    Record.Flags = (Record.Flags |(MTK_NFC_NDEFRECORD_FLAGS_MB));
                                }
                                if (RadioIdx == (pData->num_payload-1))
                                {
                                    Record.Flags = (Record.Flags |(MTK_NFC_NDEFRECORD_FLAGS_ME));
                                }


                                Record.Tnf = MTK_NFC_HO_NFCWELLKNOWN_RTD;
                                if (pData->data_type[RadioIdx] == MTK_NFC_HO_TXT_RTD)
                                {
                                    TmpBuf[0] = 'e';
                                    TmpBuf[1] = 'n';
                                    TmpBuf[2] = 0x00;

                                    Record.Type = TypeT;
                                    Record.TypeLength = 1;
                                    Record.PayloadLength = TmpL;
                                    memcpy(&TmpBuf[3],  pData->payload[RadioIdx].data,  pData->payload[RadioIdx].lth);
                                }
                                else
                                {
                                    TmpBuf[0] = 0x01;
                                    Record.Type = TypeU;
                                    Record.TypeLength = 1;
                                    Record.PayloadLength = TmpL;
                                    memcpy(&TmpBuf[1], pData->payload[RadioIdx].data,  pData->payload[RadioIdx].lth);
                                }

                                Record.PayloadData = TmpBuf;
                                OutLength = 0;
                                ret = mtk_nfchod_NdefRecord_Generate(&Record, (pOutData+OutIdx), (DEFAULT_ACCEPTABLE_LENGTH-OutIdx), &OutLength);
                                OutIdx = OutIdx + OutLength;
                                free(TmpBuf);
                                TmpBuf = NULL;
                            }
                        }
                        else  if (pData->data_type[RadioIdx] <= MTK_NFC_HO_RESERVED_RTD)
                        {
                            mtk_nfcho_ndefrecord_t Record;

                            mtk_nfcho_service_dbg_output("Others,%d,0x%x,%d,%d",RadioIdx,pData->data_type[RadioIdx] , pData->payload[RadioIdx].lth, pData->payload_type[RadioIdx].lth);
                            memset(&Record, 0x00, sizeof(mtk_nfcho_ndefrecord_t));
                            Record.Flags = 0;
                            if (pData->payload[RadioIdx].lth < 256)
                            {
                                Record.Flags =  (Record.Flags | (MTK_NFC_NDEFRECORD_FLAGS_SR));
                            }
                            if (RadioIdx == 0)
                            {
                                Record.Flags = (Record.Flags |(MTK_NFC_NDEFRECORD_FLAGS_MB));
                            }
                            if (RadioIdx == (pData->num_payload-1))
                            {
                                Record.Flags = (Record.Flags |(MTK_NFC_NDEFRECORD_FLAGS_ME));
                            }

                            Record.Tnf = pData->data_type[RadioIdx];
                            Record.PayloadLength = pData->payload[RadioIdx].lth;
                            Record.PayloadData = pData->payload[RadioIdx].data;
                            if ((pData->payload_type[RadioIdx].lth != 0) && (pData->payload_type[RadioIdx].data != NULL))
                            {
                                Record.TypeLength = (UINT8)pData->payload_type[RadioIdx].lth;
                                Record.Type = pData->payload_type[RadioIdx].data;
                            }
                            OutLength = 0;
                            ret = mtk_nfchod_NdefRecord_Generate(&Record, (pOutData+OutIdx), (DEFAULT_ACCEPTABLE_LENGTH-OutIdx), &OutLength);
                            OutIdx = OutIdx + OutLength;
                        }
                        else
                        {
                            ret = (-1);
                            break;
                        }
                    }
                }
                if (ret == 0) //Format OK
                {
                    mtk_nfcho_service_tag_write(pOutData, OutIdx);
                }
                else //Format Error
                {
                    ret = (-1);
                }
            }
            else if ((g_nfc_device == 2) &&  (pData->num_payload > 0) )// P2P device
            {
                memset(SelectData, 0x00, sizeof(UINT8*)*MTK_NFCHO_RADIO_END);
                memset(SelectDataLength, 0x00, sizeof(UINT32)*MTK_NFCHO_RADIO_END);

                if ((pData->negohandover == 2) || (pData->negohandover == 1))
                {
                    TotalRadioSize = pData->num_payload;
                    memset(LocalRadioType, 0xFF, sizeof(UINT8)*MTK_NFCHO_RADIO_END);

                    for ( RadioIdx = 0; RadioIdx <  pData->num_payload ; RadioIdx++)
                    {
                        LocalRadioType[RadioIdx] = pData->radio_type[RadioIdx];

                        SelectDataLength[RadioIdx] = pData->payload[RadioIdx].lth;
                        SelectData[RadioIdx] = (UINT8*)malloc(pData->payload[RadioIdx].lth);
                        memset(SelectData[RadioIdx], 0x00, SelectDataLength[RadioIdx]);
                        memcpy(SelectData[RadioIdx], pData->payload[RadioIdx].data, SelectDataLength[RadioIdx]);
                        mtk_nfcho_service_dbg_output("p2p,ho,L,%d", SelectDataLength[RadioIdx]);
                    }
                }
                else if ((pData->negohandover == 0) || (pData->negohandover == 3))
                {
                    TotalRadioSize = 1;
                    memset(LocalRadioType, 0xFF, sizeof(UINT8)*MTK_NFCHO_RADIO_END);

                    for ( RadioIdx = 0; RadioIdx <  pData->num_payload ; RadioIdx++)
                    {
                        if (pData->radio_type[RadioIdx] < MTK_NFCHO_RADIO_END)
                        {
                            if ((pData->payload[RadioIdx].data != NULL) && (pData->payload[RadioIdx].lth != 0))
                            {
                                OutLength = 0;
                                ret = mtk_nfcho_handover_token_build(pData->payload[RadioIdx].data, pData->payload[RadioIdx].lth, (pOutData+OutIdx), &OutLength, pData->radio_type[RadioIdx], RadioIdx, pData->num_payload);
                                OutIdx = OutIdx + OutLength;
                            }
                            else
                            {
                                ret = (-2);
                                break;
                            }
                        }
                        else if ((pData->data_type[RadioIdx] == MTK_NFC_HO_TXT_RTD) || (pData->data_type[RadioIdx]  == MTK_NFC_HO_URI_RTD))
                        {
                            UINT8 TypeT[] = "T";
                            UINT8 TypeU[] = "U";
                            UINT32 TmpL = 0;

                            if (pData->data_type[RadioIdx] == MTK_NFC_HO_TXT_RTD)
                            {
                                TmpL = (pData->payload[RadioIdx].lth+3);
                            }
                            else
                            {
                                TmpL = (pData->payload[RadioIdx].lth+1);
                            }
                            TmpBuf = (UINT8*)malloc(TmpL);
                            if (TmpBuf != NULL)
                            {
                                memset(TmpBuf, 0x00, TmpL);
                                memset(&Record, 0x00, sizeof(mtk_nfcho_ndefrecord_t));

                                Record.Flags = 0;
                                if (pData->payload[RadioIdx].lth < 256)
                                {
                                    Record.Flags =  (Record.Flags | (MTK_NFC_NDEFRECORD_FLAGS_SR));
                                }
                                if (RadioIdx == 0)
                                {
                                    Record.Flags = (Record.Flags |(MTK_NFC_NDEFRECORD_FLAGS_MB));
                                }
                                if (RadioIdx == (pData->num_payload-1))
                                {
                                    Record.Flags = (Record.Flags |(MTK_NFC_NDEFRECORD_FLAGS_ME));
                                }
                                Record.Tnf = MTK_NFC_HO_NFCWELLKNOWN_RTD;
                                if (pData->data_type[RadioIdx] == MTK_NFC_HO_TXT_RTD)
                                {
                                    TmpBuf[0] = 'e';
                                    TmpBuf[1] = 'n';
                                    TmpBuf[2] = 0x00;

                                    Record.Type = TypeT;
                                    Record.TypeLength = 1;
                                    Record.PayloadLength = TmpL;
                                    memcpy(&TmpBuf[3],  pData->payload[RadioIdx].data, pData->payload[RadioIdx].lth);
                                }
                                else
                                {
                                    TmpBuf[0] = 0x01;

                                    Record.Type = TypeU;
                                    Record.TypeLength = 1;
                                    Record.PayloadLength = TmpL;
                                    memcpy(&TmpBuf[1], pData->payload[RadioIdx].data, pData->payload[RadioIdx].lth);
                                }
                                Record.PayloadData = TmpBuf;
                                OutLength = 0;
                                ret = mtk_nfchod_NdefRecord_Generate(&Record, (pOutData+OutIdx), (DEFAULT_ACCEPTABLE_LENGTH-OutIdx), &OutLength);
                                OutIdx = OutIdx + OutLength;
                                free(TmpBuf);
                                TmpBuf = NULL;
                            }
                            else
                            {
                                ret = (-1);
                                break;
                            }
                        }
                        else  if (pData->data_type[RadioIdx] <= MTK_NFC_HO_RESERVED_RTD)
                        {
                            memset(&Record, 0x00, sizeof(mtk_nfcho_ndefrecord_t));
                            Record.Flags = 0;
                            if (pData->payload[RadioIdx].lth < 256)
                            {
                                Record.Flags =  (Record.Flags | (MTK_NFC_NDEFRECORD_FLAGS_SR));
                            }
                            if (RadioIdx == 0)
                            {
                                Record.Flags = (Record.Flags |(MTK_NFC_NDEFRECORD_FLAGS_MB));
                            }
                            if (RadioIdx == (pData->num_payload-1))
                            {
                                Record.Flags = (Record.Flags |(MTK_NFC_NDEFRECORD_FLAGS_ME));
                            }

                            Record.Tnf = pData->data_type[RadioIdx];
                            Record.PayloadLength = pData->payload[RadioIdx].lth;
                            Record.PayloadData = pData->payload[RadioIdx].data;
                            if ((pData->payload_type[RadioIdx].lth != 0) && (pData->payload_type[RadioIdx].data != NULL))
                            {
                                Record.TypeLength = (UINT8)pData->payload_type[RadioIdx].lth;
                                Record.Type = pData->payload_type[RadioIdx].data;
                            }
                            OutLength = 0;
                            ret = mtk_nfchod_NdefRecord_Generate(&Record, (pOutData+OutIdx), (DEFAULT_ACCEPTABLE_LENGTH-OutIdx), &OutLength);
                            OutIdx = OutIdx + OutLength;

                        }
                    }
                    SelectDataLength[0] = OutIdx;
                    SelectData[0] = (UINT8*)malloc(OutIdx);
                    memset(SelectData[0] , 0x00, OutIdx);
                    memcpy(SelectData[0] , pOutData, OutIdx);
                }

                if (ret == 0)
                {
                    if ((pData->negohandover == 1) || (pData->negohandover == 2))// // ==> Yes nego handover
                    {
                        IsNegoHandover = 1;
                    }
                    else// ==> Not nego handover
                    {
                        IsNegoHandover = 0;
                    }
                    if ((pData->negohandover == 0) || (pData->negohandover == 1) || (pData->negohandover == 3))
                    {
                        // 1 : handover requester: Trigger Client mode, 0: common ndef client mode, 3 :static handover format
                        mtk_nfcho_service_p2p_create_client();
                    }
                }
                else
                {
                    ret = (-2); //Format Fail
                }
            }
            free(pOutData);
            pOutData = NULL;
        }
        else
        {
            ret = (-3);
        }
    }
    else
    {
        ret = (-4);
    }
    return ret;

}

INT32 mtk_nfc_app_service_card_get_selist(void)
{

    mtk_nfcho_service_send_msg(MTK_NFC_GET_SELIST_REQ, NULL, 0);

    return 0;

}

INT32 mtk_nfc_app_service_card_enable(unsigned char SEid, unsigned char Action)
{

    UINT32 u4Length = 0;
    s_mtk_nfc_jni_se_set_mode_req_t ReqData;


    u4Length = sizeof(ReqData);

    memset(&ReqData, 0x00, u4Length);

    ReqData.enable = Action;
    ReqData.seid = SEid;
    mtk_nfcho_service_send_msg(MTK_NFC_SE_MODE_SET_REQ, (UINT8*)&ReqData, u4Length);

    return 0;

}

#ifdef SUPPORT_EM_CMD

INT32 mtk_nfcho_service_nfc_start(UINT8 Action)
{
    s_mtk_nfc_em_polling_req ReqData;
    UINT32 u4Length = sizeof(s_mtk_nfc_em_polling_req);

    memset(&ReqData, 0x00, u4Length);
    if ((Action != MTK_WIFINFC_READERTAG) && (Action != MTK_WIFINFC_FORMATTAG))
    {
        g_nfc_opt = Action;
    }
    mtk_nfcho_service_dbg_output("setup,%d", g_nfc_opt);
    if (Action == MTK_WIFINFC_SET_ON)
    {
        ReqData.action = NFC_EM_ACT_START;
        ReqData.enablefunc = (EM_ENABLE_FUNC_READER_MODE|EM_ENABLE_FUNC_P2P_MODE);
        ReqData.p2pM.action = NFC_EM_ACT_START;
        ReqData.p2pM.isDisableCardM = 1;
        ReqData.p2pM.mode = EM_P2P_MODE_PASSIVE_MODE;     /* BITMAPS bit0: Passive mode, bit1: Active mode*/
        ReqData.p2pM.role = (EM_P2P_ROLE_TARGET_MODE | EM_P2P_ROLE_INITIATOR_MODE);     /* BITMAPS bit0: Initator, bit1: Target*/
        ReqData.p2pM.supporttype = (EM_ALS_READER_M_TYPE_F | EM_ALS_READER_M_TYPE_A);
        ReqData.p2pM.typeF_datarate = (EM_ALS_READER_M_SPDRATE_424 | EM_ALS_READER_M_SPDRATE_212);
        ReqData.p2pM.typeA_datarate = EM_ALS_READER_M_SPDRATE_106;
        ReqData.Period = 0x44;
        ReqData.phase  = 0;
        ReqData.readerM.action = NFC_EM_ACT_START;
        ReqData.readerM.supporttype = (EM_ALS_READER_M_TYPE_A |EM_ALS_READER_M_TYPE_B);
        ReqData.readerM.typeA_datarate = (EM_ALS_READER_M_SPDRATE_106);
        ReqData.readerM.typeB_datarate = (EM_ALS_READER_M_SPDRATE_106);
        mtk_nfcho_service_send_msg(MTK_NFC_EM_POLLING_MODE_REQ, (UINT8*)&ReqData, u4Length));
    }
    else if (Action == MTK_WIFINFC_SET_READER)
    {
        ReqData.action = NFC_EM_ACT_START;
        ReqData.enablefunc = EM_ENABLE_FUNC_READER_MODE;
        ReqData.Period = 0;
        ReqData.phase  = 1;
        ReqData.readerM.action = NFC_EM_ACT_START;
        ReqData.readerM.supporttype = (EM_ALS_READER_M_TYPE_A |EM_ALS_READER_M_TYPE_B);
        ReqData.readerM.typeA_datarate = (EM_ALS_READER_M_SPDRATE_106);
        ReqData.readerM.typeB_datarate = (EM_ALS_READER_M_SPDRATE_106);
        mtk_nfcho_service_send_msg(MTK_NFC_EM_POLLING_MODE_REQ, (UINT8*)&ReqData, u4Length));
    }
    else if ((Action == MTK_WIFINFC_SET_P2PSERVER) || (Action == MTK_WIFINFC_SET_P2PCLIENT))
    {
        ReqData.action = NFC_EM_ACT_START;
        ReqData.enablefunc = EM_ENABLE_FUNC_P2P_MODE;
        ReqData.p2pM.action = NFC_EM_ACT_START;
        ReqData.p2pM.isDisableCardM = 1;
        ReqData.p2pM.mode = EM_P2P_MODE_PASSIVE_MODE;     /* BITMAPS bit0: Passive mode, bit1: Active mode*/
        ReqData.p2pM.role = (EM_P2P_ROLE_TARGET_MODE | EM_P2P_ROLE_INITIATOR_MODE);    /* BITMAPS bit0: Initator, bit1: Target*/
        ReqData.p2pM.supporttype = EM_ALS_READER_M_TYPE_F;
        ReqData.p2pM.typeF_datarate = EM_ALS_READER_M_SPDRATE_424;
        ReqData.Period = 0x44;
        ReqData.phase  = 0;
        mtk_nfcho_service_send_msg(MTK_NFC_EM_POLLING_MODE_REQ, (UINT8*)&ReqData, u4Length));
    }
    else if ((Action == MTK_WIFINFC_SET_P2PSERVER_T) || (Action == MTK_WIFINFC_SET_P2PCLIENT_T))
    {
        ReqData.action = NFC_EM_ACT_START;
        ReqData.enablefunc = EM_ENABLE_FUNC_P2P_MODE;
        ReqData.p2pM.action = NFC_EM_ACT_START;
        ReqData.p2pM.isDisableCardM = 1;
        ReqData.p2pM.mode = EM_P2P_MODE_PASSIVE_MODE;     /* BITMAPS bit0: Passive mode, bit1: Active mode*/
        ReqData.p2pM.role = EM_P2P_ROLE_TARGET_MODE;// EM_P2P_ROLE_INITIATOR_MODE;     /* BITMAPS bit0: Initator, bit1: Target*/
        ReqData.p2pM.supporttype = EM_ALS_READER_M_TYPE_F;
        ReqData.p2pM.typeF_datarate = EM_ALS_READER_M_SPDRATE_424;
        ReqData.Period = 0x44;
        ReqData.phase  = 0;
        mtk_nfcho_service_send_msg(MTK_NFC_EM_POLLING_MODE_REQ, (UINT8*)&ReqData, u4Length));
    }
    else if ((Action == MTK_WIFINFC_SET_P2PSERVER_I) || (Action == MTK_WIFINFC_SET_P2PCLIENT_I))
    {
        ReqData.action = NFC_EM_ACT_START;
        ReqData.enablefunc = EM_ENABLE_FUNC_P2P_MODE;
        ReqData.p2pM.action = NFC_EM_ACT_START;
        ReqData.p2pM.isDisableCardM = 1;
        ReqData.p2pM.mode = EM_P2P_MODE_PASSIVE_MODE;     /* BITMAPS bit0: Passive mode, bit1: Active mode*/
        ReqData.p2pM.role = EM_P2P_ROLE_INITIATOR_MODE;//     /* BITMAPS bit0: Initator, bit1: Target*/
        ReqData.p2pM.supporttype = EM_ALS_READER_M_TYPE_F;
        ReqData.p2pM.typeF_datarate = EM_ALS_READER_M_SPDRATE_424;
        ReqData.Period = 0;
        ReqData.phase  = 1;
        mtk_nfcho_service_send_msg(MTK_NFC_EM_POLLING_MODE_REQ, (UINT8*)&ReqData, u4Length));
    }
    else if ((Action == MTK_WIFINFC_SET_READER_P2PSERVER) || (Action == MTK_WIFINFC_SET_READERP2PCLIENT))
    {
        ReqData.action = NFC_EM_ACT_START;
        ReqData.enablefunc = (EM_ENABLE_FUNC_READER_MODE|EM_ENABLE_FUNC_P2P_MODE);
        ReqData.p2pM.action = NFC_EM_ACT_START;
        ReqData.p2pM.isDisableCardM = 1;
        ReqData.p2pM.mode = EM_P2P_MODE_PASSIVE_MODE;     /* BITMAPS bit0: Passive mode, bit1: Active mode*/
        ReqData.p2pM.role = (EM_P2P_ROLE_TARGET_MODE|EM_P2P_ROLE_INITIATOR_MODE);     /* BITMAPS bit0: Initator, bit1: Target*/
        ReqData.p2pM.supporttype = (EM_ALS_READER_M_TYPE_F | EM_ALS_READER_M_TYPE_A);
        ReqData.p2pM.typeF_datarate = (EM_ALS_READER_M_SPDRATE_424 | EM_ALS_READER_M_SPDRATE_212);
        ReqData.p2pM.typeA_datarate = EM_ALS_READER_M_SPDRATE_106;
        ReqData.Period = 0x44;
        ReqData.phase  = 0;
        ReqData.readerM.action = NFC_EM_ACT_START;
        ReqData.readerM.supporttype = (EM_ALS_READER_M_TYPE_A |EM_ALS_READER_M_TYPE_B);
        ReqData.readerM.typeA_datarate = (EM_ALS_READER_M_SPDRATE_106);
        ReqData.readerM.typeB_datarate = (EM_ALS_READER_M_SPDRATE_106);
        mtk_nfcho_service_send_msg(MTK_NFC_EM_POLLING_MODE_REQ, (UINT8*)&ReqData, u4Length));
    }
    else if ((Action == MTK_WIFINFC_SET_READER_P2PSERVER_T) || (Action == MTK_WIFINFC_SET_READERP2PCLIENT_T))
    {
        ReqData.action = NFC_EM_ACT_START;
        //     ReqData.cardM.action = NFC_EM_ACT_STOP;
        ReqData.enablefunc = (EM_ENABLE_FUNC_READER_MODE|EM_ENABLE_FUNC_P2P_MODE);
        ReqData.p2pM.action = NFC_EM_ACT_START;
        ReqData.p2pM.isDisableCardM = 1;
        ReqData.p2pM.mode = EM_P2P_MODE_PASSIVE_MODE;     /* BITMAPS bit0: Passive mode, bit1: Active mode*/
        ReqData.p2pM.role = EM_P2P_ROLE_TARGET_MODE;// EM_P2P_ROLE_INITIATOR_MODE;     /* BITMAPS bit0: Initator, bit1: Target*/
        ReqData.p2pM.supporttype = (EM_ALS_READER_M_TYPE_F | EM_ALS_READER_M_TYPE_A);
        ReqData.p2pM.typeF_datarate = (EM_ALS_READER_M_SPDRATE_424 | EM_ALS_READER_M_SPDRATE_212);
        ReqData.p2pM.typeA_datarate = EM_ALS_READER_M_SPDRATE_106;
        ReqData.Period = 0x44;
        ReqData.phase  = 0;
        ReqData.readerM.action = NFC_EM_ACT_START;
        ReqData.readerM.supporttype = (EM_ALS_READER_M_TYPE_A |EM_ALS_READER_M_TYPE_B);
        ReqData.readerM.typeA_datarate = (EM_ALS_READER_M_SPDRATE_106);
        ReqData.readerM.typeB_datarate = (EM_ALS_READER_M_SPDRATE_106);
        mtk_nfcho_service_send_msg(MTK_NFC_EM_POLLING_MODE_REQ, (UINT8*)&ReqData, u4Length));
    }
    else if ((Action == MTK_WIFINFC_SET_READER_P2PSERVER_I) || (Action == MTK_WIFINFC_SET_READERP2PCLIENT_I))
    {
        ReqData.action = NFC_EM_ACT_START;
        //     ReqData.cardM.action = NFC_EM_ACT_STOP;
        ReqData.enablefunc = (EM_ENABLE_FUNC_READER_MODE|EM_ENABLE_FUNC_P2P_MODE);
        ReqData.p2pM.action = NFC_EM_ACT_START;
        ReqData.p2pM.isDisableCardM = 1;
        ReqData.p2pM.mode = EM_P2P_MODE_PASSIVE_MODE;     /* BITMAPS bit0: Passive mode, bit1: Active mode*/
        ReqData.p2pM.role = EM_P2P_ROLE_INITIATOR_MODE;// EM_P2P_ROLE_INITIATOR_MODE;     /* BITMAPS bit0: Initator, bit1: Target*/
        ReqData.p2pM.supporttype = (EM_ALS_READER_M_TYPE_F | EM_ALS_READER_M_TYPE_A);
        ReqData.p2pM.typeF_datarate = (EM_ALS_READER_M_SPDRATE_424 | EM_ALS_READER_M_SPDRATE_212);
        ReqData.p2pM.typeA_datarate = EM_ALS_READER_M_SPDRATE_106;
        ReqData.Period = 0x44;
        ReqData.phase  = 0;
        ReqData.readerM.action = NFC_EM_ACT_START;
        ReqData.readerM.supporttype = (EM_ALS_READER_M_TYPE_A |EM_ALS_READER_M_TYPE_B);
        ReqData.readerM.typeA_datarate = (EM_ALS_READER_M_SPDRATE_106);
        ReqData.readerM.typeB_datarate = (EM_ALS_READER_M_SPDRATE_106);
        mtk_nfcho_service_send_msg(MTK_NFC_EM_POLLING_MODE_REQ, (UINT8*)&ReqData, u4Length));
    }
    else if (Action == MTK_WIFINFC_SET_OFF)
    {
        g_nfc_device = 0;
        ReqData.action = NFC_EM_ACT_STOP;

        mtk_nfcho_service_dbg_output("Trigger NFC Stop");
        mtk_nfcho_service_send_msg(MTK_NFC_EM_POLLING_MODE_REQ, (UINT8*)&ReqData, u4Length));
    }
    else if (Action == MTK_WIFINFC_FORMATTAG)
    {
        if (g_nfc_device == 1)
        {
            mtk_nfcho_service_tag_format();
        }
        else
        {
            mtk_nfcho_service_dbg_output("Cannot Format Without Tag");
        }
    }
    else if ((Action == MTK_WIFINFC_SET_P2PSERVER_A) || (Action == MTK_WIFINFC_SET_P2PCLIENT_A))
    {
        ReqData.action = NFC_EM_ACT_START;
        ReqData.enablefunc = EM_ENABLE_FUNC_P2P_MODE;
        ReqData.p2pM.action = NFC_EM_ACT_START;
        ReqData.p2pM.isDisableCardM = 1;
        ReqData.p2pM.mode = EM_P2P_MODE_PASSIVE_MODE;     /* BITMAPS bit0: Passive mode, bit1: Active mode*/
        ReqData.p2pM.role = (EM_P2P_ROLE_TARGET_MODE | EM_P2P_ROLE_INITIATOR_MODE);// EM_P2P_ROLE_INITIATOR_MODE;     /* BITMAPS bit0: Initator, bit1: Target*/
        ReqData.p2pM.supporttype = EM_ALS_READER_M_TYPE_A;
        ReqData.p2pM.typeA_datarate = (EM_ALS_READER_M_SPDRATE_106);
        ReqData.Period = 0x44;
        ReqData.phase  = 0;
        mtk_nfcho_service_send_msg(MTK_NFC_EM_POLLING_MODE_REQ, (UINT8*)&ReqData, u4Length));
    }
    else if (Action == MTK_WIFINFC_SET_P2PSERVER_212_I)
    {
        ReqData.action = NFC_EM_ACT_START;
        ReqData.enablefunc = EM_ENABLE_FUNC_P2P_MODE; //EM_ENABLE_FUNC_READER_MODE;//(EM_ENABLE_FUNC_READER_MODE|EM_ENABLE_FUNC_P2P_MODE);
        ReqData.p2pM.action = NFC_EM_ACT_START;
        ReqData.p2pM.isDisableCardM = 1;
        ReqData.p2pM.mode = EM_P2P_MODE_PASSIVE_MODE;     /* BITMAPS bit0: Passive mode, bit1: Active mode*/
        ReqData.p2pM.role = (EM_P2P_ROLE_INITIATOR_MODE);// EM_P2P_ROLE_INITIATOR_MODE;     /* BITMAPS bit0: Initator, bit1: Target*/
        ReqData.p2pM.supporttype = EM_ALS_READER_M_TYPE_F;
        ReqData.p2pM.typeF_datarate = EM_ALS_READER_M_SPDRATE_212;
        ReqData.Period = 0;
        ReqData.phase  = 1;
        mtk_nfcho_service_send_msg(MTK_NFC_EM_POLLING_MODE_REQ, (UINT8*)&ReqData, u4Length));
    }
    else if (Action == MTK_WIFINFC_SET_P2PSERVER_212_T)
    {
        ReqData.action = NFC_EM_ACT_START;
        ReqData.enablefunc = EM_ENABLE_FUNC_P2P_MODE; //EM_ENABLE_FUNC_READER_MODE;//(EM_ENABLE_FUNC_READER_MODE|EM_ENABLE_FUNC_P2P_MODE);
        ReqData.p2pM.action = NFC_EM_ACT_START;
        ReqData.p2pM.isDisableCardM = 1;
        ReqData.p2pM.mode = EM_P2P_MODE_PASSIVE_MODE;     /* BITMAPS bit0: Passive mode, bit1: Active mode*/
        ReqData.p2pM.role = (EM_P2P_ROLE_TARGET_MODE);// EM_P2P_ROLE_INITIATOR_MODE;     /* BITMAPS bit0: Initator, bit1: Target*/
        ReqData.p2pM.supporttype = EM_ALS_READER_M_TYPE_F;
        ReqData.p2pM.typeF_datarate = EM_ALS_READER_M_SPDRATE_212;
        ReqData.Period = 0x44;
        ReqData.phase  = 0;
        mtk_nfcho_service_send_msg(MTK_NFC_EM_POLLING_MODE_REQ, (UINT8*)&ReqData, u4Length));
    }

    return 0;
}

#else

INT32 mtk_nfcho_service_nfc_start(UINT8 Action)
{
    s_mtk_nfc_dev_conn_req Req;
    UINT32 Length = sizeof(s_mtk_nfc_dev_conn_req);
    INT32 ret = 0;

    mtk_nfcho_service_dbg_output("nfc_start");

    memset(&Req, 0x00, sizeof(s_mtk_nfc_dev_conn_req));

    Req.isconnect = Action;
    Req.dumpdata = 1;

#ifdef SUPPORT_I2C_IF
    Req.conntype = CONN_TYPE_I2C;
#else
    Req.conntype = CONN_TYPE_UART;
    Req.connid.type_uart.comport = 1;
    Req.connid.type_uart.buardrate = 115200;
#endif

    mtk_nfcho_service_send_msg(MTK_NFC_CHIP_CONNECT_REQ, (UINT8*)&Req, Length);

    return ret;
}


INT32 mtk_nfcho_service_nfc_hwtest(UINT8 TestMode)
{
    UINT32 Length = 0;
    INT32 ret = 0;

    if (TestMode == MTK_NFC_START_HWTEST_DEP)
    {
        s_mtk_nfc_em_als_readerm_req Req;
        Length =  sizeof(s_mtk_nfc_em_als_readerm_req);

        memset(&Req, 0x00, Length);

        mtk_nfcho_service_dbg_output("nfc_test,dep");

        Req.action = 0x00;
        Req.supporttype = EM_ALS_READER_M_TYPE_A;
        Req.typeA_datarate = (EM_ALS_READER_M_SPDRATE_106 + EM_ALS_READER_M_SPDRATE_212 + EM_ALS_READER_M_SPDRATE_424 + EM_ALS_READER_M_SPDRATE_848);
        mtk_nfcho_service_send_msg(MTK_NFC_FM_READ_DEP_TEST_REQ, (UINT8*)&Req, Length);

    }
    else  if (TestMode == MTK_NFC_START_HWTEST_SWP)
    {
        s_mtk_nfc_fm_swp_test_req Req;

        Length = sizeof(s_mtk_nfc_fm_swp_test_req);
        memset(&Req, 0x00, Length);

        mtk_nfcho_service_dbg_output("nfc_test,swp");
        Req.action = 1;
        Req.SEmap = 0x01;
        mtk_nfcho_service_send_msg(MTK_NFC_FM_SWP_TEST_REQ, (UINT8*)&Req, Length);

    }
    else  if (TestMode == MTK_NFC_START_HWTEST_CARD)
    {
        s_mtk_nfc_em_als_cardm_req Req;

        Length = sizeof(s_mtk_nfc_em_als_cardm_req);
        memset(&Req, 0x00, Length);
        mtk_nfcho_service_dbg_output("nfc_test,card");
        Req.action = 0x00;
        Req.SWNum = 1;
        Req.supporttype = 0x01; //type A/B/F/B'
        Req.fgvirtualcard = 0;
        mtk_nfcho_service_send_msg(MTK_NFC_FM_CARD_MODE_TEST_REQ, (UINT8*)&Req, Length);
    }
    else  if (TestMode == MTK_NFC_START_HWTEST_VCARD)
    {
        s_mtk_nfc_em_virtual_card_req Req;

        Length = sizeof(s_mtk_nfc_em_virtual_card_req);
        memset(&Req, 0x00, Length);
        mtk_nfcho_service_dbg_output("nfc_test,vcard");
        Req.action = 0x00;
        Req.supporttype = (EM_ALS_READER_M_TYPE_A  + EM_ALS_READER_M_TYPE_B  + EM_ALS_READER_M_TYPE_F);
        Req.typeF_datarate = (EM_ALS_READER_M_SPDRATE_212 + EM_ALS_READER_M_SPDRATE_424);

        mtk_nfcho_service_send_msg(MTK_NFC_FM_VIRTUAL_CARD_REQ, (UINT8*)&Req, Length);
    }
    else
    {
        mtk_nfcho_service_dbg_output("nfc_test,0x%x,NoSupport", TestMode );
        ret = (-1);
    }
    return ret;
}


INT32 mtk_nfcho_service_nfc_op(UINT8 Action)
{
    s_mtk_nfc_discovery_req Req;
    UINT32 Length = 0;
    INT32 ret = 0;

    mtk_nfcho_service_dbg_output("nfc_op,0x%x", Action); // 1: on , 0: off

    Length = sizeof(s_mtk_nfc_discovery_req);
    memset(&Req, 0x00, Length);
    Req.action = 1;
    if (Action == MTK_WIFINFC_SET_ON)
    {
        Req.tag_setting.autopolling = 1;
        Req.tag_setting.checkndef = 1;
        Req.tag_setting.status = 1;
        Req.tag_setting.supportmultupletag = 0;
        Req.tag_setting.tech_for_reader = (TYPE_A| TYPE_B|TYPE_F);
        Req.tag_setting.u1BitRateA = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateB = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateF = (EM_ALS_READER_M_SPDRATE_212);
        Req.tag_setting.u1CodingSelect = 1;
        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 2;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;
        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_SERVER;

    }
    else if (Action == MTK_WIFINFC_SET_P2PSERVER)
    {
        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 2;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;
        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_SERVER;
    }
    else if (Action == MTK_WIFINFC_SET_P2PSERVER_T)
    {
        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 1;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;
        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_SERVER;
    }
    else if (Action == MTK_WIFINFC_SET_READER_P2PSERVER_T)
    {
        Req.tag_setting.autopolling = 1;
        Req.tag_setting.checkndef = 1;
        Req.tag_setting.status = 1;
        Req.tag_setting.supportmultupletag = 0;
        Req.tag_setting.tech_for_reader = (TYPE_A| TYPE_B|TYPE_F);
        Req.tag_setting.u1BitRateA = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateB = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateF = (EM_ALS_READER_M_SPDRATE_212);
        Req.tag_setting.u1CodingSelect = 1;

        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 1;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;
        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_SERVER;
    }
    else if (Action == MTK_WIFINFC_SET_P2PSERVER_I)
    {
        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 0;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;
        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_SERVER;
    }
    else if (Action == MTK_WIFINFC_SET_READER_P2PSERVER_I)
    {
        Req.tag_setting.autopolling = 1;
        Req.tag_setting.checkndef = 1;
        Req.tag_setting.status = 1;
        Req.tag_setting.supportmultupletag = 0;
        Req.tag_setting.tech_for_reader = (TYPE_A| TYPE_B|TYPE_F);
        Req.tag_setting.u1BitRateA = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateB = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateF = (EM_ALS_READER_M_SPDRATE_212);
        Req.tag_setting.u1CodingSelect = 1;

        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 0;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;
        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_SERVER;
    }
    else if (Action == MTK_WIFINFC_SET_P2PCLIENT)
    {
        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 2;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;
        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_CLIENT;
    }
    else if (Action == MTK_WIFINFC_SET_READERP2PCLIENT)
    {
        Req.tag_setting.autopolling = 1;
        Req.tag_setting.checkndef = 1;
        Req.tag_setting.status = 1;
        Req.tag_setting.supportmultupletag = 0;
        Req.tag_setting.tech_for_reader = (TYPE_A| TYPE_B|TYPE_F);
        Req.tag_setting.u1BitRateA = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateB = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateF = (EM_ALS_READER_M_SPDRATE_212);
        Req.tag_setting.u1CodingSelect = 1;

        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 2;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;
        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_CLIENT;
    }
    else if (Action == MTK_WIFINFC_SET_P2PCLIENT_T)
    {
        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 1;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;
        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_CLIENT;
    }
    else if (Action == MTK_WIFINFC_SET_READERP2PCLIENT_T)
    {
        Req.tag_setting.autopolling = 1;
        Req.tag_setting.checkndef = 1;
        Req.tag_setting.status = 1;
        Req.tag_setting.supportmultupletag = 0;
        Req.tag_setting.tech_for_reader = (TYPE_A| TYPE_B|TYPE_F);
        Req.tag_setting.u1BitRateA = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateB = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateF = (EM_ALS_READER_M_SPDRATE_212);
        Req.tag_setting.u1CodingSelect = 1;

        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 1;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;
        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_CLIENT;
    }
    else if (Action == MTK_WIFINFC_SET_P2PCLIENT_I)
    {
        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 0;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;
        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_CLIENT;
    }
    else if (Action == MTK_WIFINFC_SET_READERP2PCLIENT_I)
    {
        Req.tag_setting.autopolling = 1;
        Req.tag_setting.checkndef = 1;
        Req.tag_setting.status = 1;
        Req.tag_setting.supportmultupletag = 0;
        Req.tag_setting.tech_for_reader = (TYPE_A| TYPE_B|TYPE_F);
        Req.tag_setting.u1BitRateA = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateB = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateF = (EM_ALS_READER_M_SPDRATE_212);
        Req.tag_setting.u1CodingSelect = 1;

        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 0;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;
        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_CLIENT;
    }
    else if (Action == MTK_WIFINFC_SET_CARD_READER_P2P)
    {
        Req.tag_setting.autopolling = 1;
        Req.tag_setting.checkndef = 1;
        Req.tag_setting.status = 1;
        Req.tag_setting.supportmultupletag = 0;
        Req.tag_setting.tech_for_reader = (TYPE_A| TYPE_B|TYPE_F);
        Req.tag_setting.u1BitRateA = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateB = (EM_ALS_READER_M_SPDRATE_106);
        Req.tag_setting.u1BitRateF = (EM_ALS_READER_M_SPDRATE_212);
        Req.tag_setting.u1CodingSelect = 1;

        Req.p2p_setting.status = 1;
        Req.p2p_setting.role = 2;
        Req.p2p_setting.bm_mode = EM_P2P_MODE_PASSIVE_MODE;

        Req.se_setting.status = 1;
        Req.se_setting.SE[0].seid = 0x12345678; // For Test Card mode command

        p2p_mode = MTK_NFC_LLCP_SERVICEROLE_SERVER;

    }
    else
    {
        Req.action = 0;
        g_nfc_device = 0;
    }
    mtk_nfcho_service_send_msg(MTK_NFC_DISCOVERY_REQ, (UINT8*)&Req, Length);

    return ret;
}

#endif


INT32 mtk_nfc_app_service_proc (UINT8* Data)
{

    MTK_NFC_MSG_T *pMainMsg = (MTK_NFC_MSG_T *)Data;
    UINT8* pBuf = NULL;
    UINT8  Ret = 0;
    INT32 Result = 0;

    switch(pMainMsg->type)
    {

#ifdef SUPPORT_EM_CMD

        case MTK_NFC_EM_POLLING_MODE_NTF:
        {
            s_mtk_nfc_em_polling_ntf* pPolling_Ntf = NULL;

            pPolling_Ntf = (s_mtk_nfc_em_polling_ntf*)((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));

            mtk_nfcho_service_dbg_output("Detected,%d,p2p", pPolling_Ntf->detecttype );
            if ((pPolling_Ntf->ntf.reader.result == 0) && (pPolling_Ntf->detecttype == EM_ENABLE_FUNC_READER_MODE))
            {
                g_nfc_device = 1;
                mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_FOUNDTAG, 0, 0, 0, NULL );
                mtk_nfcho_service_dbg_output("Tag");
            }
            else if ((pPolling_Ntf->ntf.p2p.link_status  == 1) && (pPolling_Ntf->detecttype == EM_ENABLE_FUNC_P2P_MODE))
            {
                g_nfc_device = 2;
                mtk_nfcho_service_p2p_create_server(SerNameH, strlen(SerNameH) ,u1SAP);
                mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_FOUNDDEVICE, 0, 0, 0, NULL );
                mtk_nfcho_service_dbg_output("Device");
            }
            else if ((pPolling_Ntf->ntf.p2p.link_status == 0) && (pPolling_Ntf->detecttype == EM_ENABLE_FUNC_P2P_MODE))
            {
                UINT8 idx =0;

                mtk_nfcho_service_dbg_output("link,%d", pPolling_Ntf->ntf.p2p.link_status);

                //Reset Variables
                memset((UINT8*)RemoteRadioType, 0xFF, sizeof(UINT8)*MTK_NFCHO_RADIO_END);
                g_nfc_device = 0;
                if (p2p_working_handle != 0)
                {
                    mtk_nfcho_snep_stop(p2p_working_handle);
                }
                if ( p2p_mode == MTK_NFC_LLCP_SERVICEROLE_CLIENT)
                {
                    if (p2p_server_handle != 0)
                    {
                        mtk_nfcho_snep_stop(p2p_server_handle);
                        p2p_server_handle = 0;
                    }
                }
                for (idx = 0 ; idx < MTK_NFCHO_RADIO_END ; idx++)
                {
                    if (SelectData[idx] != NULL)
                    {
                        free(SelectData[idx]);
                        SelectData[idx] = NULL;
                        SelectDataLength[idx] = 0;
                    }
                }
                TotalRadioSize = 0;
                IsNegoHandover = 0;
                mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_DEVICELEFT,0 , 0, 0, NULL );
            }
            else
            {
                mtk_nfcho_service_dbg_output("NotSupport");
                Result = (-1);
            }
        }
        break;

        case MTK_NFC_EM_ALS_READER_MODE_NTF:
        {
            s_mtk_nfc_em_als_readerm_ntf* pReaderm_Ntf = NULL;

            pReaderm_Ntf = (s_mtk_nfc_em_als_readerm_ntf*)((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));
            Ret = 0;

            if (pReaderm_Ntf != NULL)
            {
                mtk_nfcho_service_dbg_output("Device Disc,%d", pReaderm_Ntf->result);
                if (pReaderm_Ntf->result == 0x02)
                {
                    g_nfc_device = 0;
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_DEVICELEFT, Ret, 0, 0, NULL );
                }
            }
            else
            {
                mtk_nfcho_service_dbg_output("Device Disc Ntf,NULL");
                g_nfc_restart = 1;
                mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_DEVICELEFT, Ret, 0, 0, NULL );
                mtk_nfcho_service_nfc_start(0);
                Result = (-1);
            }
        }
        break;

        case MTK_NFC_EM_POLLING_MODE_RSP:
        {
            s_mtk_nfc_em_polling_rsp* pPolling_Rsp = NULL;

            pBuf = ((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));
            pPolling_Rsp = (s_mtk_nfc_em_polling_rsp*)pBuf;
            mtk_nfcho_service_dbg_output("POLLING_RSP,ret,%d", pPolling_Rsp->result);

            if (pPolling_Rsp->result == 0)
            {
                if (g_nfc_opt == MTK_WIFINFC_SET_OFF) // Stop OK
                {
                    g_nfc_status = 0;

                    if ( g_nfc_restart == 1)
                    {
                        mtk_nfcho_service_dbg_output("Restart,OK");
                        mtk_nfcho_service_nfc_start(1);
                        g_nfc_restart = 0;
                    }
                }
                else if (g_nfc_opt >= MTK_WIFINFC_SET_ON) //Start OK
                {
                    g_nfc_status = 1;
                }
                else
                {
                    g_nfc_status = 0;
                }
                Ret = 0;
            }
            else
            {
                Ret = 0xFF;
                Result = (-1);
            }

            mtk_nfcho_service_dbg_output("Ret,%d,status,%d,Opt,%d", Ret, g_nfc_status, g_nfc_opt);
            mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_RESULT, MTK_NFC_START_RSP, 0, 1, &Ret );
        }
        break;

        case MTK_NFC_EM_ALS_READER_MODE_OPT_RSP:
        {
            s_mtk_nfc_em_als_readerm_opt_rsp* pReader_Rsp = NULL;
            INT32 parse_reulst = 0;

            mtk_nfcho_service_dbg_output("TAG_RSP");
            pBuf = ((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));
            pReader_Rsp =(s_mtk_nfc_em_als_readerm_opt_rsp*)pBuf;

            if ((pReader_Rsp->result == 0) && (ReaderOpt == 1))
            {
                memset(u1RecvData, 0x00, sizeof(u1RecvData));
                RecvLength = pReader_Rsp->ndef_read.length;
                if (RecvLength != 0)
                {
                    memcpy(u1RecvData, pReader_Rsp->ndef_read.data, RecvLength);
                    parse_reulst = mtk_nfcho_handover_static_mgt(u1RecvData, RecvLength);
                    if (parse_reulst != 0)
                    {
                        Ret = 0xFF;
                    }
                    else
                    {
                        Ret = 0x00;
                    }
                }
                else
                {
                    Ret = 0xFF;
                }

                if (Ret ==  0xFF)
                {
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_RESULT, MTK_NFC_READ_RSP, 0, 1, &Ret);
                }
            }
            else if ( ReaderOpt == 2)
            {
                if (pReader_Rsp->result == 0)
                {
                    Ret = 0x00;
                }
                else
                {
                    Ret = 0xFF;
                }
                mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_WRITE, 0, 0, 1, &Ret );
            }
            else
            {
                mtk_nfcho_service_dbg_output("ReaderRsp,%d,Opt,%d", pReader_Rsp->result , ReaderOpt);
                Ret = 0xFF;
                Result = (-1);
                mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_RESULT, MTK_NFC_READ_RSP,0, 1, &Ret);
            }
        }
        break;


#else

        case MTK_NFC_CHIP_CONNECT_RSP:
        {
            s_mtk_nfc_dev_conn_rsp* pRsp = NULL;

            pRsp = (s_mtk_nfc_dev_conn_rsp*)((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));
            if (pRsp != NULL)
            {
                if (pRsp->result == 0)
                {
                    if (pRsp->status == 1)
                    {
                        g_nfc_status = 1;
                        Ret = 0x00;
                        mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_RESULT, MTK_NFC_ENTER_RSP,0, 1, &Ret );
                        mtk_nfcho_service_p2p_create_server(SerNameH, strlen((const char*)SerNameH) ,u1SAP);
                    }
                    else
                    {
                        g_nfc_status = 0;
                        mtk_nfcho_service_exit();
                    }
                }
                else
                {
                    mtk_nfcho_service_dbg_output("CHIP_RSP,%d,%d,%d", g_nfc_status,pRsp->result,  pRsp->status);
                    Ret = 0xFF;
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_RESULT, MTK_NFC_ENTER_RSP,0, 1, &Ret );
                }
                mtk_nfcho_service_dbg_output("CHIP_RSP,sw,0x%x,fw,0x%x,0x%x", pRsp->sw_ver, pRsp->fw_ver, pRsp->chipID);
            }
            else
            {
                Result = (-1);
            }
        }
        break;

        case MTK_NFC_DISCOVERY_RSP:
        {
            s_mtk_nfc_discovery_rsp* pRsp = NULL;

            pRsp = (s_mtk_nfc_discovery_rsp*)((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));

            if (pRsp != NULL)
            {
                if (pRsp->status == 1)
                {
                    if (pRsp->result == 0)
                    {
                        mtk_nfcho_service_dbg_output("Start,Poll");
                        Ret = 0;
                    }
                    else
                    {
                        mtk_nfcho_service_dbg_output("Start,Poll,E");
                        Ret = 0xFF;
                    }
                    mtk_nfcho_service_dbg_output("Ret,%d,status,%d,Opt,%d", Ret, g_nfc_status, g_nfc_opt);
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_RESULT, MTK_NFC_START_RSP,0, 1, &Ret );
                }
                else
                {
                    g_nfc_device = 0;
                    if (pRsp->result == 0)
                    {
                        mtk_nfcho_service_dbg_output("Stop,Poll");
                    }
                    else
                    {
                        mtk_nfcho_service_dbg_output("Stop,Poll,E");
                    }

                }
            }
            else
            {
                Result = (-1);
            }
        }
        break;

        case MTK_NFC_DISCOVERY_NTF:
        {
            s_mtk_nfc_discovery_detect_rsp* pRsp = NULL;
            pRsp = (s_mtk_nfc_discovery_detect_rsp*)((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));

            if (pRsp != NULL)
            {
                if (pRsp->detectiontype == DT_READ_MODE)
                {
                    g_nfc_device = 1;
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_FOUNDTAG, 0 ,0, 0, NULL );
                    mtk_nfcho_service_dbg_output("Tag,%d", pRsp->discovery_type.taginfo.cardinfotype );
                }
                else if (pRsp->detectiontype == DT_P2P_MODE)
                {
                    mtk_nfcho_service_dbg_output("Dev,%d,%d", pRsp->discovery_type.p2pinfo.p2p_role, g_nfc_device);
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_FOUNDDEVICE, 0, 0, 0, NULL );
                }
                else if (pRsp->detectiontype == DT_ENUM_END)
                {
                    mtk_nfcho_service_dbg_output("Bye Tag");
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_DEVICELEFT, 0 ,0, 0, NULL );
                }
            }
            else
            {
                Result = (-1);
            }
        }
        break;

        case MTK_NFC_P2P_LINK_NTF:
        {
            MTK_NFC_LLCP_LINK_STATUS* pRsp = NULL;

            pRsp = (MTK_NFC_LLCP_LINK_STATUS*)((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));
            if (pRsp != NULL)
            {
                if ((pRsp->ret == 0) && (pRsp->elink == MTK_NFC_LLCP_LINK_ACTIVATE))
                {
                    g_nfc_device = 2;
                }
                else if ((pRsp->ret == 0) && (pRsp->elink == MTK_NFC_LLCP_LINK_DEACTIVATE))
                {
                    UINT8 idx = 0;

                    mtk_nfcho_service_dbg_output("Bye Dev");
                    //Reset Variables
                    memset((UINT8*)RemoteRadioType, 0xFF, sizeof(UINT8)*MTK_NFCHO_RADIO_END);
                    g_nfc_device = 0;
                    if (p2p_working_handle != 0)
                    {
                        mtk_nfcho_snep_stop(p2p_working_handle);
                    }
                    if ( p2p_mode == MTK_NFC_LLCP_SERVICEROLE_CLIENT)
                    {
                        if (p2p_server_handle != 0)
                        {
                            mtk_nfcho_snep_stop(p2p_server_handle);
                            p2p_server_handle = 0;
                        }
                    }
                    for (idx = 0 ; idx < MTK_NFCHO_RADIO_END ; idx++)
                    {
                        if (SelectData[idx] != NULL)
                        {
                            free(SelectData[idx]);
                            SelectData[idx] = NULL;
                            SelectDataLength[idx] = 0;
                        }
                    }
                    TotalRadioSize = 0;
                    IsNegoHandover = 0;
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_DEVICELEFT, 0 , 0, 0, NULL );

                }
            }
            else
            {
                Result = (-1);
            }
        }
        break;

        case MTK_NFC_JNI_TAG_READ_RSP:
        {
            INT32 reulst = 0;
            s_mtk_nfc_jni_doread_rsp_t* pRsp = NULL;
            pRsp = (s_mtk_nfc_jni_doread_rsp_t*)((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));

            RecvLength = 0;
            memset(u1RecvData, 0x00, PAYLOAD_LENGTH);
            if (pRsp != NULL)
            {
                // For Debug
                //  mtk_nfcho_service_dbg_output("Read,Tag, (%d)=(%d)", pRsp->result  , pRsp->len );
                if((pRsp->result == 0) && (pRsp->len > 0))
                {
                    RecvLength = pRsp->len;
                    memcpy(u1RecvData, &pRsp->data, RecvLength);

#if 0 // For Debug
                    {
                        INT32 KKK = 0;

                        for ( KKK = 0;  KKK < RecvLength ; KKK++)
                        {
                            mtk_nfcho_service_dbg_output("Read,Tag, (%d)=(0x%x)", KKK , u1RecvData[KKK]);
                        }
                    }
#endif
                    reulst = mtk_nfcho_handover_static_mgt(u1RecvData, RecvLength);
                    if (reulst == (-1))
                    {
                        Ret = 0xFF;
                    }
                    else
                    {
                        Ret = 0x00;
                    }
                }
                else
                {
                    Ret = 0xFF;
                }
            }
            else
            {
                Ret = 0xFF;
            }

            if (Ret ==  0xFF)
            {
                Result = (-1);
                mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_RESULT, MTK_NFC_READ_RSP, 0,  1, &Ret);
            }
        }
        break;

        case MTK_NFC_JNI_TAG_WRITE_RSP:
        {
            s_mtk_nfc_jni_rsp_t* pRsp = NULL;
            pRsp = (s_mtk_nfc_jni_rsp_t*)((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));

            if (pRsp != NULL)
            {
                if (pRsp->result == 0)
                {
                    Ret = 0x00;
                }
                else
                {
                    Ret = 0xFF;
                }
            }
            else
            {
                Result = (-1);
                Ret = 0xFF;
            }
            mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_WRITE, 0, 0, 1, &Ret );
        }
        break;

#endif

        case MTK_NFC_P2P_CREATE_SERVER_RSP:
        {
            MTK_NFC_LLCP_RSP_SERVICE* pSerRsp = NULL;

            pBuf = ((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));
            pSerRsp = (MTK_NFC_LLCP_RSP_SERVICE*)pBuf;

            if (pSerRsp->ret == 0)
            {
                p2p_server_handle = pSerRsp->llcp_handle;
            }
            mtk_nfcho_service_dbg_output("SERVER_RSP,ret,%d,H,0x%x", pSerRsp->ret , p2p_server_handle);
        }
        break;

        case MTK_NFC_P2P_CREATE_SERVER_NTF:
        {
            MTK_NFC_LISTEN_SERVICE_NTF* pSerNtf = NULL;

            pBuf = ((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));
            pSerNtf = (MTK_NFC_LISTEN_SERVICE_NTF*)pBuf;

            if (pSerNtf->ret == 0)
            {
                p2p_working_handle = pSerNtf->incoming_handle;
                p2p_remote_miu = pSerNtf->remote_miu;
                mtk_nfcho_service_p2p_accpet_server(pSerNtf->incoming_handle);
            }
            mtk_nfcho_service_dbg_output("SERVER_NTF,ret,%d,WH,0x%x,SH,0x%x", pSerNtf->ret ,p2p_working_handle, pSerNtf->service_handle);
        }
        break;

        case MTK_NFC_P2P_ACCEPT_SERVER_RSP:
        {
            pBuf = ((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));

            if ( *(INT32*)pBuf == 0)
            {
                p2p_mode = MTK_NFC_LLCP_SERVICEROLE_SERVER;
                doneReading = 1;
                mtk_nfcho_snep_recv();
            }
            mtk_nfcho_service_dbg_output("ACCEPT_RSP,%d,m,%d", *(INT32*)pBuf,  p2p_mode);

        }
        break;

        case MTK_NFC_P2P_CREATE_CLIENT_RSP:
        {
            MTK_NFC_LLCP_RSP_SERVICE* pSerRsp = NULL;

            pBuf = ((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));
            pSerRsp = (MTK_NFC_LLCP_RSP_SERVICE*)pBuf;

            if (pSerRsp->ret == 0)
            {
                p2p_working_handle = pSerRsp->llcp_handle;
                mtk_nfcho_service_p2p_connect_client(pSerRsp->llcp_handle);
            }
            mtk_nfcho_service_dbg_output("CLIENT_RSP,%d,SH,0x%x",pSerRsp->ret, p2p_working_handle);
        }
        break;

        case MTK_NFC_P2P_CONNECT_CLIENT_RSP:
        {
            pBuf = ((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));

            mtk_nfcho_service_dbg_output("CONN_RSP,%d", *(INT32*)pBuf );
        }
        break;

        case MTK_NFC_P2P_CONNECTION_NTF:
        {
            MTK_NFC_CONNECTION_NTF* pConnNtf = NULL;
            UINT8 idx = 0;

            pBuf = ((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));
            pConnNtf = (MTK_NFC_CONNECTION_NTF*)pBuf;

            if (( pConnNtf->ret == 0) && (pConnNtf->errcode == 0))
            {
                p2p_mode = MTK_NFC_LLCP_SERVICEROLE_CLIENT;
                doneReading = 1;
                mtk_nfcho_handover_client_run(IsNegoHandover, TotalRadioSize, LocalRadioType, SelectData, SelectDataLength,  u1SAP);

                for (idx = 0 ; idx < TotalRadioSize ; idx++)
                {
                    if (SelectData[idx] != NULL)
                    {
                        free(SelectData[idx]);
                        SelectData[idx] = NULL;
                        SelectDataLength[idx] = 0;
                    }
                }
                TotalRadioSize = 0;
                IsNegoHandover = 0;
            }
            mtk_nfcho_service_dbg_output("CONN_NTF,%d,%d, mode,%d", pConnNtf->ret, pConnNtf->errcode ,p2p_mode);
        }
        break;

        case MTK_NFC_P2P_SOCKET_STATUS_NTF:
        {
            MTK_NFC_CONNECTION_NTF* pConnNtf = NULL;

            pBuf = ((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));
            pConnNtf = (MTK_NFC_CONNECTION_NTF*)pBuf;

            mtk_nfcho_service_dbg_output("SOCKET_NTF,%d,%d" ,pConnNtf->ret,  pConnNtf->errcode);
        }
        break;

        case MTK_NFC_P2P_SEND_DATA_RSP:
        {
            MTK_NFC_LLCP_RSP* pRsp = (MTK_NFC_LLCP_RSP*)((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));

            mtk_nfcho_service_dbg_output("SEND_RSP");
            if ((pRsp != NULL) &&( pRsp->ret == 0) )
            {
                if (p2p_mode == MTK_NFC_LLCP_SERVICEROLE_SERVER) //Server mode Send done
                {
                    mtk_nfcho_service_dbg_output("%d,%d", p2p_sending, p2p_other_length);
                    if ((p2p_sending == 1) && (p2p_other_length > 0))
                    {
                        mtk_nfcho_service_p2p_send_data(u1SendOtherData, p2p_other_length);
                        p2p_sending = 0;
                    }
                    else
                    {
                        mtk_nfcho_snep_stop(p2p_working_handle);
                    }
                }
                if (doneReading == 2)
                {
                    mtk_nfcho_service_dbg_output("F,pdu");
                    mtk_nfcho_snep_stop(p2p_working_handle);
                }
                else
                {
                    mtk_nfcho_service_dbg_output("Rec,pdu,%d", doneReading);
                    mtk_nfcho_snep_recv();
                }

            }
            else if ( pRsp->ret != 0)
            {
                mtk_nfcho_service_dbg_output("SendF");
                mtk_nfcho_snep_stop(p2p_working_handle); // Error case
            }
        }
        break;

        case MTK_NFC_P2P_RECV_DATA_RSP:
        {
            MTK_NFC_RECVDATA_RSP* pSerRsp = NULL;
            INT32 ret = 0;

            pBuf = ((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));
            pSerRsp = (MTK_NFC_RECVDATA_RSP*)pBuf;

            memset(u1RecvData, 0x00, PAYLOAD_LENGTH);
            RecvLength = 0;
            if ( pSerRsp->ret == 0)
            {
                RecvLength = pSerRsp->llcp_data_recv_buf.length;
                memcpy(u1RecvData, pSerRsp->llcp_data_recv_buf.buffer, RecvLength);

                //  mtk_nfcho_service_dbg_output("RecvL,%d", RecvLength );
                if (u1SAP == 0x04)
                {
                    mtk_nfcho_snep_parse(u1RecvData, (UINT16)RecvLength, p2p_mode, &doneReading, 1); //With SNEP
                }
                else
                {
                    mtk_nfcho_snep_parse(u1RecvData, (UINT16)RecvLength, p2p_mode, &doneReading, 0); //Without SNEP
                }

                mtk_nfcho_service_dbg_output("Reading,%d",doneReading);

                if ( doneReading == 0)// Try to Receive the next PDU
                {
                    ret = mtk_nfcho_service_p2p_receive_data();
                }
                else if ( doneReading == 1) // Receive Done
                {
                    if (p2p_mode == MTK_NFC_LLCP_SERVICEROLE_CLIENT) //Client mode Receive done
                    {
                        mtk_nfcho_service_dbg_output("ClRecvDone");
                        mtk_nfcho_snep_stop(p2p_working_handle);
                    }
                }
                else if ( doneReading == 2)// SNEP format Error
                {
                    mtk_nfcho_service_dbg_output("RecvErr");
                    mtk_nfcho_snep_stop(p2p_working_handle);
                }
                else if ( doneReading == 3) // Wait SNEP_REQUEST_CONTINUE
                {
                    mtk_nfcho_service_dbg_output("Reading,%d",doneReading);
                }
            }
            else if ( pSerRsp->ret != 0)
            {
                if (p2p_working_handle != 0)
                {
                    mtk_nfcho_snep_stop(p2p_working_handle);
                }
            }
            mtk_nfcho_service_dbg_output("Recv,ret,%d", pSerRsp->ret );
        }
        break;

        case MTK_NFC_P2P_DISC_RSP:
        {
            MTK_NFC_LLCP_RSP* pRsp = (MTK_NFC_LLCP_RSP*)((UINT8*)pMainMsg+sizeof(MTK_NFC_MSG_T));

            if (pRsp != NULL)
            {
                mtk_nfcho_service_dbg_output("DISC_RSP,%d", pRsp->ret);
            }
            p2p_working_handle = 0;
        }

        break;

#ifdef SUPPORT_EM_CMD

        case MTK_NFC_EXIT_RSP:
        {
            mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_KILLDONE, 0, 0, 0, NULL );
            mtk_nfcho_service_dbg_output("EXIT_RSP");
        }
        break;

#else
        case MTK_NFC_STOP_RSP:
        {
            mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_KILLDONE, 0, 0, 0, NULL );
            mtk_nfcho_service_dbg_output("STOP_RSP");
        }
        break;

#endif

        case MTK_NFC_FM_READ_DEP_TEST_RSP:
        {
            s_mtk_nfc_em_als_readerm_opt_rsp* pRspData = NULL;

            pRspData = (s_mtk_nfc_em_als_readerm_opt_rsp*)((UINT8*)pMainMsg + sizeof(MTK_NFC_MSG_T));

            if (pRspData != NULL)
            {
                Result = pRspData->result;
                mtk_nfcho_service_dbg_output("hwdep,%d", Result);
            }
            else
            {
                mtk_nfcho_service_dbg_output("read,null,%d", pMainMsg->type);
            }
            mtk_nfcho_service_exit();
        }
        break;

        case MTK_NFC_FM_SWP_TEST_RSP:
        {
            s_mtk_nfc_fm_swp_test_rsp* pRspData = NULL;

            pRspData = (s_mtk_nfc_fm_swp_test_rsp*)((UINT8*)pMainMsg + sizeof(MTK_NFC_MSG_T));

            if (pRspData != NULL)
            {
                Result = pRspData->result;
                mtk_nfcho_service_dbg_output("hwswp,%d", Result);
            }
            else
            {
                mtk_nfcho_service_dbg_output("null,%d", pMainMsg->type);
            }
            mtk_nfcho_service_exit();
        }
        break;

        case MTK_NFC_FM_CARD_MODE_TEST_RSP:
        {
            s_mtk_nfc_em_als_cardm_rsp* pRspData = NULL;

            pRspData = (s_mtk_nfc_em_als_cardm_rsp*)((UINT8*)pMainMsg + sizeof(MTK_NFC_MSG_T));

            if (pRspData != NULL)
            {
                Result = pRspData->result;
                mtk_nfcho_service_dbg_output("hwcard,%d", Result);
            }
            else
            {
                mtk_nfcho_service_dbg_output("null,%d", pMainMsg->type);
            }
            mtk_nfcho_service_exit();
        }
        break;

        case MTK_NFC_FM_VIRTUAL_CARD_RSP:
        {
            s_mtk_nfc_em_virtual_card_rsp* pRspData = NULL;

            pRspData = (s_mtk_nfc_em_virtual_card_rsp*)((UINT8*)pMainMsg + sizeof(MTK_NFC_MSG_T));

            if (pRspData != NULL)
            {
                Result = pRspData->result;
                mtk_nfcho_service_dbg_output("hwvcard,%d", Result);
            }
            else
            {
                mtk_nfcho_service_dbg_output("null,%d", pMainMsg->type);
            }
            mtk_nfcho_service_exit();
        }
        break;

        case MTK_NFC_GET_SELIST_RSP:
        {
            UINT8* pRspData = NULL;
            UINT16 Length = (UINT16)sizeof(grSeListInfor);

            pRspData = ((UINT8*)pMainMsg + sizeof(MTK_NFC_MSG_T));

            memcpy(&grSeListInfor, pRspData, Length);

            mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_RESULT, MTK_NFC_SELIST_RSP, 0, Length, (UINT8*)&grSeListInfor );

        }
        break;

        case MTK_NFC_SE_MODE_SET_RSP:
        {
            s_mtk_nfc_jni_se_set_mode_rsp_t* pRspData = NULL;

            pRspData = (s_mtk_nfc_jni_se_set_mode_rsp_t*)((UINT8*)pMainMsg + sizeof(MTK_NFC_MSG_T));

            Ret = (UINT8)pRspData->status ;

            mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_RESULT, MTK_NFC_SE_ENABLE_RSP, 0, 1, &Ret );
        }
        break;

        default:
        {
            mtk_nfcho_service_dbg_output("service,T,%d,L,%d",pMainMsg->type, pMainMsg->length);
            Result = (-1);
        }
        break;
    }
    return Result;
}


INT32 mtk_nfcho_service_p2p_create_server(UINT8 SerName[], UINT32 Length, UINT8 uSap)
{
    MTK_NFC_LLCP_CREATE_SERVICE* pSer = NULL;
    UINT32 u4Length = 0;

    mtk_nfcho_service_dbg_output("create,%s",SerName);

    u4Length = (sizeof(MTK_NFC_LLCP_CREATE_SERVICE) + Length);
    pSer = (MTK_NFC_LLCP_CREATE_SERVICE*)malloc(u4Length);

    if (pSer != NULL)
    {
        memset(pSer, 0x00, u4Length);

        pSer->buffer_options.miu = 300;
        pSer->buffer_options.rw = 3;

        pSer->connection_type = MTK_NFC_LLCP_CONNECTION_ORIENTED;
        pSer->sap = uSap;

        pSer->llcp_sn.length = Length;
        if((Length != 0) && (SerName != NULL))
        {
            memcpy (pSer->llcp_sn.buffer, SerName, Length);
        }
        mtk_nfcho_service_send_msg(MTK_NFC_P2P_CREATE_SERVER_REQ, (UINT8*)pSer, u4Length);
        free(pSer);
        pSer = NULL;
    }
    return 0;
}


INT32 mtk_nfcho_service_p2p_accpet_server(UINT32 Handle)
{
    MTK_NFC_LLCP_ACCEPT_SERVICE SerAccept;
    UINT32 u4Length = 0;

    //  mtk_nfcho_service_dbg_output("accpet,0x%x", Handle );

    u4Length = sizeof(MTK_NFC_LLCP_ACCEPT_SERVICE);

    memset(&SerAccept, 0x00, u4Length);

    SerAccept.buffer_options.miu = 300;
    SerAccept.buffer_options.rw = 3;
    SerAccept.incoming_handle = Handle; // p2p_incoming_handle

    mtk_nfcho_service_send_msg( MTK_NFC_P2P_ACCEPT_SERVER_REQ,  (UINT8*)&SerAccept , u4Length);
    return 0;
}


INT32 mtk_nfcho_service_p2p_create_client(void)
{
    MTK_NFC_LLCP_CREATE_SERVICE Ser;
    UINT32 u4Length = 0;

    u4Length = sizeof(MTK_NFC_LLCP_CREATE_SERVICE);
    memset(&Ser, 0x00, u4Length);

    Ser.buffer_options.miu = 300;
    Ser.buffer_options.rw = 3;
    Ser.connection_type = MTK_NFC_LLCP_CONNECTION_ORIENTED;
    Ser.sap = 0;
    Ser.llcp_sn.length = 0;

    mtk_nfcho_service_dbg_output("create_client");

    mtk_nfcho_service_send_msg(MTK_NFC_P2P_CREATE_CLIENT_REQ, (UINT8*)&Ser, u4Length);

    return 0;
}


INT32 mtk_nfcho_service_p2p_connect_client(UINT32 Handle)
{
    MTK_NFC_LLCP_CONN_SERVICE* pConnSer = NULL;
    UINT32 u4Length = 0;
    if ( strlen((const char*)SerNameH) != 0)
    {
        u4Length = (sizeof(MTK_NFC_LLCP_CONN_SERVICE) + strlen((const char*)SerNameH));
    }
    else
    {
        u4Length = (sizeof(MTK_NFC_LLCP_CONN_SERVICE));
    }

    pConnSer = (MTK_NFC_LLCP_CONN_SERVICE *)malloc(u4Length);
    if (pConnSer != NULL)
    {
        memset(pConnSer, 0x00, u4Length);

        pConnSer->client_handle = Handle;
        if ( strlen((const char*)SerNameH) != 0)
        {
            pConnSer->sap = u1SAP;
            if (u1SAP == 0x04)
            {
                pConnSer->llcp_sn.length = 0;
            }
            else
            {
                pConnSer->llcp_sn.length = strlen((const char*)SerNameH);
                memcpy (pConnSer->llcp_sn.buffer, SerNameH, strlen((const char*)SerNameH));
            }
        }
        else
        {
            pConnSer->sap = 0x04;
            pConnSer->llcp_sn.length = 0;
        }

        mtk_nfcho_service_dbg_output("connect,H,0x%x,sap,%d,L,%d",Handle, pConnSer->sap, pConnSer->llcp_sn.length);
        mtk_nfcho_service_send_msg(MTK_NFC_P2P_CONNECT_CLIENT_REQ,  (UINT8*)pConnSer, u4Length);
        free(pConnSer);
        pConnSer = NULL;
    }
    return 0;
}



INT32 mtk_nfcho_service_p2p_send_data(UINT8* DataBuf, UINT32 DataLth)
{
    MTK_NFC_LLCP_SEND_DATA* pSendData = NULL;
    UINT32 u4Length = 0;
    UINT32 SendDataLength = 0;


    if (DataLth > p2p_remote_miu)
    {
        memset(u1SendOtherData, 0x00, PAYLOAD_LENGTH);
        p2p_other_length = DataLth - p2p_remote_miu;
        SendDataLength = p2p_remote_miu;
        p2p_sending = 1;
        memcpy(u1SendOtherData, &DataBuf[p2p_remote_miu], p2p_other_length );
    }
    else
    {
        SendDataLength = DataLth;
    }

    mtk_nfcho_service_dbg_output("send,H,0x%x,L,%d", p2p_working_handle, SendDataLength);
    u4Length = (sizeof(MTK_NFC_LLCP_SEND_DATA) + SendDataLength);
    pSendData = (MTK_NFC_LLCP_SEND_DATA *)malloc(u4Length);
    if (pSendData != NULL)
    {
        memset(pSendData, 0x00, u4Length);
        pSendData->service_handle = p2p_working_handle;
        pSendData->sap = 0x20;
        pSendData->llcp_data_send_buf.length = SendDataLength;
        if (SendDataLength > 0)
        {
            memcpy(pSendData->llcp_data_send_buf.buffer, DataBuf, SendDataLength);

#if 0
            {
                UINT32 ii = 0;

                mtk_nfcho_service_dbg_output("SendBuf,[%d]", DataLth);
                for (ii=0 ; ii < DataLth ; ii++)
                {
                    mtk_nfcho_service_dbg_output("SendBuf,[%d],0x%x,%c", ii, pSendData->llcp_data_send_buf.buffer[ii], pSendData->llcp_data_send_buf.buffer[ii]);
                }
            }
#endif
        }

        mtk_nfcho_service_send_msg(MTK_NFC_P2P_SEND_DATA_REQ, (UINT8*)pSendData, u4Length);
        free(pSendData);
        pSendData = NULL;
    }
    return 0;
}


INT32 mtk_nfcho_service_p2p_receive_data(void)
{
    MTK_NFC_LLCP_SOKCET RecvReq;
    UINT32 u4Length = 0;

    //  mtk_nfcho_service_dbg_output("recv,0x%x", p2p_working_handle);
    if (p2p_working_handle != 0)
    {

        u4Length = sizeof(MTK_NFC_LLCP_SOKCET);

        memset(&RecvReq, 0x00, u4Length);
        RecvReq.llcp_service_handle = p2p_working_handle;
        mtk_nfcho_service_send_msg(MTK_NFC_P2P_RECV_DATA_REQ, (UINT8*)&RecvReq, u4Length);
        //mtk_nfcho_service_dbg_output("receive,wait.");
    }
    else
    {
        mtk_nfcho_service_dbg_output("receive,llcp doesn't work");
    }
    return 0;
}


INT32 mtk_nfcho_service_p2p_close_service(UINT32 handle)
{
    UINT32 u4Length = 0;
    UINT32 service_handle = 0;
    INT32 ret = 0;

    service_handle = handle;

    mtk_nfcho_service_dbg_output("close,0x%x", service_handle);
    if (service_handle != 0)
    {
        u4Length = sizeof(UINT32);
        mtk_nfcho_service_send_msg(MTK_NFC_P2P_DISC_REQ, (UINT8*)&service_handle, u4Length);
    }
    else
    {
        ret = (-1);
        mtk_nfcho_service_dbg_output("llcp not work");
    }
    return ret;
}


#ifdef SUPPORT_EM_CMD

INT32 mtk_nfcho_service_tag_write(UINT8* DataBuf, UINT32 DataLth)
{
    UINT32 u4Length = 0;
    s_mtk_nfc_em_als_readerm_opt_req EmReaderOPTReq;

    ReaderOpt = 2;

    u4Length = sizeof(s_mtk_nfc_em_als_readerm_opt_req);

    memset(&EmReaderOPTReq, 0x00, u4Length);


    EmReaderOPTReq.action = NFC_EM_OPT_ACT_WRITE_RAW;
    EmReaderOPTReq.ndef_write.ndef_data.EXT_Data.EXTLength = (UINT16)DataLth;
    if ((DataLth != 0) && (DataBuf != NULL))
    {
        memcpy(EmReaderOPTReq.ndef_write.ndef_data.EXT_Data.EXTData, DataBuf, DataLth);
    }
    mtk_nfcho_service_send_msg(MTK_NFC_EM_ALS_READER_MODE_OPT_REQ, (UINT8*)&EmReaderOPTReq, u4Length);


    return 0;
}

INT32 mtk_nfcho_service_tag_read(void)
{
    UINT32 u4Length = 0;
    s_mtk_nfc_em_als_readerm_opt_req EmReaderOPTReq;

    ReaderOpt = 1;

    u4Length = sizeof(s_mtk_nfc_em_als_readerm_opt_req);
    memset(&EmReaderOPTReq, 0x00, u4Length);
    EmReaderOPTReq.action = NFC_EM_OPT_ACT_READ;
    mtk_nfcho_service_send_msg(MTK_NFC_EM_ALS_READER_MODE_OPT_REQ, (UINT8*)&EmReaderOPTReq, u4Length);

    return 0;
}


INT32 mtk_nfcho_service_tag_format(void)
{
    UINT32 u4Length = 0;
    s_mtk_nfc_em_als_readerm_opt_req EmReaderOPTReq;

    ReaderOpt = 3;

    u4Length = sizeof(s_mtk_nfc_em_als_readerm_opt_req);
    memset(&EmReaderOPTReq, 0x00, u4Length);

    EmReaderOPTReq.action = NFC_EM_OPT_ACT_FORMAT;
    mtk_nfcho_service_send_msg(MTK_NFC_EM_ALS_READER_MODE_OPT_REQ, (UINT8*)&EmReaderOPTReq, u4Length);

    return 0;
}


#else
INT32 mtk_nfcho_service_tag_write(UINT8* DataBuf, UINT32 DataLth)
{
    UINT32 u4Length = 0;
    mtk_nfc_ndef_data_t* pReqData = NULL;

    if ((DataLth != 0) && (DataBuf != NULL))
    {
        ReaderOpt = 2;

        u4Length = sizeof(pReqData) + DataLth;
        pReqData = (mtk_nfc_ndef_data_t*)malloc(u4Length);
        memset(pReqData, 0x00, u4Length);
        pReqData->datalen = DataLth;
        memcpy(&pReqData->databuffer, DataBuf, DataLth);
        mtk_nfcho_service_send_msg(MTK_NFC_JNI_TAG_WRITE_REQ, (UINT8*)pReqData, u4Length);
        free(pReqData);
        pReqData = NULL;
    }
    return 0;
}

INT32 mtk_nfcho_service_tag_read(void)
{
    UINT32 u4Length = 0;
    mtk_nfc_ndef_data_t ReqData;

    ReaderOpt = 1;

    u4Length = sizeof(mtk_nfc_ndef_data_t);

    memset(&ReqData, 0x00, u4Length);

    ReqData.datalen = 0;
    mtk_nfcho_service_send_msg(MTK_NFC_JNI_TAG_READ_REQ, (UINT8*)&ReqData, u4Length);

    return 0;
}



INT32 mtk_nfcho_service_nfc_deactive(void)
{
    mtk_nfcho_service_nfc_op(0);
    return 0;
}

#endif


INT32 mtk_nfcho_service_send_msg(UINT32 type, UINT8* payload, UINT32 len)
{
    MTK_NFC_MSG_T *prmsg = NULL;
    UINT32 TotalLength = 0;


    mtk_nfcho_service_dbg_output("t,%d,l,%d", type, len);

    // malloc msg
    TotalLength = (sizeof(MTK_NFC_MSG_T) + len);

    prmsg = (MTK_NFC_MSG_T *)malloc(TotalLength);

    if (prmsg == NULL)
    {
        mtk_nfcho_service_dbg_output("malloc msg fail\n");
        return (-1);
    }
    else
    {
        // fill type & length
        memset(prmsg, 0x00, TotalLength);
        prmsg->type = type;
        prmsg->length = len;
        if ((len > 0)  && (payload != NULL))
        {
            memcpy(((UINT8 *)prmsg + sizeof(MTK_NFC_MSG_T)), payload, len);
        }

        // send msg to nfc daemon
        mtk_nfcho_service_send_handle((UINT8 *)prmsg, TotalLength);

    }

    // free msg
    if (prmsg != NULL)
    {
        free(prmsg);
    }
    return 0;
}



INT32 mtk_nfcho_handover_nego_mgt(UINT8 InData[], UINT32 InLength, UINT8 OutData[], UINT32* pOutLength)
{
    INT32 ret = 0, idx = 0 , RadioIdx = 0;
    UINT8** OData = NULL;
    UINT16 OLength[MTK_NFCHO_RADIO_END];
    UINT8 RemotePower[MTK_NFCHO_RADIO_END];
    UINT8 NumRadio = 0;
    UINT16 ReceivedRandom = 0;
    MTK_NFC_APP_DATA_T Data[MTK_NFCHO_RADIO_END];

    mtk_nfcho_service_dbg_output("nego_mgt,L,%d", InLength);
    OData = (UINT8**)malloc((sizeof(UINT8*) * MTK_NFCHO_RADIO_END));
    for ( idx = 0; idx < MTK_NFCHO_RADIO_END; idx++ )
    {
        OData[idx] = (UINT8*)malloc((sizeof(UINT8) * PAYLOAD_LENGTH));
        memset(OData[idx] , 0x00, PAYLOAD_LENGTH);
    }

    memset(OLength, 0x00, (sizeof(UINT16)*MTK_NFCHO_RADIO_END));
    // Check handover
    memset(&RemoteRadioType[0], 0xFF, (sizeof(UINT8)*MTK_NFCHO_RADIO_END));
    memset(&RemoteCarrierData[0], 0xFF, (sizeof(UINT8)*MTK_NFCHO_RADIO_END));
    memset(&RemotePower[0], 0x00, (sizeof(UINT8)*MTK_NFCHO_RADIO_END));


    if (mtk_nfcho_handover_request_parse( InData, InLength, MTK_NFCHO_RADIO_END, OData, OLength, &ReceivedRandom,  &NumRadio, RemotePower, RemoteRadioType, RemoteCarrierData) == 0)
    {
        mtk_nfcho_service_dbg_output("Requester,%d,0x%x,0x%x", NumRadio, ReceivedRandom, RequestRandom);

        if ( ((RequestRandom != 0) && (ReceivedRandom > RequestRandom)) || (RequestRandom == 0) )
        {
            mtk_nfcho_service_dbg_output("I'm Selector,%d", NumRadio );
            for (RadioIdx = 0 ; RadioIdx < NumRadio ; RadioIdx++)
            {
                if (RemoteRadioType[RadioIdx] == MTK_NFCHO_RADIO_WIFIWPS)
                {
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WPSHOR_TYPE,0,  OLength[RadioIdx], OData[RadioIdx]  );
                }
                else if (RemoteRadioType[RadioIdx] == MTK_NFCHO_RADIO_WIFIDIRECT)
                {
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WIFIDIRECTHOR_TYPE,0,  OLength[RadioIdx], OData[RadioIdx]  );
                }
                else if (RemoteRadioType[RadioIdx] == MTK_NFCHO_RADIO_WIFIFC)
                {
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WIFIFCR_TYPE ,0,  OLength[RadioIdx], OData[RadioIdx] );
                }
                else  if (RemoteRadioType[RadioIdx] == MTK_NFCHO_RADIO_BT)
                {
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_BTR_TYPE, 0,  OLength[RadioIdx] , OData[RadioIdx] );
                    // find BT
                }
                else
                {
                    mtk_nfcho_service_dbg_output("Not Support,%d", RemoteRadioType[RadioIdx]);
                }
            }
            // Build Select recoed
            if ( (TotalRadioSize != 0) && (IsNegoHandover == 1))
            {
                memset(&Data, 0x00, sizeof(MTK_NFC_APP_DATA_T)*MTK_NFCHO_RADIO_END);
                for (idx = 0 ; idx < TotalRadioSize ; idx++)
                {
                    Data[idx].data = SelectData[idx];
                    Data[idx].lth = (UINT16)SelectDataLength[idx];
                }

                mtk_nfcho_handover_select_build( TotalRadioSize,LocalPower, LocalRadioType, Data, pOutLength, OutData);
                mtk_nfcho_service_dbg_output("nego_mgt,Built,%d", (*pOutLength));

                for (idx = 0 ; idx < TotalRadioSize ; idx++)
                {
                    free(SelectData[idx]);
                    SelectData[idx] = NULL;
                    SelectDataLength[idx] = 0;
                }
                TotalRadioSize = 0;
                IsNegoHandover = 0;

                ret = 0;
            }
            else
            {
                mtk_nfcho_service_dbg_output("WithoutSelectData,%d,%d", TotalRadioSize, IsNegoHandover );
                ret = -1;
            }
            RequestRandom = 0;
        }
        else
        {
            mtk_nfcho_service_dbg_output("Requester" );
            ret = 0;
        }
    }
    else if(mtk_nfcho_handover_select_parse( InData, InLength, MTK_NFCHO_RADIO_END, OData, OLength, &NumRadio, RemotePower, RemoteRadioType, RemoteCarrierData) == 0)
    {
        if (RequestRandom != 0)
        {
            mtk_nfcho_service_dbg_output("Requester,%d", NumRadio);
            for (RadioIdx = 0 ; RadioIdx < NumRadio ; RadioIdx++)
            {
                if (RemoteRadioType[RadioIdx] == MTK_NFCHO_RADIO_WIFIWPS)
                {
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WPSHOS_TYPE , 0, OLength[RadioIdx], OData[RadioIdx] );
                }
                else if (RemoteRadioType[RadioIdx] == MTK_NFCHO_RADIO_WIFIDIRECT)
                {
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WIFIDIRECTHOS_TYPE, 0, OLength[RadioIdx], OData[RadioIdx] );
                }
                else if (RemoteRadioType[RadioIdx] == MTK_NFCHO_RADIO_WIFIFC)
                {
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WIFIFCS_TYPE, 0, OLength[RadioIdx], OData[RadioIdx] );
                }
                else if (RemoteRadioType[RadioIdx] == MTK_NFCHO_RADIO_BT)
                {
                    mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_BTS_TYPE, 0, OLength[RadioIdx], OData[RadioIdx]  );
                    // find BT
                }
                else
                {
                    mtk_nfcho_service_dbg_output("Not Support,%d", RemoteRadioType[RadioIdx]);
                }
            }
            ret = 0; //Selector
            RequestRandom = 0;
        }
        else
        {
            mtk_nfcho_service_dbg_output("Never Send Request" );
            ret = -1;
        }
    }
    else    // Others General NDEF
    {
        ret = mtk_nfcho_handover_token_parse(InData, InLength);
    }
    if (OData != NULL)
    {
        for ( idx = 0; idx < MTK_NFCHO_RADIO_END; idx++ )
        {
            if (OData[idx] != NULL)
            {
                free(OData[idx]);
                OData[idx] = NULL;
            }
        }
        free(OData);
        OData = NULL;
    }
    mtk_nfcho_service_dbg_output("ret,%d",ret );
    return ret;
}

INT32 mtk_nfcho_handover_static_mgt( UINT8 u1Data[], UINT32 Length)
{
    INT32 ret = 0, RadioIdx = 0;
    UINT8 TokenData[PAYLOAD_LENGTH];
    UINT8** pConfigData = NULL;
    UINT16 TokenLength[MTK_NFCHO_RADIO_END];
    UINT8 u1RadioNum = 0, u1TestTemp = 0;

    memset(RemoteRadioType, 0xFF, sizeof(MTK_NFCHO_RADIO_END));

    if (( u1Data != NULL) && (Length != 0))
    {
        mtk_nfcho_service_dbg_output("static_mgt,%d", Length );
        memset(TokenData, 0x00, PAYLOAD_LENGTH);
        memset(TokenLength, 0x00, sizeof(UINT16)*MTK_NFCHO_RADIO_END);

        pConfigData = (UINT8**)malloc(sizeof(UINT8*)*MTK_NFCHO_RADIO_END);

        if (pConfigData != NULL)
        {
            for ( RadioIdx = 0; RadioIdx < MTK_NFCHO_RADIO_END ; RadioIdx++)
            {
                pConfigData[RadioIdx] = (UINT8*)malloc(sizeof(UINT8)*PAYLOAD_LENGTH);
                memset(pConfigData[RadioIdx], 0x00, (sizeof(UINT8)*PAYLOAD_LENGTH));
            }
            memset(TokenLength, 0x00, sizeof(UINT16)*MTK_NFCHO_RADIO_END);
            ret = mtk_nfcho_handover_select_parse(u1Data, Length, MTK_NFCHO_RADIO_END,  pConfigData, TokenLength, &u1RadioNum, &u1TestTemp, (UINT8*)RemoteRadioType, &u1TestTemp);

            if (ret < 0)
            {
#if 0
                {
                    // For verify request format
                    UINT16 u2TestTemp = 0;
                    memset(TokenLength, 0x00, sizeof(UINT16)*MTK_NFCHO_RADIO_END);
                    ret = mtk_nfcho_handover_request_parse(u1Data, Length, MTK_NFCHO_RADIO_END, pConfigData, TokenLength,  &u2TestTemp, &u1RadioNum, &u1TestTemp, (UINT8*)RemoteRadioType, &u1TestTemp);// For Test
                    mtk_nfcho_service_dbg_output("static_mgt,NotSupport,%d,%d", TokenLength[0], RemoteRadioType[0]);
                }
#endif
                ret = mtk_nfcho_handover_token_parse(u1Data, Length);
            }
            else
            {
                for ( RadioIdx = 0; RadioIdx < u1RadioNum ; RadioIdx++)
                {
                    if (RemoteRadioType[RadioIdx] == MTK_NFCHO_RADIO_WIFIFC)
                    {
                        mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WIFIFCS_TYPE, 0,  TokenLength[RadioIdx] , pConfigData[RadioIdx]);
                    }
                    else  if (RemoteRadioType[RadioIdx] == MTK_NFCHO_RADIO_WIFIWPS)
                    {
                        mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WPSHOS_TYPE, 0,  TokenLength[RadioIdx] , pConfigData[RadioIdx]);
                    }
                    else if (RemoteRadioType[RadioIdx] == MTK_NFCHO_RADIO_WIFIDIRECT)
                    {
                        mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WIFIDIRECTHOS_TYPE, 0,  TokenLength[RadioIdx], pConfigData[RadioIdx] );
                    }
                    else if (RemoteRadioType[RadioIdx] == MTK_NFCHO_RADIO_BT)
                    {
                        mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_BTS_TYPE, 0,  TokenLength[RadioIdx], pConfigData[RadioIdx] );
                    }
                }
            }
            for ( RadioIdx = 0; RadioIdx < MTK_NFCHO_RADIO_END; RadioIdx++ )
            {
                if (pConfigData[RadioIdx] != NULL)
                {
                    free(pConfigData[RadioIdx]);
                    pConfigData[RadioIdx] = NULL;
                }
            }
            free(pConfigData);
            pConfigData = NULL;
        }
        else
        {
            ret = -1;
        }
    }
    else
    {
        ret = -1;
    }
    mtk_nfcho_service_dbg_output("ret,%d",ret );

    return ret;

}

INT32 mtk_nfcho_handover_client_run(UINT8 IsRadio, UINT8 NumRadio, UINT8 RadioList[], UINT8* DataBuf[], UINT32 DataLth[], UINT8 Sap)
{
    INT32 ret = 0 , idx = 0;
    UINT32 OutLength = 0;
    UINT8 OutData[PAYLOAD_LENGTH];
    MTK_NFC_APP_DATA_T Data[MTK_NFCHO_RADIO_END];


    memset(OutData, 0x00, PAYLOAD_LENGTH);
    memset(Data, 0x00, sizeof(MTK_NFC_APP_DATA_T)*MTK_NFCHO_RADIO_END);

    mtk_nfcho_service_dbg_output("handover_client,%d,%d",NumRadio, IsRadio );

    if ((DataLth != NULL) && (DataBuf != NULL) && (NumRadio != 0))
    {
        if (IsRadio == 1)
        {
            if (NumRadio < MTK_NFCHO_RADIO_END)
            {
                for (idx = 0; idx < NumRadio ; idx++)
                {
                    Data[idx].lth = (UINT16)DataLth[idx];
                    Data[idx].data = DataBuf[idx];
                }
                mtk_nfcho_handover_request_build(NumRadio,LocalPower, RadioList, Data, &OutLength, OutData, &RequestRandom);

                if (Sap == 0x04)
                {
                    mtk_nfcho_snep_client_get( OutData, OutLength);
                }
                else
                {
                    mtk_nfcho_service_p2p_send_data(OutData, OutLength);
                    mtk_nfcho_service_p2p_receive_data();
                }
            }
            else
            {
                ret = (-2);
                mtk_nfcho_service_dbg_output("para,-2");
            }
        }
        else if (IsRadio == 0)
        {
            idx = 0;
            if (Sap == 0x04)
            {
                mtk_nfcho_snep_client_put(DataBuf[idx], DataLth[idx]);
            }
            else
            {
                mtk_nfcho_service_p2p_send_data(DataBuf[idx], DataLth[idx]);
                mtk_nfcho_service_p2p_receive_data();
            }
        }
        else
        {
            ret = (-1);
            mtk_nfcho_service_dbg_output("para,-1,%d", IsRadio);
        }
    }
    else
    {
        ret = (-3);
        mtk_nfcho_service_dbg_output("para,-3");
    }
    return ret;
}



