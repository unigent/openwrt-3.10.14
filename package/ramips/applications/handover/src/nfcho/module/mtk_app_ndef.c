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

/* to check the bitfields in the Flags Byte and return the status flag */
UINT8 mtk_nfchod_NdefRecord_NdefFlag(UINT8 Flags,UINT8 Mask)
{
    UINT8 check_flag = 0x00;
    check_flag = ((Flags) & (Mask));
    return check_flag;
}

UINT32 mtk_nfchod_NdefRecord_GetLength(mtk_nfcho_ndefrecord_t *Record)
{
    UINT32 RecordLength=1;
    UINT8  FlagCheck=0;

    FlagCheck=mtk_nfchod_NdefRecord_NdefFlag(Record->Tnf,MTK_NFC_NDEFRECORD_TNFBYTE_MASK);

    /* ++ is for the Type Length Byte */
    RecordLength++;
    if( FlagCheck != MTK_NFC_NDEFRECORD_TNF_EMPTY &&
        FlagCheck != MTK_NFC_NDEFRECORD_TNF_UNKNOWN &&
        FlagCheck != MTK_NFC_NDEFRECORD_TNF_UNCHANGED )
    {
        RecordLength += Record->TypeLength;
    }

    /* to check if payloadlength is 8bit or 32bit*/
    FlagCheck=mtk_nfchod_NdefRecord_NdefFlag(Record->Flags,MTK_NFC_NDEFRECORD_FLAGS_SR);
    if(FlagCheck!=0)
    {
        /* ++ is for the Payload Length Byte */
        RecordLength++;/* for short record*/
    }
    else
    {
        /* + PHFRINFCNDEFRECORD_NORMAL_RECORD_BYTE is for the Payload Length Byte */
        RecordLength += (4);/* for normal record*/
    }

    /* for non empty record */
    FlagCheck=mtk_nfchod_NdefRecord_NdefFlag(Record->Tnf,MTK_NFC_NDEFRECORD_TNFBYTE_MASK);
    if(FlagCheck != MTK_NFC_NDEFRECORD_TNF_EMPTY)
    {
        RecordLength += Record->PayloadLength;
    }

    /* ID and IDlength are present only if IL flag is set*/
    FlagCheck=mtk_nfchod_NdefRecord_NdefFlag(Record->Flags,MTK_NFC_NDEFRECORD_FLAGS_IL);
    if(FlagCheck!=0)
    {
        RecordLength +=Record->IdLength;
        /* ++ is for the ID Length Byte */
        RecordLength ++;
    }
    return RecordLength;
}


/* Calculate the Flags of the record */
UINT8 mtk_nfchod_NdefRecord_RecordFlag ( UINT8* Record)
{
    UINT8 flag = 0;

    if ((*Record & MTK_NFC_NDEFRECORD_FLAGS_MB) == MTK_NFC_NDEFRECORD_FLAGS_MB )
    {
        flag = flag | MTK_NFC_NDEFRECORD_FLAGS_MB;
    }
    if ((*Record & MTK_NFC_NDEFRECORD_FLAGS_ME) == MTK_NFC_NDEFRECORD_FLAGS_ME )
    {
        flag = flag | MTK_NFC_NDEFRECORD_FLAGS_ME;
    }
    if ((*Record & MTK_NFC_NDEFRECORD_FLAGS_CF) == MTK_NFC_NDEFRECORD_FLAGS_CF )
    {
        flag = flag | MTK_NFC_NDEFRECORD_FLAGS_CF;
    }
    if ((*Record & MTK_NFC_NDEFRECORD_FLAGS_SR) == MTK_NFC_NDEFRECORD_FLAGS_SR )
    {
        flag = flag | MTK_NFC_NDEFRECORD_FLAGS_SR;
    }
    if ((*Record & MTK_NFC_NDEFRECORD_FLAGS_IL) == MTK_NFC_NDEFRECORD_FLAGS_IL )
    {
        flag = flag | MTK_NFC_NDEFRECORD_FLAGS_IL;
    }
    return flag;
}

/* Calculate the Type Name Format for the record */
UINT8 mtk_nfchod_NdefRecord_TypeNameFormat ( UINT8* Record)
{
    UINT8     tnf = 0;

    switch (*Record & MTK_NFC_NDEFRECORD_TNFBYTE_MASK)
    {
        case MTK_NFC_NDEFRECORD_TNF_EMPTY:
            tnf = MTK_NFC_NDEFRECORD_TNF_EMPTY;
            break;

        case MTK_NFC_NDEFRECORD_TNF_NFCWELLKNOWN:
            tnf = MTK_NFC_NDEFRECORD_TNF_NFCWELLKNOWN;
            break;

        case MTK_NFC_NDEFRECORD_TNF_MEDIATYPE:
            tnf = MTK_NFC_NDEFRECORD_TNF_MEDIATYPE;
            break;

        case MTK_NFC_NDEFRECORD_TNF_ABSURI:
            tnf = MTK_NFC_NDEFRECORD_TNF_ABSURI;
            break;

        case MTK_NFC_NDEFRECORD_TNF_NFCEXT:
            tnf = MTK_NFC_NDEFRECORD_TNF_NFCEXT;
            break;

        case MTK_NFC_NDEFRECORD_TNF_UNKNOWN:
            tnf = MTK_NFC_NDEFRECORD_TNF_UNKNOWN;
            break;

        case MTK_NFC_NDEFRECORD_TNF_UNCHANGED:
            tnf = MTK_NFC_NDEFRECORD_TNF_UNCHANGED;
            break;

        case MTK_NFC_NDEFRECORD_TNF_RESERVED:
            tnf = MTK_NFC_NDEFRECORD_TNF_RESERVED;
            break;
        default :
            tnf = 0xFF;
            break;
    }

    return tnf;
}

INT32 mtk_nfchod_NdefRecord_RecordIDCheck ( UINT8* Record,
        UINT8     *TypeLength,
        UINT8     *TypeLengthByte,
        UINT8     *PayloadLengthByte,
        UINT32      *PayloadLength,
        UINT8     *IDLengthByte,
        UINT8     *IDLength)
{
    INT32 Status = MTK_NFC_SUCCESS;

    /* Check for Tnf bits 0x07 is reserved for future use */
    if ((*Record & MTK_NFC_NDEFRECORD_TNFBYTE_MASK) ==
        MTK_NFC_NDEFRECORD_TNF_RESERVED)
    {
        /* TNF 07  Error */
        Status = -1;
        return Status;
    }

    /* Check for Type Name Format  depending on the TNF,  Type Length value is set*/
    if ((*Record & MTK_NFC_NDEFRECORD_TNFBYTE_MASK)==
        MTK_NFC_NDEFRECORD_TNF_EMPTY)
    {
        *TypeLength = *(Record + MTK_NFC_NDEFRECORD_BUF_INC1);

        if (*(Record + MTK_NFC_NDEFRECORD_BUF_INC1) != 0)
        {
            /* Type Length  Error */
            Status = -1;
            return Status;
        }

        *TypeLengthByte = 1;

        /* Check for Short Record */
        if ((*Record & MTK_NFC_NDEFRECORD_FLAGS_SR) == MTK_NFC_NDEFRECORD_FLAGS_SR)
        {
            /* For Short Record, Payload Length Byte is 1 */
            *PayloadLengthByte = 1;
            /*  1 for Header byte */
            *PayloadLength = *(Record + *TypeLengthByte + 1);
            if (*PayloadLength != 0)
            {
                /* PayloadLength  Error */
                Status = -1;
                return Status;
            }
        }
        else
        {
            /* For Normal Record, Payload Length Byte is 4 */
            *PayloadLengthByte = (4);
            *PayloadLength =    ((((UINT32)(*(Record + MTK_NFC_NDEFRECORD_BUF_INC2))) << (24)) +
                                 (((UINT32)(*(Record + MTK_NFC_NDEFRECORD_BUF_INC3))) << (16)) +
                                 (((UINT32)(*(Record + MTK_NFC_NDEFRECORD_BUF_INC4))) << (8))  +
                                 *(Record + MTK_NFC_NDEFRECORD_BUF_INC5));
            if (*PayloadLength != 0)
            {
                /* PayloadLength  Error */
                Status = -1;
                return Status;
            }
        }

        /* Check for ID Length existence */
        if ((*Record & MTK_NFC_NDEFRECORD_FLAGS_IL) == MTK_NFC_NDEFRECORD_FLAGS_IL)
        {
            /* Length Byte exists and it is 1 byte */
            *IDLengthByte = 1;
            /*  1 for Header byte */
            *IDLength = (UINT8)*(Record + *PayloadLengthByte + *TypeLengthByte + MTK_NFC_NDEFRECORD_BUF_INC1);
            if (*IDLength != 0)
            {
                /* IDLength  Error */
                Status = -1;
                return Status;
            }
        }
        else
        {
            *IDLengthByte = 0;
            *IDLength = 0;
        }
    }
    else
    {
        if ((*Record & MTK_NFC_NDEFRECORD_TNFBYTE_MASK)== MTK_NFC_NDEFRECORD_TNF_UNKNOWN
            || (*Record & MTK_NFC_NDEFRECORD_TNFBYTE_MASK) ==
            MTK_NFC_NDEFRECORD_TNF_UNCHANGED)
        {
            if (*(Record + MTK_NFC_NDEFRECORD_BUF_INC1) != 0)
            {
                /* Type Length  Error */
                Status = -1;
                return Status;
            }
            *TypeLength = 0;
            *TypeLengthByte = 1;
        }
        else
        {
            /*  1 for Header byte */
            *TypeLength = *(Record + MTK_NFC_NDEFRECORD_BUF_INC1);
            *TypeLengthByte = 1;
        }

        /* Check for Short Record */
        if ((*Record & MTK_NFC_NDEFRECORD_FLAGS_SR) ==
            MTK_NFC_NDEFRECORD_FLAGS_SR)
        {
            /* For Short Record, Payload Length Byte is 1 */
            *PayloadLengthByte = 1;
            /*  1 for Header byte */
            *PayloadLength = *(Record + *TypeLengthByte + MTK_NFC_NDEFRECORD_BUF_INC1);
        }
        else
        {
            /* For Normal Record, Payload Length Byte is 4 */
            *PayloadLengthByte = (4);
            *PayloadLength =    ((((UINT32)(*(Record + MTK_NFC_NDEFRECORD_BUF_INC2))) << (24)) +
                                 (((UINT32)(*(Record + MTK_NFC_NDEFRECORD_BUF_INC3))) << (16)) +
                                 (((UINT32)(*(Record + MTK_NFC_NDEFRECORD_BUF_INC4))) << (8))  +
                                 *(Record + MTK_NFC_NDEFRECORD_BUF_INC5));
        }

        /* Check for ID Length existence */
        if ((*Record & MTK_NFC_NDEFRECORD_FLAGS_IL) ==
            MTK_NFC_NDEFRECORD_FLAGS_IL)
        {
            *IDLengthByte = 1;
            /*  1 for Header byte */
            *IDLength = (UINT8)*(Record + *PayloadLengthByte + *TypeLengthByte + MTK_NFC_NDEFRECORD_BUF_INC1);
        }
        else
        {
            *IDLengthByte = 0;
            *IDLength = 0;
        }
    }
    return Status;
}

INT32 mtk_nfchod_NdefRecord_Generate(mtk_nfcho_ndefrecord_t *Record,
                                     UINT8          *Buffer,
                                     UINT32           MaxBufferSize,
                                     UINT32           *BytesWritten)
{
    UINT8   FlagCheck=0, TypeCheck=0, *temp = NULL, i = 0;
    UINT32    i_data=0;

    if(Record==NULL ||Buffer==NULL||BytesWritten==NULL||MaxBufferSize == 0)
    {
        return (-1);
    }

    if (Record->Tnf == MTK_NFC_NDEFRECORD_TNF_RESERVED)
    {
        return (-1);
    }

    /* calculate the length of the record and check with the buffersize if it exceeds return */
    i_data = mtk_nfchod_NdefRecord_GetLength(Record);
    if(i_data > MaxBufferSize)
    {
        return (-1);
    }
    *BytesWritten = i_data;

    /*fill the first byte of the message(all the flags) */
    /*increment the buffer*/
    *Buffer = ( (Record->Flags & MTK_NFC_NDEFRECORD_FLAG_MASK) | (Record->Tnf & MTK_NFC_NDEFRECORD_TNFBYTE_MASK));
    Buffer++;

    /* check the TypeNameFlag for PH_FRINFC_NDEFRECORD_TNF_EMPTY */
    FlagCheck = mtk_nfchod_NdefRecord_NdefFlag(Record->Tnf,MTK_NFC_NDEFRECORD_TNFBYTE_MASK);
    if(FlagCheck == MTK_NFC_NDEFRECORD_TNF_EMPTY)
    {
        /* fill the typelength idlength and payloadlength with zero(empty message)*/
        for(i=0; i<3; i++)
        {
            *Buffer=MTK_NFC_NDEFRECORD_BUF_TNF_VALUE;
            Buffer++;
        }
        return (0);
    }

    /* check the TypeNameFlag for PH_FRINFC_NDEFRECORD_TNF_RESERVED */
    /* TNF should not be reserved one*/
    FlagCheck = mtk_nfchod_NdefRecord_NdefFlag(Record->Tnf,MTK_NFC_NDEFRECORD_TNFBYTE_MASK);
    if(FlagCheck == MTK_NFC_NDEFRECORD_TNF_RESERVED)
    {
        return (-1);
    }

    /* check for TNF Unknown or Unchanged */
    FlagCheck = mtk_nfchod_NdefRecord_NdefFlag(Record->Tnf,MTK_NFC_NDEFRECORD_TNFBYTE_MASK);
    if(FlagCheck == MTK_NFC_NDEFRECORD_TNF_UNKNOWN || \
       FlagCheck == MTK_NFC_NDEFRECORD_TNF_UNCHANGED)
    {
        *Buffer = MTK_NFC_NDEFRECORD_BUF_TNF_VALUE;
        Buffer++;
    }
    else
    {
        *Buffer = Record->TypeLength;
        Buffer++;
        TypeCheck=1;
    }

    /* check for the short record bit if it is then payloadlength is only one byte */
    FlagCheck = mtk_nfchod_NdefRecord_NdefFlag(Record->Flags,MTK_NFC_NDEFRECORD_FLAGS_SR);
    if(FlagCheck!=0)
    {
        *Buffer = (UINT8)(Record->PayloadLength & 0x000000ff);
        Buffer++;
    }
    else
    {
        /* if it is normal record payloadlength is 4 byte(32 bit)*/
        *Buffer = (UINT8)((Record->PayloadLength & 0xff000000) >> (24));
        Buffer++;
        *Buffer = (UINT8)((Record->PayloadLength & 0x00ff0000) >> (16));
        Buffer++;
        *Buffer = (UINT8)((Record->PayloadLength & 0x0000ff00) >> (8));
        Buffer++;
        *Buffer = (UINT8)((Record->PayloadLength & 0x000000ff));
        Buffer++;
    }

    /*check for IL bit set(Flag), if so then IDlength is present*/
    FlagCheck = mtk_nfchod_NdefRecord_NdefFlag(Record->Flags,MTK_NFC_NDEFRECORD_FLAGS_IL);
    if(FlagCheck!=0)
    {
        *Buffer=Record->IdLength;
        Buffer++;
    }

    /*check for TNF and fill the Type*/
    temp=Record->Type;
    if(TypeCheck!=0)
    {
        for(i=0; i<(Record->TypeLength); i++)
        {
            *Buffer = *temp;
            Buffer++;
            temp++;
        }
    }

    /*check for IL bit set(Flag), if so then IDlength is present and fill the ID*/
    FlagCheck = mtk_nfchod_NdefRecord_NdefFlag(Record->Flags,MTK_NFC_NDEFRECORD_FLAGS_IL);
    temp=Record->Id;
    if(FlagCheck!=0)
    {
        for(i=0; i<(Record->IdLength); i++)
        {
            *Buffer = *temp;
            Buffer++;
            temp++;
        }
    }

    temp=Record->PayloadData;
    /*check for SR bit and then correspondingly use the payload length*/
    FlagCheck = mtk_nfchod_NdefRecord_NdefFlag(Record->Flags,MTK_NFC_NDEFRECORD_FLAGS_SR);
    for(i_data=0; i_data < (Record->PayloadLength) ; i_data++)
    {
        *Buffer = *temp;
        Buffer++;
        temp++;
    }

    return (0);
}


INT32 mtk_nfchod_NdefRecord_Parse(mtk_nfcho_ndefrecord_t* Record,
                                  UINT8*   RawRecord)
{
    INT32 Status = 0;
    UINT8   PayloadLengthByte = 0,
            TypeLengthByte = 0,
            TypeLength = 0,
            IDLengthByte = 0,
            IDLength = 0,
            Tnf     =   0;
    UINT32    PayloadLength = 0;

    //  mtk_nfcho_service_dbg_output("mtk_nfchod_NdefRecord_Parse");

    if (Record == NULL || RawRecord == NULL)
    {
        Status = (-1);
    }

    else
    {

        /* Calculate the Flag Value */
        Record->Flags = mtk_nfchod_NdefRecord_RecordFlag ( RawRecord);

        /* Calculate the Type Namr format of the record */
        Tnf = mtk_nfchod_NdefRecord_TypeNameFormat( RawRecord);
        if(Tnf != 0xFF)
        {
            Record->Tnf = Tnf;
            /* To Calculate the IDLength and PayloadLength for short or normal record */
            Status = mtk_nfchod_NdefRecord_RecordIDCheck ( RawRecord,
                     &TypeLength,
                     &TypeLengthByte,
                     &PayloadLengthByte,
                     &PayloadLength,
                     &IDLengthByte,
                     &IDLength);
            Record->TypeLength = TypeLength;
            Record->PayloadLength = PayloadLength;
            Record->IdLength = IDLength;
            RawRecord = (RawRecord +  PayloadLengthByte + IDLengthByte + TypeLengthByte + MTK_NFC_NDEFRECORD_BUF_INC1);
            Record->Type = RawRecord;

            RawRecord = (RawRecord + Record->TypeLength);

            if (Record->IdLength != 0)
            {
                Record->Id = RawRecord;
            }

            RawRecord = RawRecord + Record->IdLength;
            Record->PayloadData = RawRecord;
            // mtk_nfcho_service_dbg_output("mtk_nfchod_NdefRecord_Parse,0x%x", Record->PayloadData[0] );
        }
        else
        {
            Status = -1;
        }
    }
    return Status;
}



