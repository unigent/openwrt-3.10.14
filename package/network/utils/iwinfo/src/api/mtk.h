#ifndef __MTK_H__
#define __MTK_H__

#define MAC_ADDR_LENGTH		6
#define MAX_NUMBER_OF_MAC	32
typedef unsigned char 	UCHAR;
typedef char		CHAR;
typedef unsigned int	UINT32;
typedef unsigned short	USHORT;
typedef short		SHORT;
typedef unsigned long	ULONG;

#if WIRELESS_EXT <= 11
#ifndef SIOCDEVPRIVATE
#define SIOCDEVPRIVATE                             	 0x8BE0
#endif
#define SIOCIWFIRSTPRIV                                  SIOCDEVPRIVATE
#endif

#define RTPRIV_IOCTL_GET_MAC_TABLE                       (SIOCIWFIRSTPRIV + 0x0F)
#define RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT                (SIOCIWFIRSTPRIV + 0x1F)        /* modified by Red@Ralink, 2009/09/30 */


/* MIMO Tx parameter, ShortGI, MCS, STBC, etc.  these are fields in TXWI. Don't change this definition!!! */
typedef union _MACHTTRANSMIT_SETTING {
        struct {
                USHORT MCS:7;   /* MCS */
                USHORT BW:1;    /*channel bandwidth 20MHz or 40 MHz */
                USHORT ShortGI:1;
                USHORT STBC:2;  /*SPACE */
                USHORT rsv:3;
                USHORT MODE:2;  /* Use definition MODE_xxx. */
        } field;
        USHORT word;
} MACHTTRANSMIT_SETTING, *PMACHTTRANSMIT_SETTING;

typedef struct _RT_802_11_MAC_ENTRY {
        UCHAR ApIdx;
        UCHAR Addr[MAC_ADDR_LENGTH];
        UCHAR Aid;
        UCHAR Psm;              /* 0:PWR_ACTIVE, 1:PWR_SAVE */
        UCHAR MimoPs;           /* 0:MMPS_STATIC, 1:MMPS_DYNAMIC, 3:MMPS_Enabled */
        CHAR AvgRssi0;
        CHAR AvgRssi1;
        CHAR AvgRssi2;
        UINT32 ConnectedTime;
        MACHTTRANSMIT_SETTING TxRate;
        UINT32          LastRxRate;
        SHORT           StreamSnr[3];                           /* BF SNR from RXWI. Units=0.25 dB. 22 dB offset removed */
        SHORT           SoundingRespSnr[3];                     /* SNR from Sounding Response. Units=0.25 dB. 22 dB offset removed */
/*      SHORT           TxPER;  */                                      /* TX PER over the last second. Percent */
/*      SHORT           reserved;*/
} RT_802_11_MAC_ENTRY, *PRT_802_11_MAC_ENTRY;

typedef struct _RT_802_11_MAC_TABLE {
        ULONG Num;
        RT_802_11_MAC_ENTRY Entry[MAX_NUMBER_OF_MAC];
} RT_802_11_MAC_TABLE, *PRT_802_11_MAC_TABLE;


#endif // __MTK_H__

