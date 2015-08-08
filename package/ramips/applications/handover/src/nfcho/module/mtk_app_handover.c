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
 *  nfcd_snep.c
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


#define HO_VERSION  0x12

#define SNEP_HEADER_LENGTH  6
#define SNEP_VERSION_MAJOR  0x01
#define SNEP_VERSION_MINOR  0x00
#define SNEP_VERSION  0x10

#define SNEP_REQUEST_CONTINUE 0x00
#define SNEP_REQUEST_GET 0x01
#define SNEP_REQUEST_PUT 0x02
#define SNEP_REQUEST_REJECT 0x07

#define SNEP_RESPONSE_SUCCESS 0x81
#define SNEP_RESPONSE_CONTINUE 0x80
#define SNEP_RESPONSE_NOT_FOUND 0xC0
#define SNEP_RESPONSE_REJECT 0xFF
#define SNEP_RESPONSE_BAD_REQUEST 0xC2
#define SNEP_RESPONSE_UNSUPPORTED_VERSION 0xE1
#define SNEP_RESPONSE_DATA        0xFE


UINT8 SnepReadData[DEFAULT_ACCEPTABLE_LENGTH];  // Read/Received Data Parameters
UINT32 SnepReadLength = 0;
UINT16 SnepReadIndex = 0;

UINT32 SnepWriteLength = 0;  // Write/Sent Data Parameters
UINT8 SnepWriteData[DEFAULT_ACCEPTABLE_LENGTH];
UINT32 SnepMaxLength = 0;


const char* g_RadioType[MTK_NFCHO_RADIO_END] = {"application/vnd.wfa.wsc" ,
                                                "application/vnd.bluetooth.ep.oob",
                                                "application/vnd.wfa.p2p",
                                                "application/vnd.wfa.wsc"
                                               };

UINT8 g_RadioLth[MTK_NFCHO_RADIO_END] = { MTK_NFC_WIFIWPS_TYPE_LENGTH, MTK_NFC_BT_TYPE_LENGTH, MTK_NFC_WIFIDIRECT_TYPE_LENGTH, MTK_NFC_WIFIFC_TYPE_LENGTH};


INT32 mtk_nfcho_handover_radio_payload_parse(UINT8 RadioType, UINT8 Data[], UINT16 Length)
{
    INT32 ret = MTK_NFCHO_RADIODATA_DEFAULT;  // 1 : OOB device password , 2 : configuration , 3 : beam+,  0: default
    UINT16 TokenLength = 0, Index = 0, Type = 0, L = 0;
    UINT8 StarAddr = 0;

    TokenLength = Length;

//   mtk_nfcho_service_dbg_output("pay_parse,%d", RadioType);

    if (RadioType == MTK_NFCHO_RADIO_WIFIWPS)
    {
        for (StarAddr = 0 ; StarAddr < 2 ; StarAddr=StarAddr+2) // 1 : OOB device password , 2 : configuration's start address is 2, 3 : beam+'s start address is 0
        {
            Index = StarAddr;
            //        mtk_nfcho_service_dbg_output("Start,%d", StarAddr);
            while (Index < TokenLength)
            {
                memcpy(&Type, (UINT16*)&Data[Index], 2);
                Type = SWAP16(Type);
                //        mtk_nfcho_service_dbg_output("Idx,%d,Type,0x%x", Index , Type);
                if (Type == 0x102C)// OOB Device ID
                {
                    ret = MTK_NFCHO_RADIODATA_PASSWD;
                    break;
                }
                else if ((Type == 0x5001)  || (Type == 0x5002) || (Type == 0x5003) || (Type == 0x5004) || (Type == 0x5005) || (Type == 0x5006))// OOB Device ID
                {
                    ret = MTK_NFCHO_RADIODATA_BEAMPLUS;
                    break;
                }
                else if (Type == 0x100E)
                {
                    ret = MTK_NFCHO_RADIODATA_CONFIG;
                    break;
                }
                Index = Index + 2; // Type
                memcpy(&L, (UINT16*)&Data[Index], 2);
                L = SWAP16(L);
                Index = Index + 2; //Length
                Index = Index + L; //Vaule
                //    mtk_nfcho_service_dbg_output("L,%d", L);
            }
            if (ret > 0)
            {
                break;
            }
        }
    }
    mtk_nfcho_service_dbg_output("pay_parse,%d,(%d),S,%d,T,0x%x,I,%d,%d",RadioType, ret, StarAddr, Type, Index, TokenLength);

    return  ret;
}



INT32 mtk_nfcho_handover_select_parse(UINT8 Data[], UINT32 Length, UINT8 MaxNum,
                                      UINT8** OutData, UINT16* OutLength,
                                      UINT8* pNum, UINT8 Power[], UINT8 Radio[],
                                      UINT8 CarrierID[])
{
    INT32 ret = 0, DataType = MTK_NFCHO_RADIODATA_DEFAULT;
    mtk_nfcho_ndefrecord_t NDEFData;
    mtk_nfcho_ndefrecord_t AcData;
    INT32 CheckLength = 0 , AcLength = 0, AcIdx = 0 ;
    UINT8 HoVer = 0, Flags = 0, RadioIdx = 0, idx = 0 ;

    mtk_nfcho_service_dbg_output("select_pars,%d,%d", Length, MaxNum);

    if ((Data != NULL) && (Length != 0) && (OutData != NULL) && (OutLength != NULL) &&
        (pNum != NULL) && (Power != NULL) && (Radio != NULL) && (CarrierID != NULL))
    {
#if 0
        for (idx = 0; idx < Length ; idx++)
        {
            mtk_nfcho_service_dbg_output("[%d]=0x%x, %c", idx, Data[idx], Data[idx]);
        }
#endif

        memset(&NDEFData, 0x00, sizeof(mtk_nfcho_ndefrecord_t));
        CheckLength = 0;

        mtk_nfchod_NdefRecord_Parse(&NDEFData, Data);
        Flags = NDEFData.Flags;

        //   mtk_nfcho_service_dbg_output("(0),0x%x, %s, %d", Flags, NDEFData.Type, NDEFData.TypeLength);
        if (strncmp((const char*)NDEFData.Type, "Hs", NDEFData.TypeLength) == 0)
        {
            CheckLength =  CheckLength + NDEFData.TypeLength + NDEFData.PayloadLength + 3;
            mtk_nfcho_service_dbg_output("Hs,ChkL,%d,PayL,%d", CheckLength, NDEFData.PayloadLength);

            AcLength = NDEFData.PayloadLength;

            HoVer = NDEFData.PayloadData[0];
            AcIdx = 1;
            RadioIdx = 0;

            if (HoVer == HO_VERSION)
            {
                while ((AcIdx < AcLength) && (RadioIdx < MaxNum)) //Max radio numner is 4
                {
                    memset(&AcData, 0x00, sizeof(mtk_nfcho_ndefrecord_t));
                    mtk_nfchod_NdefRecord_Parse(&AcData,  &NDEFData.PayloadData[AcIdx]);

                    AcIdx = AcIdx+AcData.TypeLength+AcData.PayloadLength+3;
                    // Check  AC recode
                    if ( strncmp((const char*)AcData.Type, "ac", AcData.TypeLength) == 0)
                    {
                        Radio[RadioIdx] = AcData.PayloadData[0];
                        CarrierID[RadioIdx] = AcData.PayloadData[2];
                        mtk_nfcho_service_dbg_output("ac,(%d),0x%x,0x%x",RadioIdx,  Radio[RadioIdx] , CarrierID[RadioIdx]);
                    }
                    RadioIdx++;
                    // mtk_nfcho_service_dbg_output("(%d),(%d), 0x%x, %s, %d, %d",RadioIdx,AcIdx, AcData.Flags, AcData.Type, AcData.TypeLength, AcData.PayloadLength);
                }

                if ((RadioIdx > 0) && (RadioIdx <= MaxNum))
                {
                    (*pNum) = RadioIdx;
                    mtk_nfcho_service_dbg_output("N,%d", (*pNum));

                    for (RadioIdx = 0; RadioIdx < (*pNum) ; RadioIdx++)
                    {
                        memset(&NDEFData, 0x00, sizeof(mtk_nfcho_ndefrecord_t));
                        mtk_nfchod_NdefRecord_Parse(&NDEFData,  &Data[CheckLength]);
                        //           mtk_nfcho_service_dbg_output("(%d), 0x%x, %s, %d, %d",RadioIdx, NDEFData.Flags, NDEFData.Type, NDEFData.TypeLength, NDEFData.PayloadLength);

                        for (idx = 0; idx < MTK_NFCHO_RADIO_END ; idx++)
                        {
                            if ( memcmp(NDEFData.Type, g_RadioType[idx], g_RadioLth[idx]) == 0)
                            {
                                Radio[RadioIdx] = idx;
                                //   mtk_nfcho_service_dbg_output("(%d),Found,Radio,%d", RadioIdx, Radio[RadioIdx]);
                                break;
                            }
                        }
                        if (idx >= MTK_NFCHO_RADIO_END)
                        {
                            Radio[RadioIdx] = 0xFF;
                        }

                        if (((OutLength+RadioIdx) != NULL) && (NDEFData.PayloadLength != 0) && (OutData+RadioIdx) != NULL)
                        {
                            memcpy( *(OutData+RadioIdx), NDEFData.PayloadData, NDEFData.PayloadLength);
                            *(OutLength+RadioIdx) = (UINT16)NDEFData.PayloadLength;
                            //mtk_nfcho_service_dbg_output("Length, %d, %d", NDEFData.PayloadLength, *(OutLength+RadioIdx));
                        }

#if 0
                        for (idx = 0; idx <  *(OutLength+RadioIdx) ; idx++)
                        {
                            mtk_nfcho_service_dbg_output("Config [%d]=0x%x, 0x%x", idx, OutData[RadioIdx][idx], NDEFData.PayloadData[idx]);
                        }
#endif

                        DataType = mtk_nfcho_handover_radio_payload_parse(Radio[RadioIdx],  *(OutData+RadioIdx), *(OutLength+RadioIdx));
                        if (DataType == MTK_NFCHO_RADIODATA_BEAMPLUS)
                        {
                            Radio[RadioIdx] = MTK_NFCHO_RADIO_WIFIFC;
                        }
                        mtk_nfcho_service_dbg_output("Ra,(%d),%d,%d",RadioIdx, Radio[RadioIdx],  *(OutLength+RadioIdx));
                    }
                }
                else
                {
                    mtk_nfcho_service_dbg_output("para,err,1");
                    ret = -1;
                    //error
                }

            }
            else
            {
                mtk_nfcho_service_dbg_output("para,err,2");
                ret = -2;
                //err ver
            }
        }
        else
        {
            mtk_nfcho_service_dbg_output("para,err,3");
            ret = -3;
            //error
        }
    }
    else
    {
        mtk_nfcho_service_dbg_output("para,err,4");
        ret = -4;
        //error
    }
    return ret;
}



INT32 mtk_nfcho_handover_request_parse(UINT8 Data[], UINT32 Length, UINT8 MaxNum,
                                       UINT8** OutData, UINT16* OutLength,
                                       UINT16* pReceivedRandom, UINT8* pNum, UINT8 Power[], UINT8 Radio[],
                                       UINT8 CarrierID[] )
{
    INT32 ret = 0, DataType = MTK_NFCHO_RADIODATA_DEFAULT;
    mtk_nfcho_ndefrecord_t NDEFData;
    mtk_nfcho_ndefrecord_t AcData;
    INT32  CheckLength = 0 , AcLength = 0, AcIdx = 0 ;
    UINT8 HoVer = 0, Flags = 0, RadioIdx = 0 , idx = 0;
    UINT8 u1HR[] = {'H', 'r'};
    UINT8 u1cr[] = {'c', 'r'};
    UINT8 u1ac[] = {'a', 'c'};

    mtk_nfcho_service_dbg_output("request_pars,%d", Length);

    if ((Data != NULL) && (Length != 0) && (OutData != NULL) && (OutLength != NULL) && (pReceivedRandom != NULL) &&
        (pNum != NULL) && (Power != NULL) && (Radio != NULL) && (CarrierID != NULL))
    {
#if 0
        for (idx = 0; idx < Length ; idx++)
        {
            mtk_nfcho_service_dbg_output("[%d]=0x%x, %c", idx, Data[idx], Data[idx]);
        }
#endif

        memset(&NDEFData, 0x00, sizeof(mtk_nfcho_ndefrecord_t));
        CheckLength = 0;

        mtk_nfchod_NdefRecord_Parse(&NDEFData, Data);
        Flags = NDEFData.Flags;

        //      mtk_nfcho_service_dbg_output("(0), 0x%x, 0x%x, 0x%x, %d", Flags, NDEFData.Type[0] , NDEFData.Type[1], NDEFData.TypeLength);

        if (memcmp(NDEFData.Type, u1HR, NDEFData.TypeLength) == 0)
        {
            CheckLength =  CheckLength + NDEFData.TypeLength + NDEFData.PayloadLength + 3;
            mtk_nfcho_service_dbg_output("Hr,ChkL,%d,PayL,%d", CheckLength, NDEFData.PayloadLength);

            AcLength = NDEFData.PayloadLength;

            HoVer = NDEFData.PayloadData[0];
            AcIdx = 1;
            RadioIdx = 0;

            if (HoVer == HO_VERSION)
            {
                while ((AcIdx < AcLength) && (RadioIdx < (MaxNum + 1))) //Max radio numner is 8
                {
                    memset(&AcData, 0x00, sizeof(mtk_nfcho_ndefrecord_t));
                    mtk_nfchod_NdefRecord_Parse(&AcData,  &NDEFData.PayloadData[AcIdx]);
                    // Check AR & AC recode
                    AcIdx = AcIdx+AcData.TypeLength+AcData.PayloadLength+3;
                    if (RadioIdx == 0)
                    {
                        if (memcmp(AcData.Type, u1cr, AcData.TypeLength) == 0)
                        {
                            mtk_nfcho_service_dbg_output("cr");
                            (*pReceivedRandom) = *(UINT16*)&AcData.PayloadData[0];
                        }
                        else
                        {
                            //error
                            break;
                        }
                    }
                    else
                    {
                        if ( memcmp(AcData.Type, u1ac, AcData.TypeLength) == 0)
                        {
                            Radio[RadioIdx-1] = AcData.PayloadData[0];
                            CarrierID[RadioIdx-1] = AcData.PayloadData[2];
                            mtk_nfcho_service_dbg_output("ac,(%d),0x%x,0x%x",RadioIdx,  Radio[RadioIdx-1] , CarrierID[RadioIdx-1]);
                        }
                    }
                    RadioIdx++;
                    // mtk_nfcho_service_dbg_output("(0),(%d),(%d), 0x%x, %s, %d, %d",RadioIdx,AcIdx, AcData.Flags, AcData.Type, AcData.TypeLength, AcData.PayloadLength);
                }

                if ((RadioIdx > 1) && (RadioIdx <= (MaxNum + 1)))
                {
                    (*pNum) = (RadioIdx-1);
                    mtk_nfcho_service_dbg_output("N,%d", (*pNum));

                    for (RadioIdx = 0; RadioIdx < (*pNum) ; RadioIdx++)
                    {
                        memset(&NDEFData, 0x00, sizeof(mtk_nfcho_ndefrecord_t));
                        mtk_nfchod_NdefRecord_Parse(&NDEFData,  &Data[CheckLength]);
//                           mtk_nfcho_service_dbg_output("(%d), 0x%x, %s, %d, %d",RadioIdx, NDEFData.Flags, NDEFData.Type, NDEFData.TypeLength, NDEFData.PayloadLength);

                        for (idx = 0; idx < MTK_NFCHO_RADIO_END ; idx++)
                        {
                            if ( memcmp(NDEFData.Type, g_RadioType[idx], g_RadioLth[idx]) == 0)
                            {
                                Radio[RadioIdx] = idx;
                                //               mtk_nfcho_service_dbg_output("(%d),Found,Radio,%d", RadioIdx, Radio[RadioIdx]);
                                break;
                            }
                        }
                        if (idx >= MTK_NFCHO_RADIO_END)
                        {
                            Radio[RadioIdx] = 0xFF;
                        }

                        if (((OutLength+RadioIdx) != NULL) && (NDEFData.PayloadLength != 0) && (OutData+RadioIdx) != NULL)
                        {
                            memcpy( *(OutData+RadioIdx), NDEFData.PayloadData, NDEFData.PayloadLength);
                            *(OutLength+RadioIdx) = (UINT16)NDEFData.PayloadLength;
                            //      mtk_nfcho_service_dbg_output("Length, %d", *(OutLength+RadioIdx));
                        }
#if 0
                        for (idx = 0; idx <  *(OutLength+RadioIdx) ; idx++)
                        {
                            mtk_nfcho_service_dbg_output("Config [%d]=0x%x", idx, OutData[RadioIdx][idx]);
                        }
#endif
                        DataType = mtk_nfcho_handover_radio_payload_parse(Radio[RadioIdx],  *(OutData+RadioIdx), *(OutLength+RadioIdx));
                        if (DataType == MTK_NFCHO_RADIODATA_BEAMPLUS)
                        {
                            Radio[RadioIdx] = MTK_NFCHO_RADIO_WIFIFC;
                        }
                        mtk_nfcho_service_dbg_output("Ra,(%d),%d,%d",RadioIdx, Radio[RadioIdx],  *(OutLength+RadioIdx));

                    }
                }
                else
                {
                    mtk_nfcho_service_dbg_output("para,err,1");
                    ret = -1;
                    //error
                }

            }
            else
            {
                mtk_nfcho_service_dbg_output("para,err,2");
                ret = -2;
                //err ver
            }
        }
        else
        {
            mtk_nfcho_service_dbg_output("para,err,3");
            ret = -3;
            //error
        }
    }
    else
    {
        mtk_nfcho_service_dbg_output("para,err,4");
        ret = -4;
        //error
    }
    return ret;
}

INT32 mtk_nfcho_handover_token_parse(UINT8 Data[], UINT32 Length)
{
    INT32 ret = 0, DataIdx = 0, DataType = 0;
    UINT8 idx = 0, RadioIdx = 0;
    UINT8 TokenData[PAYLOAD_LENGTH];
    UINT16  TokenLength = 0;
    UINT8 RadioType = 0;
    mtk_nfcho_ndefrecord_t NDEFData;

    mtk_nfcho_service_dbg_output("token_parse,%d" , Length);

    if ((Data != NULL) && (Length != 0))
    {
        DataIdx = 0;

        while (DataIdx < Length)
        {
            memset(&NDEFData, 0x00, sizeof(mtk_nfcho_ndefrecord_t));
            memset(TokenData, 0x00, PAYLOAD_LENGTH);
            RadioType = 0xFF;
            TokenLength = 0;
            ret = mtk_nfchod_NdefRecord_Parse(&NDEFData, (Data+DataIdx));

            if (ret == 0)
            {
#if 0
                mtk_nfcho_service_dbg_output("TypeL,%d",NDEFData.TypeLength);
                for (idx = 0; idx < NDEFData.TypeLength ; idx++)
                {
                    mtk_nfcho_service_dbg_output("[%d]=%c", idx, NDEFData.Type[idx]);
                }
                mtk_nfcho_service_dbg_output("PayLength,%d", NDEFData.PayloadLength);
                for (idx = 0; idx < NDEFData.PayloadLength ; idx++)
                {
                    mtk_nfcho_service_dbg_output("[%d]=%x",idx,NDEFData.PayloadData[idx]);
                }
#endif

                for (idx = 0; idx < MTK_NFCHO_RADIO_END ; idx++)
                {
                    if ( (strncmp((const char*)NDEFData.Type, g_RadioType[idx], NDEFData.TypeLength) == 0)&&
                         (NDEFData.TypeLength == g_RadioLth[idx]) )
                    {
                        if ((NDEFData.PayloadLength != 0) && (NDEFData.PayloadData != NULL))
                        {
                            TokenLength = (UINT16)NDEFData.PayloadLength;
                            memcpy(TokenData, NDEFData.PayloadData, NDEFData.PayloadLength);
                            RadioType = idx;
                            DataType = mtk_nfcho_handover_radio_payload_parse(RadioType,  TokenData, TokenLength);
                            if (DataType == MTK_NFCHO_RADIODATA_BEAMPLUS)
                            {
                                RadioType = MTK_NFCHO_RADIO_WIFIFC;
                            }
                            break;
                        }
                    }
                }
                if (RadioType == 0xFF) // Common NDEF record
                {
                    if ((NDEFData.Tnf == MTK_NFC_HO_NFCWELLKNOWN_RTD) && (*NDEFData.Type == 'T') && (NDEFData.TypeLength == 1))
                    {
                        mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_TXT_RTD, 0, (UINT16)(NDEFData.PayloadLength), &NDEFData.PayloadData[0]);
                    }
                    else if ((NDEFData.Tnf == MTK_NFC_HO_NFCWELLKNOWN_RTD) &&(*NDEFData.Type == 'U') && (NDEFData.TypeLength == 1))
                    {
                        mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_URI_RTD, 0,  (UINT16)(NDEFData.PayloadLength), &NDEFData.PayloadData[0]);
                    }
                    else
                    {
                        UINT8* pOData = NULL;
                        UINT16 ODataLth = (UINT16)(NDEFData.PayloadLength + NDEFData.TypeLength);

                        pOData = (UINT8*)malloc(ODataLth);

                        if (pOData != NULL)
                        {
                            memset(pOData, 0x00, ODataLth);

                            if (NDEFData.TypeLength != 0)
                            {
                                memcpy(pOData, NDEFData.Type, NDEFData.TypeLength);
                            }
                            memcpy( (pOData+NDEFData.TypeLength), NDEFData.PayloadData, NDEFData.PayloadLength);

                            mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, NDEFData.Tnf, (UINT16)NDEFData.TypeLength, ODataLth, pOData);
                            free(pOData);
                            pOData = NULL;
                        }
                    }
                }
                else
                {
                    if (RadioType == MTK_NFCHO_RADIO_WIFIWPS)
                    {
                        if (DataType == MTK_NFCHO_RADIODATA_PASSWD)
                        {
                            mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WPSPD_TYPE, 0,  TokenLength, TokenData );
                        }
                        else if(DataType == MTK_NFCHO_RADIODATA_CONFIG)
                        {
                            mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WPSCF_TYPE, 0,  TokenLength, TokenData );
                        }
                        else
                        {
                            mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WPSCF_TYPE, 0,  TokenLength , TokenData);
                        }

                    }
                    else if(RadioType == MTK_NFCHO_RADIO_BT)
                    {
                        mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_BTS_TYPE, 0,  TokenLength , TokenData);
                    }
                    else
                    {
                        mtk_nfcho_service_event_handle(MTK_NFC_APP_EVENT_READ, MTK_NFC_HO_WPSCF_TYPE, 0,  TokenLength , TokenData);
                    }
                }
                if ((NDEFData.Flags & MTK_NFC_NDEFRECORD_FLAGS_IL) == MTK_NFC_NDEFRECORD_FLAGS_IL)
                {
                    DataIdx  = DataIdx  + NDEFData.PayloadLength + NDEFData.IdLength + NDEFData.TypeLength + 4;
                }
                else
                {
                    DataIdx = DataIdx + NDEFData.PayloadLength+NDEFData.TypeLength + 3;
                }
                mtk_nfcho_service_dbg_output("token_par,%d,Tnf,%d,%d,%d,%d,R,%d,%d",RadioIdx, NDEFData.Tnf, NDEFData.PayloadLength, DataIdx, RadioIdx, RadioType, DataType );
                RadioIdx++;
            }
        }
    }
    else
    {
        ret = -1;
    }

    return ret;
}





INT32 mtk_nfcho_handover_select_build( UINT8 Num, UINT8 Power[], UINT8 Radio[], MTK_NFC_APP_DATA_T* pData,
                                       UINT32* pOutLength, UINT8 OutData[] )
{
    INT32 ret = 0 ,Idx = 0, AcLength = 0;
    UINT32 BytesWritten = 0, TotalLength = 0;
    UINT8 u1HS[] = "Hs";
    UINT8 u1ac[] = "ac";
    UINT8 u1acData[4];
    UINT8 IdxRadio = 0, u1RefData = 0;
    UINT8 u1Data[128];
    UINT8* pu1PayId = NULL;
    mtk_nfcho_ndefrecord_t* pAcRec = NULL;
    mtk_nfcho_ndefrecord_t** NdefRecord = NULL;

    mtk_nfcho_service_dbg_output("select_build,%d", Num);

    if( (pOutLength != NULL) && (OutData != NULL) && (Num != 0) && (pData != NULL))
    {
        memset(u1Data, 0x00, 128);

        NdefRecord = (mtk_nfcho_ndefrecord_t**)malloc(sizeof(mtk_nfcho_ndefrecord_t*)*(Num+1));

        if (NdefRecord != NULL)
        {
            for (Idx = 0; Idx < (Num+1) ; Idx++)
            {
                NdefRecord[Idx] = (mtk_nfcho_ndefrecord_t*)malloc(sizeof(mtk_nfcho_ndefrecord_t));
                memset(NdefRecord[Idx] , 0x00, (sizeof(mtk_nfcho_ndefrecord_t)));
            }

            NdefRecord[0]->Flags = (MTK_NFC_NDEFRECORD_FLAGS_MB|MTK_NFC_NDEFRECORD_FLAGS_SR);
            NdefRecord[0]->Tnf = MTK_NFC_NDEFRECORD_TNF_NFCWELLKNOWN;
            NdefRecord[0]->TypeLength = 2;
            NdefRecord[0]->Type = u1HS;

            u1Data[0] = 0x12;
            AcLength = 1;
            // Add Ac Record
            pAcRec = (mtk_nfcho_ndefrecord_t*)malloc(sizeof(mtk_nfcho_ndefrecord_t));
            if (pAcRec != NULL)
            {
                // Add Ac Record
                for (IdxRadio = 0 ; IdxRadio < Num ; IdxRadio++)
                {
                    memset(pAcRec, 0x00, (sizeof(mtk_nfcho_ndefrecord_t)));
                    u1RefData = 0;
                    pAcRec->Flags = (MTK_NFC_NDEFRECORD_FLAGS_SR);
                    if (IdxRadio == (Num-1))
                    {
                        pAcRec->Flags = (pAcRec->Flags|(MTK_NFC_NDEFRECORD_FLAGS_ME));
                    }
                    if (IdxRadio == 0)
                    {
                        pAcRec->Flags = (pAcRec->Flags|(MTK_NFC_NDEFRECORD_FLAGS_MB));
                    }
                    pAcRec->Tnf = MTK_NFC_NDEFRECORD_TNF_NFCWELLKNOWN;
                    pAcRec->TypeLength = 2;
                    pAcRec->Type = u1ac;
                    pAcRec->PayloadLength = 4;
                    u1acData[0] = Power[IdxRadio];
                    u1acData[1] = 1;
                    u1RefData = (UINT8)(0x30 | IdxRadio);
                    u1acData[2] = u1RefData;
                    u1acData[3] = 0;
                    pAcRec->PayloadData = u1acData;
                    BytesWritten = 0;
                    mtk_nfchod_NdefRecord_Generate(pAcRec,&u1Data[AcLength], (128-AcLength), &BytesWritten);
                    AcLength = AcLength + BytesWritten;
                    //  mtk_nfcho_service_dbg_output("(%d),BytesWritten,%d, L,%d", IdxRadio, BytesWritten, AcLength);

#if  0
                    {
                        INT32 iii = 0;

                        for (iii=0; iii<AcLength ; iii++)
                        {
                            mtk_nfcho_service_dbg_output("(%d), 0x%x", iii , u1Data[iii]);
                        }
                    }
#endif
                }

                // mtk_nfcho_service_dbg_output("AcLength,%d", AcLength);
                NdefRecord[0]->PayloadLength = AcLength;
                NdefRecord[0]->PayloadData = u1Data;
                free(pAcRec);
                pAcRec = NULL;

                // Add Radio Carrier Record

                pu1PayId =  (UINT8*)malloc(sizeof(UINT8)*Num);

                if (pu1PayId != NULL)
                {
                    memset(pu1PayId, 0x00, (sizeof(UINT8)*Num));

                    for (IdxRadio = 0 ; IdxRadio < Num ; IdxRadio++)
                    {
                        if (pData[IdxRadio].lth < 256)
                        {
                            NdefRecord[1+IdxRadio]->Flags = (NdefRecord[1+IdxRadio]->Flags | MTK_NFC_NDEFRECORD_FLAGS_SR);
                        }
                        if (IdxRadio == (Num-1))
                        {
                            NdefRecord[1+IdxRadio]->Flags = (NdefRecord[1+IdxRadio]->Flags | MTK_NFC_NDEFRECORD_FLAGS_ME);
                        }
                        NdefRecord[1+IdxRadio]->Flags = (NdefRecord[1+IdxRadio]->Flags|MTK_NFC_NDEFRECORD_FLAGS_IL);

                        NdefRecord[1+IdxRadio]->Tnf = MTK_NFC_NDEFRECORD_TNF_MEDIATYPE;



                        NdefRecord[1+IdxRadio]->IdLength = 1;
                        pu1PayId[IdxRadio] = (UINT8)(0x30 | IdxRadio);

                        NdefRecord[1+IdxRadio]->Id = &pu1PayId[IdxRadio];

                        NdefRecord[1+IdxRadio]->Type = (UINT8*)g_RadioType[Radio[IdxRadio]];
                        NdefRecord[1+IdxRadio]->TypeLength = g_RadioLth[Radio[IdxRadio]];
                        NdefRecord[1+IdxRadio]->PayloadData = pData[IdxRadio].data;
                        NdefRecord[1+IdxRadio]->PayloadLength = pData[IdxRadio].lth;

                        mtk_nfcho_service_dbg_output("%d,%d,%d", NdefRecord[1+IdxRadio]->TypeLength ,  NdefRecord[1+IdxRadio]->PayloadLength, pData[IdxRadio].lth );

                    }
                    TotalLength = 0;

                    for (Idx = 0; Idx <= Num ; Idx++)
                    {
                        BytesWritten = 0;
                        ret = mtk_nfchod_NdefRecord_Generate(NdefRecord[Idx],(OutData+TotalLength), (DEFAULT_ACCEPTABLE_LENGTH-TotalLength), &BytesWritten);
                        TotalLength = TotalLength + BytesWritten;
                        //         mtk_nfcho_service_dbg_output("Idx,%d,Written,%d,L,%d", Idx, BytesWritten, TotalLength);
                    }

                    (*pOutLength) = TotalLength;
                    free(pu1PayId);
                    pu1PayId = NULL;
                }
                else
                {
                    mtk_nfcho_service_dbg_output("para,err,1");
                    ret = -1;
                }
            }
            else
            {
                mtk_nfcho_service_dbg_output("para,err,2");
                ret = -2;
            }

            for (Idx = 0; Idx < (Num+1) ; Idx++)
            {
                free(NdefRecord[Idx]);
                NdefRecord[Idx] = NULL;
            }
            free(NdefRecord);
            NdefRecord = NULL;
        }
        else
        {
            mtk_nfcho_service_dbg_output("para,err,3");
            ret = -3;
        }
    }
    else
    {
        mtk_nfcho_service_dbg_output("para,err,4");
        ret = -4;
    }
    return ret;
}


INT32 mtk_nfcho_handover_request_build(UINT8 Num, UINT8 Power[], UINT8 Radio[], MTK_NFC_APP_DATA_T* pData,
                                       UINT32* pOutLength, UINT8 OutData[], UINT16* pRequestRandom)
{
    INT32 ret = 0 ,Idx = 0, AcLength = 0;
    UINT32 BytesWritten = 0, TotalLength = 0;
    UINT8 u1HR[] = "Hr";
    UINT8 u1cr[] = "cr";
    UINT8 u1ac[] = "ac";
    UINT8 u1arData[2];
    UINT8 u1acData[4];
    UINT8 u1Data[128];
    UINT8 IdxRadio = 0, u1RefData = 0;
    UINT8* pu1PayId = NULL;
    mtk_nfcho_ndefrecord_t** NdefRecord = NULL;
    mtk_nfcho_ndefrecord_t* pAcRec = NULL;

    mtk_nfcho_service_dbg_output("request_build,%d", Num);

    if ((pOutLength != NULL) && (OutData != NULL) && (Num != 0) && (pData != NULL))
    {

        NdefRecord = (mtk_nfcho_ndefrecord_t**)malloc(sizeof(mtk_nfcho_ndefrecord_t*)*(Num+1));

        if (NdefRecord != NULL)
        {
            for (Idx = 0; Idx < (Num+1) ; Idx++)
            {
                NdefRecord[Idx] = (mtk_nfcho_ndefrecord_t*)malloc(sizeof(mtk_nfcho_ndefrecord_t));
                memset(NdefRecord[Idx] , 0x00, (sizeof(mtk_nfcho_ndefrecord_t)));
            }

            srand((UINT32)time(NULL));
            TotalLength = 0;
            memset(u1Data, 0x00, 128);

            NdefRecord[0]->Flags = (MTK_NFC_NDEFRECORD_FLAGS_MB|MTK_NFC_NDEFRECORD_FLAGS_SR);
            NdefRecord[0]->Tnf = MTK_NFC_NDEFRECORD_TNF_NFCWELLKNOWN;
            NdefRecord[0]->TypeLength = 2;
            NdefRecord[0]->Type = u1HR;
            u1Data[0] = 0x12;

            pAcRec = (mtk_nfcho_ndefrecord_t*)malloc(sizeof(mtk_nfcho_ndefrecord_t));
            AcLength = 1;

            if (pAcRec != NULL)
            {
                memset(pAcRec, 0x00, (sizeof(mtk_nfcho_ndefrecord_t)));

                pAcRec->Flags = (MTK_NFC_NDEFRECORD_FLAGS_MB | MTK_NFC_NDEFRECORD_FLAGS_SR);
                pAcRec->Tnf = MTK_NFC_NDEFRECORD_TNF_NFCWELLKNOWN;
                pAcRec->TypeLength = 2;
                pAcRec->Type = u1cr;
                pAcRec->PayloadLength = 2;
                (*pRequestRandom) = rand() % 0x10000;
                memcpy(u1arData , (UINT8*)pRequestRandom, 2);
                pAcRec->PayloadData = u1arData;

                BytesWritten = 0;
                ret = mtk_nfchod_NdefRecord_Generate(pAcRec,(&u1Data[AcLength]), (128-AcLength), &BytesWritten);
                AcLength = AcLength + BytesWritten;
                //  mtk_nfcho_service_dbg_output("AcLength,%d", AcLength);


                for (IdxRadio = 0; IdxRadio < Num; IdxRadio++)
                {
                    memset(pAcRec, 0x00, (sizeof(mtk_nfcho_ndefrecord_t)));
                    pAcRec->Flags = MTK_NFC_NDEFRECORD_FLAGS_SR;

                    if ( IdxRadio == (Num-1))
                    {
                        pAcRec->Flags = (pAcRec->Flags | MTK_NFC_NDEFRECORD_FLAGS_ME);
                    }
                    pAcRec->Tnf = MTK_NFC_NDEFRECORD_TNF_NFCWELLKNOWN;
                    pAcRec->TypeLength = 2;
                    pAcRec->Type = u1ac;
                    u1acData[0] = Power[IdxRadio];
                    u1acData[1] = 1;
                    u1RefData = (UINT8)(0x30 | IdxRadio);
                    u1acData[2] = u1RefData;
                    u1acData[3] = 0;
                    pAcRec->PayloadData = u1acData;
                    pAcRec->PayloadLength = 4;

                    BytesWritten = 0;
                    ret = mtk_nfchod_NdefRecord_Generate(pAcRec,(&u1Data[AcLength]), (128-AcLength), &BytesWritten);
                    AcLength = AcLength + BytesWritten;
                    //   mtk_nfcho_service_dbg_output("(%d),BytesWritten,%d, L,%d", IdxRadio, BytesWritten, AcLength);
                }
                NdefRecord[0]->PayloadData = u1Data;
                NdefRecord[0]->PayloadLength = AcLength;
                free(pAcRec);
                pAcRec = NULL;
                //  mtk_nfcho_service_dbg_output("AcLength,%d", AcLength);
                // Add Radio Carrier Record

                pu1PayId =  (UINT8*)malloc(sizeof(UINT8)*Num);

                if (pu1PayId != NULL)
                {
                    memset(pu1PayId, 0x00, (sizeof(UINT8)*Num));

                    for (IdxRadio = 0 ; IdxRadio < Num ; IdxRadio++)
                    {
                        if (pData[IdxRadio].lth < 256)
                        {
                            NdefRecord[1+IdxRadio]->Flags = (NdefRecord[1+IdxRadio]->Flags | MTK_NFC_NDEFRECORD_FLAGS_SR);
                        }
                        if (IdxRadio == (Num-1))
                        {
                            NdefRecord[1+IdxRadio]->Flags = (NdefRecord[1+IdxRadio]->Flags | MTK_NFC_NDEFRECORD_FLAGS_ME);
                        }
                        NdefRecord[1+IdxRadio]->Flags = (NdefRecord[1+IdxRadio]->Flags|MTK_NFC_NDEFRECORD_FLAGS_IL);

                        NdefRecord[1+IdxRadio]->Tnf = MTK_NFC_NDEFRECORD_TNF_MEDIATYPE;
                        NdefRecord[1+IdxRadio]->IdLength = 1;

                        pu1PayId[IdxRadio] = (UINT8)(0x30 | IdxRadio);
                        NdefRecord[1+IdxRadio]->Id = &pu1PayId[IdxRadio];

                        NdefRecord[1+IdxRadio]->Type = (UINT8*)g_RadioType[Radio[IdxRadio]];
                        NdefRecord[1+IdxRadio]->TypeLength = g_RadioLth[Radio[IdxRadio]];
                        NdefRecord[1+IdxRadio]->PayloadData = pData[IdxRadio].data;
                        NdefRecord[1+IdxRadio]->PayloadLength = pData[IdxRadio].lth;
                        mtk_nfcho_service_dbg_output("%s,%d,%d,%d",  NdefRecord[1+IdxRadio]->Type, NdefRecord[1+IdxRadio]->TypeLength ,  NdefRecord[1+IdxRadio]->PayloadLength, pData[IdxRadio].lth );
                    }
                    TotalLength = 0;

                    for (Idx = 0; Idx <= Num ; Idx++)
                    {
                        BytesWritten = 0;
                        ret = mtk_nfchod_NdefRecord_Generate(NdefRecord[Idx],(OutData+TotalLength), (DEFAULT_ACCEPTABLE_LENGTH-TotalLength), &BytesWritten);
                        TotalLength = TotalLength + BytesWritten;
                        // mtk_nfcho_service_dbg_output("Idx,%d,Written,%d,L,%d", Idx, BytesWritten, TotalLength);
                    }

                    (*pOutLength) = TotalLength;
                    free(pu1PayId);
                    pu1PayId = NULL;
                }
                else
                {
                    mtk_nfcho_service_dbg_output("para,err,1");
                    ret = -1;
                }

            }
            else
            {
                mtk_nfcho_service_dbg_output("para,err,2");
                ret = -2;
            }
            for (Idx = 0; Idx < (Num) ; Idx++)
            {
                free(NdefRecord[Idx]);
                NdefRecord[Idx] = NULL;
            }
            free(NdefRecord);
            NdefRecord = NULL;

        }
        else
        {
            mtk_nfcho_service_dbg_output("para,err,3");
            ret = -3;
        }
    }
    else
    {
        mtk_nfcho_service_dbg_output("para,err,4");
        ret = -4;
    }
    return ret;

}


INT32 mtk_nfcho_handover_token_build(UINT8 TokenData[],UINT16 TokenLength, UINT8 OutData[],UINT32* pOutLength, UINT8 Radio, UINT8 Idx, UINT8 TotalNum)
{
    INT32 ret = 0;
    UINT32 BytesWritten = 0;
    mtk_nfcho_ndefrecord_t NdefRecord;

    memset(&NdefRecord, 0x00, sizeof(mtk_nfcho_ndefrecord_t));

    NdefRecord.Flags = 0;

    if (TokenLength < 256)
    {
        NdefRecord.Flags =  (NdefRecord.Flags | (MTK_NFC_NDEFRECORD_FLAGS_SR));
    }
    if (Idx == 0)
    {
        NdefRecord.Flags = (NdefRecord.Flags |(MTK_NFC_NDEFRECORD_FLAGS_MB));
    }
    if (Idx == (TotalNum-1))
    {
        NdefRecord.Flags = (NdefRecord.Flags |(MTK_NFC_NDEFRECORD_FLAGS_ME));
    }

    NdefRecord.Tnf = MTK_NFC_NDEFRECORD_TNF_MEDIATYPE;

    if ((Radio < MTK_NFCHO_RADIO_END) && ((TokenLength != 0) && (TokenData != NULL)) &&((pOutLength != NULL) && (OutData != NULL)))
    {
        NdefRecord.TypeLength = g_RadioLth[Radio];
        NdefRecord.Type = (UINT8*)g_RadioType[Radio];

        NdefRecord.PayloadLength = TokenLength;
        NdefRecord.PayloadData = TokenData;
        ret = mtk_nfchod_NdefRecord_Generate(&NdefRecord, OutData, PAYLOAD_LENGTH, &BytesWritten);
        (*pOutLength) = (*pOutLength) + BytesWritten;

        mtk_nfcho_service_dbg_output("handover_token_build,L,%d",(*pOutLength));

        if ((*pOutLength) > PAYLOAD_LENGTH)
        {
            ret = -1;
        }
    }
    else
    {
        mtk_nfcho_service_dbg_output("handover_token_build,err,%d,%d",Radio, TokenLength);
        ret = -1;
    }
    return ret;
}




INT32 mtk_nfcho_snep_msg_send(UINT8 field, UINT32 length, UINT32 acceptableLength, UINT8 data[])
{
    UINT8* Data = NULL;
    UINT32 MsgLength = 0;
    UINT32 PDULength = length , AcceptL = acceptableLength;

    mtk_nfcho_service_dbg_output("snep_send,f,%d,L,%d,accept,%d", field , PDULength, acceptableLength);
    if (field == SNEP_RESPONSE_DATA)
    {
        MsgLength = PDULength;
        Data = (UINT8*)malloc(PDULength);
        memset(Data, 0x00 , MsgLength);
        memcpy(Data, data, PDULength);
    }
    else if (AcceptL == 0)
    {
        MsgLength = (SNEP_HEADER_LENGTH+PDULength);
        Data = (UINT8*)malloc(MsgLength);
        memset(Data, 0x00 , MsgLength);
        Data[0] = SNEP_VERSION;
        Data[1] = field;

        if ( (data != NULL) && (PDULength != 0) )
        {
            memcpy(&Data[6], data, PDULength);
        }
        PDULength = SWAP32(PDULength);
        *(UINT32*)&Data[2] = PDULength;
        mtk_nfcho_service_dbg_output("OutL,0x%x",PDULength);
    }
    else
    {
        MsgLength = (SNEP_HEADER_LENGTH+PDULength+4);
        Data = (UINT8*)malloc(MsgLength);
        memset(Data, 0x00 , MsgLength);
        Data[0] = SNEP_VERSION;
        Data[1] = field;

        if ( (data != NULL) && (PDULength != 0) )
        {
            memcpy(&Data[10], data, PDULength);
        }
        PDULength = SWAP32(PDULength);

        *(UINT32*)&Data[2] = PDULength;
        AcceptL = SWAP32(AcceptL);
        *(UINT32*)&Data[6] = AcceptL;
        mtk_nfcho_service_dbg_output("OutL,0x%x,acceptL,0x%x",PDULength,AcceptL);
    }
    // mtk_nfcho_service_dbg_output("snep_send,%d", MsgLength);

#if 0
    if (Data != NULL)
    {
        UINT32 ii = 0;
        for (ii = 0; ii < MsgLength ; ii++)
        {
            mtk_nfcho_service_dbg_output("Data[%d],0x%x,%c", ii, Data[ii] , Data[ii]);
        }
    }
#endif
    mtk_nfcho_service_p2p_send_data(Data, MsgLength);
    if (Data != NULL)
    {
        free(Data);
    }

    return 0;
}

/********************************************************************************************************************
Parse LLCP's received SDUs

SNEP Client :
        Response Control Packet
        NDEF DATA Packet by requesting Get Command

 SNEP Server:
        Request Control Packet
        NDEF DATA Packet by receiving Put Command

 ********************************************************************************************************************/
INT32 mtk_nfcho_snep_parse(UINT8* u1Data, UINT16 u2Length, UINT8 u1ConnType, UINT8* pReadStatus, UINT8 EnSnep)
{

    UINT8 ReqVer = 0;
    UINT8 ReqCmd = 0;
    UINT32  ReqLength = 0;
    UINT32  ReqAcceptableLength = 0;
    UINT16 ReadData = 0;
    UINT32  ret = 0;

    mtk_nfcho_service_dbg_output("snep_parse,%d",u2Length);

    if (EnSnep == 1)
    {
        if ((u2Length == 0) || (u1Data == NULL) || (pReadStatus == NULL))
        {
            mtk_nfcho_snep_msg_send(SNEP_RESPONSE_REJECT, 0, 0, NULL);
        }
        else
        {
            if (u1ConnType == MTK_NFC_LLCP_SERVICEROLE_SERVER)
            {
                if ((*pReadStatus) == 0)
                {
                    mtk_nfcho_service_dbg_output("Read,ongoing,%d", u1ConnType);
                    if (u2Length <= 0)
                    {
                        // Send SNEP_RESPONSE_REJECT
                        mtk_nfcho_service_dbg_output("RESPONSE_REJECT,Length,%d", u2Length);
                        mtk_nfcho_snep_msg_send(SNEP_RESPONSE_REJECT, 0, 0, NULL);
                    }
                    else
                    {
                        ReadData = u2Length;
                        memcpy(&SnepReadData[SnepReadLength], u1Data, ReadData);
                        SnepReadLength = SnepReadLength + ReadData;
                        if (SnepReadLength == ReqLength)
                        {
                            mtk_nfcho_service_dbg_output("RESPONSE_SUCCESS");
                            mtk_nfcho_snep_msg_send(SNEP_RESPONSE_SUCCESS, 0, 0, NULL);
                            (*pReadStatus) = 1;
                            ret = mtk_nfcho_handover_nego_mgt(SnepReadData, SnepReadLength, NULL, NULL);
                        }
                        else if (SnepReadLength >= ReqLength)
                        {
                            // Send SNEP_RESPONSE_REJECT
                            mtk_nfcho_service_dbg_output("RESPONSE_REJECT");
                            mtk_nfcho_snep_msg_send(SNEP_RESPONSE_REJECT, 0, 0, NULL);
                            (*pReadStatus) = 2; //Error
                        }
                        mtk_nfcho_service_dbg_output("ReadStatus,%d,SnepLength,%d,ReadData,%d",(*pReadStatus), SnepReadLength, ReadData);
                    }
                }
                else
                {
                    if (u2Length >= SNEP_HEADER_LENGTH)
                    {
                        ReqVer = u1Data[0];
                        ReqCmd = u1Data[1];
                        ReqLength = *(UINT32*)&u1Data[2];
                        ReqLength = SWAP32(ReqLength);
                        ReadData = u2Length-SNEP_HEADER_LENGTH;

                        mtk_nfcho_service_dbg_output("Ver,0x%x,Cmd,0x%x,ReqLength,%d,ReadData,%d", ReqVer , ReqCmd, ReqLength , ReadData);

                        if ( ((ReqVer & 0xF0)>>4) != SNEP_VERSION_MAJOR)
                        {
                            mtk_nfcho_service_dbg_output("SNEP_RESPONSE_UNSUPPORTED_VERSION,%d", ReqVer);
                            mtk_nfcho_snep_msg_send(SNEP_RESPONSE_UNSUPPORTED_VERSION, 0, 0, NULL);
                        }
                        else
                        {
                            if (ReqCmd == SNEP_REQUEST_GET)  // Read and Write
                            {
                                ReqAcceptableLength = *(UINT32*)&u1Data[6];
                                ReqAcceptableLength = SWAP32(ReqAcceptableLength);
                                SnepWriteLength = 0;
                                memset(SnepWriteData, 0x00, sizeof(SnepWriteData));
                                mtk_nfcho_service_dbg_output("ReqAcceptable,0x%x", ReqAcceptableLength);
                                // Todo parse NDEF to build message
                                if (u2Length > 10 )
                                {
                                    ret = mtk_nfcho_handover_nego_mgt( &u1Data[10], (u2Length - 10), SnepWriteData,  &SnepWriteLength);
                                    if (ret == 0)
                                    {
                                        mtk_nfcho_service_dbg_output("ReqAcceptable,%d,SnepWrite,%d", ReqAcceptableLength , SnepWriteLength);
                                        if ( (ReqAcceptableLength >= SnepWriteLength)&&(SnepWriteLength > 0) )
                                        {
                                            mtk_nfcho_service_dbg_output("SNEP_RESPONSE_SUCCESS");
#if 0
                                            if (SnepWriteData != NULL)
                                            {
                                                UINT32 ii = 0;
                                                for (ii = 0; ii < SnepWriteLength ; ii++)
                                                {
                                                    mtk_nfcho_service_dbg_output("SnepWriteData[%d],0x%x,%c", ii, SnepWriteData[ii] , SnepWriteData[ii]);
                                                }
                                            }
#endif
                                            mtk_nfcho_snep_msg_send(SNEP_RESPONSE_SUCCESS, SnepWriteLength, 0, SnepWriteData);
                                            (*pReadStatus) = 1;
                                        }
                                        else if ((ReqAcceptableLength < SnepWriteLength))
                                        {
                                            mtk_nfcho_snep_msg_send(SNEP_RESPONSE_SUCCESS, ReqAcceptableLength, 0, SnepWriteData);
                                            SnepMaxLength = ReqAcceptableLength;
                                            (*pReadStatus) = 3; //  Wait SNEP_REQUEST_CONTINUE
                                        }
                                        else
                                        {
                                            //SNEP_RESPONSE_NOT_FOUND
                                            mtk_nfcho_service_dbg_output("SNEP_RESPONSE_NOT_FOUND,1");
                                            mtk_nfcho_snep_msg_send(SNEP_RESPONSE_NOT_FOUND, 0, 0, NULL);
                                            (*pReadStatus) = 1;
                                        }
                                    }
                                    else
                                    {
                                        (*pReadStatus) = 2; //Error
                                        ret = -1;
                                    }
                                }
                                else
                                {
                                    //SNEP_RESPONSE_NOT_FOUND
                                    mtk_nfcho_service_dbg_output("SNEP_RESPONSE_NOT_FOUND,2");
                                    mtk_nfcho_snep_msg_send(SNEP_RESPONSE_NOT_FOUND, 0, 0, NULL);
                                    (*pReadStatus) = 1;
                                }
                            }
                            else if (ReqCmd == SNEP_REQUEST_PUT) //Read
                            {
                                memset(SnepReadData, 0x00, sizeof(SnepReadData));

                                SnepReadLength = ReadData;
                                memcpy(SnepReadData, &u1Data[6], ReadData);
                                mtk_nfcho_service_dbg_output("ReqLength,%d,ReadData,%d", ReqLength, ReadData);
                                if (ReqLength > ReadData)
                                {
                                    // Send SNEP_RESPONSE_CONTINUE
                                    mtk_nfcho_service_dbg_output("SNEP_RESPONSE_CONTINUE");
                                    (*pReadStatus) = 0;
                                    mtk_nfcho_snep_msg_send(SNEP_RESPONSE_CONTINUE, 0, 0, NULL);
                                }
                                else
                                {
                                    mtk_nfcho_service_dbg_output("read,done");
                                    (*pReadStatus) = 1;
                                    mtk_nfcho_snep_msg_send(SNEP_RESPONSE_SUCCESS, 0, 0, NULL);
                                    ret = mtk_nfcho_handover_nego_mgt(SnepReadData, SnepReadLength, NULL, NULL);
                                }
                            }
                            else if (ReqCmd == SNEP_REQUEST_CONTINUE)
                            {
                                mtk_nfcho_snep_msg_send(SNEP_RESPONSE_DATA, (SnepWriteLength-SnepMaxLength), 0, SnepWriteData+SnepWriteLength);
                                (*pReadStatus) = 1;
                            }
                            else
                            {
                                mtk_nfcho_service_dbg_output("SNEP_RESPONSE_BAD_REQUEST");
                                mtk_nfcho_snep_msg_send(SNEP_RESPONSE_BAD_REQUEST, 0, 0, NULL);
                                (*pReadStatus) = 2; //Error
                            }
                        }
                    }
                    else
                    {
                        mtk_nfcho_snep_msg_send(SNEP_RESPONSE_REJECT, 0, 0, NULL);
                    }
                }
            }
            else if(u1ConnType == MTK_NFC_LLCP_SERVICEROLE_CLIENT)
            {
                if ((*pReadStatus)== 0)
                {
                    mtk_nfcho_service_dbg_output("client,read,ongoing");
                    ReadData = u2Length;
                    memcpy(&SnepReadData[SnepReadLength], u1Data, ReadData);
                    SnepReadLength = SnepReadLength + ReadData;
                    mtk_nfcho_service_dbg_output("snepL,%d,reqL,%d",SnepReadLength, ReqLength);
                    if (SnepReadLength == ReqLength)
                    {
                        (*pReadStatus) = 1;
                        ret = mtk_nfcho_handover_nego_mgt(SnepReadData, SnepReadLength, NULL, NULL);
                    }
                    else if (SnepReadLength >= ReqLength)
                    {
                        (*pReadStatus) = 2; //Error
                    }
                    mtk_nfcho_service_dbg_output("ReadStatus,%d,SnepLength,%d,ReadData,%d",(*pReadStatus), SnepReadLength, ReadData);
                }
                else
                {
                    if (u2Length >= SNEP_HEADER_LENGTH)
                    {
                        ReqVer = u1Data[0];
                        ReqCmd = u1Data[1];
                        ReqLength = *(UINT32*)&u1Data[2];
                        ReqLength = SWAP32(ReqLength);
                        ReadData = u2Length-SNEP_HEADER_LENGTH;

                        mtk_nfcho_service_dbg_output("Ver,0x%x,Cmd,0x%x,ReqLength,%d,ReadData,%d", ReqVer , ReqCmd, ReqLength , ReadData);

                        if ( ((ReqVer & 0xF0)>>4) != SNEP_VERSION_MAJOR)
                        {
                            mtk_nfcho_service_dbg_output("SNEP_RESPONSE_UNSUPPORTED_VERSION,0x%x", ReqVer);
                            mtk_nfcho_snep_msg_send(SNEP_RESPONSE_UNSUPPORTED_VERSION, 0, 0, NULL);
                        }
                        else
                        {
                            if (ReqCmd == SNEP_RESPONSE_SUCCESS)
                            {
                                // Check respond NDEF
                                memset(SnepReadData, 0x00, sizeof(SnepReadData));
                                if (ReadData != 0)
                                {
                                    memcpy(SnepReadData, &u1Data[6], ReadData);
                                    SnepReadLength = ReadData;

                                    if (ReqLength > ReadData)
                                    {
                                        // Send SNEP_RESPONSE_CONTINUE
                                        mtk_nfcho_service_dbg_output("SNEP_REQUEST_CONTINUE");
                                        (*pReadStatus) = 0;
                                        mtk_nfcho_snep_msg_send(SNEP_REQUEST_CONTINUE, 0, 0, NULL);
                                    }
                                    else
                                    {
                                        mtk_nfcho_service_dbg_output("read,done");
                                        (*pReadStatus) = 1;
                                        ret = mtk_nfcho_handover_nego_mgt(SnepReadData, SnepReadLength, NULL, NULL);
                                    }
                                }
                                else
                                {
                                    mtk_nfcho_service_dbg_output("received,SNEP_RESPONSE_SUCCESS");
                                    (*pReadStatus) = 1;
                                }
                            }
                            else if (ReqCmd == SNEP_RESPONSE_CONTINUE)
                            {
                                mtk_nfcho_service_dbg_output("without,data");
                            }
                            else if  (((ReqCmd >= 0xC0) && (ReqCmd <= 0xC2)) ||
                                      ((ReqCmd >= 0xE0) && (ReqCmd <= 0xE1)) ||
                                      (ReqCmd == 0xFF))
                            {
                                mtk_nfcho_service_dbg_output("received,err,0x%x", ReqCmd);
                                (*pReadStatus) = 2; //Error
                            }
                            else
                            {
                                mtk_nfcho_service_dbg_output("received,invalid,0x%x", ReqCmd);
                                (*pReadStatus) = 2; //Error
                            }
                        }
                    }
                    else
                    {
                        mtk_nfcho_snep_msg_send(SNEP_RESPONSE_REJECT, 0, 0, NULL);
                    }
                }
            }
        }
    }
    else if (EnSnep == 0)
    {
        SnepWriteLength = 0;
        memset(SnepWriteData, 0x00, sizeof(SnepWriteData));
        ret = mtk_nfcho_handover_nego_mgt( u1Data, u2Length, SnepWriteData, &SnepWriteLength);
        if (ret == 0)
        {
            mtk_nfcho_service_p2p_send_data(SnepWriteData, SnepWriteLength);
            (*pReadStatus) = 1;
        }
        else
        {
            (*pReadStatus) = 2;
        }
        mtk_nfcho_service_dbg_output("nego_mgt,%d,%d",EnSnep, ret);
    }
    else
    {
        mtk_nfcho_service_dbg_output("nego_mgt,%d", EnSnep);
    }
    return ret;
}

INT32 mtk_nfcho_snep_recv(void)
{
    INT32 ret = 0;

//   mtk_nfcho_service_dbg_output("mtk_nfcho_snep_server_recv");

    ret = mtk_nfcho_service_p2p_receive_data();

    return ret;

}

void mtk_nfcho_snep_stop(UINT32 handle)
{
//   mtk_nfcho_service_dbg_output("Ready to Close Snep");
    mtk_nfcho_service_p2p_close_service(handle);
}

INT32 mtk_nfcho_snep_client_put( UINT8 SnepData[], UINT32 Length)
{
    INT32 ret = 0;

    ret =  mtk_nfcho_snep_msg_send(SNEP_REQUEST_PUT, Length, 0, SnepData);

    return ret;
}

INT32 mtk_nfcho_snep_client_get( UINT8 SnepData[], UINT32 Length)
{
    INT32 ret = 0;

    ret =  mtk_nfcho_snep_msg_send(SNEP_REQUEST_GET, Length, PAYLOAD_LENGTH, SnepData);

    ret = mtk_nfcho_snep_recv();

    return ret;
}


