#ifndef __RCSERVER__H__
    #define __RCSERVER__H__

#include <stdint.h>

//#define _TESTLOOP

/*---------------------------Opciones de Compilación -----------------------------------------------------------*/
#define _MAXCALENDARLST 5
//#define _OPT_FIRMWARE_UPDATE_ENABLE         // Habilita la actualización de firmware de los dispositivos NetHub
//#define _TEST_DB

//#define _TEST_DB_DEVSTATE
//#define _TEST_DB_DEVEVENT
//#define _TEST_DB_VERSION
//#define _TEST_DB_ALARM
//#define _TEST_DB_ALARM_INSERT
//#define _TEST_DB_DEVICES
/*--------------------------------------------------------------------------------------------------------------*/

#define _VERSION            "0.3"
#define _LOGOUTPUTNAME      "./logs/initlog.log"
#define DEST_DNS_NAME       "redcamserver.ddns.net"
#define _LENFILENAME        (23 + 1) 
#define DEST_PORT           5000
#define _RXBUFFER_SIZE      1350//1350 //(1458 es el máximo, pero hay que tener en cuenta el header the IP viene con opciones)
#define _TMAXDBREAD         3000//1000    // base 0,01Seg
#define _MAXVFILETIMEOUT    30        // Base 1Seg (Max 255)
//#define _MAXFRAMEFWUPDATE  1024    
//#define _MAXFRAMEFWUPDATE  128    
#define _MAXFRAMEFWUPDATE  64    

//#define _LOCAL_PATH_VIDEOS  "~/_presunciones"
//#define _LOCAL_PATH_VIDEOS  "/home/scallozzo/presunciones"
#define _AUTODECRYPTION

#define _BULKMODE
#define _BULK_SIZE          10
#define _STSIZE             60

#define _DEBUG_RCSERVER


typedef enum
{
	BIT0	= 0x00000001,
	BIT1	= 0x00000002,
	BIT2	= 0x00000004,
	BIT3	= 0x00000008,
	BIT4 	= 0x00000010,
	BIT5	= 0x00000020,
	BIT6	= 0x00000040,
	BIT7	= 0x00000080,
	BIT8	= 0x00000100,
	BIT9	= 0x00000200,
	BIT10	= 0x00000400,
	BIT11	= 0x00000800,
	BIT12	= 0x00001000,
	BIT13	= 0x00002000,
	BIT14	= 0x00004000,
	BIT15	= 0x00008000,
	BIT16	= 0x00010000,
	BIT17	= 0x00020000,
	BIT18	= 0x00040000,
	BIT19	= 0x00080000,
	BIT20	= 0x00100000,
	BIT21	= 0x00200000,
	BIT22	= 0x00400000,
	BIT23	= 0x00800000,
	BIT24	= 0x01000000,
	BIT25	= 0x02000000,
	BIT26	= 0x04000000,
	BIT27	= 0x08000000,
	BIT28	= 0x10000000,
	BIT29	= 0x20000000,
	BIT30	= 0x40000000,
	BIT31	= 0x80000000
}ebitnr;

typedef enum
{
    //----------RC Cmds------------------------//
    RC_CMD_STATUS       = 0x01,
    RC_CMD_VIDEOFILE    = 0x02,
    //----------LT Cmds------------------------//
    LT_CMD_HUB_STATUS   = 0x20,
    LT_CMD_STATUS       = 0x21,
    LT_CMD_FW_FRAME     = 0x22,
    LT_CMD_MDX_CFG      = 0x23,         // Llega como respuesta de hubstatus cuando hay nueva configuración
    //----------LT Server side Cmds------------//
    LT_CMD_SERVERSIDE   = 0x40,  
    //----------RC Server side-----------------//
    RC_CMD_SERVERSIDE   = 0x80
}erccmds;

typedef enum
{
    SS_STS_ONLINE               = BIT0,
    SS_STS_ERR_NO_ALTA          = BIT1,
    SS_STS_ERR_EQ_DUPLICADO     = BIT2,
    SSTATUS_STS_res3            = BIT3,
    SSTATUS_STS_res4            = BIT4,
    SSTATUS_STS_res5            = BIT5,
    SSTATUS_STS_DMX_ENABLE      = BIT6,
    SSTATUS_STS_FWUPDATE_ENABLE = BIT7    
}sstatus;


typedef enum
{
    LBLOCK_INIT = 0,
    LBLOCK_FIRST,
    LBLOCK_FIRST_BULK,
    LBLOCK_TXNOACK,
    LBLOCK_TXBULK_ACK,
    LBLOCK_CLOSE
}elblock;


typedef struct
{
    uint8_t mainsts;
    uint16_t dbreadtim;    
}stRCServer;


typedef struct
{
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t date;
    uint8_t month;
    uint8_t year;
}stRTC;


typedef struct __attribute__((packed))
{
    uint8_t Cmd;
    uint32_t Seq;
    uint8_t  EqID[10];
    stRTC    FyH;
    uint8_t EqStatus;
    uint8_t EqErrsts;
    uint16_t TCh1;
    uint16_t TCh2;
    uint16_t TotalVid;
    uint16_t PendingTxVid;
    uint8_t  RestartCnt;
    uint32_t TimeRunning;
    uint16_t Crc;
}stRxStatus;

typedef struct __attribute__((packed))
{
    uint8_t Cmd;
    uint32_t Seq;
    uint8_t SStatus;
    uint8_t SRequest;
    uint16_t Crc;
}stTxStatus;

typedef struct __attribute__((packed))
{
    uint8_t Cmd;
    uint32_t Seq;
    uint8_t EqID[10];
    uint8_t FName[_LENFILENAME];
    uint32_t FLen;
    uint32_t TxfrId;
    uint32_t Boffset;
    uint16_t BSize;
    uint8_t LBlock;
    uint32_t BlockNr;
    uint16_t FileCRC;
    uint8_t  BBuffer[_RXBUFFER_SIZE - _STSIZE - sizeof(uint16_t)]; // RXBUFFER - _STSIZE = (sizeof of this structure)
    uint16_t Crc;
}stRxVFile;

typedef struct __attribute__((packed))
{
    uint8_t Cmd;
    uint32_t Seq;
    uint8_t EqID[10];
    uint8_t FName[_LENFILENAME];
    uint32_t Boffset;
    uint8_t FStatus;
    uint32_t TxfrId;
    uint32_t BlockNr;
    uint16_t FileCRC;
    uint16_t Crc;
}stTxVFile;

typedef struct __attribute__((packed))
{
    uint8_t Status;
    uint8_t EqID[10];
    uint8_t WTimer;
#ifdef _BULKMODE
    uint8_t BulkBuffer[_BULK_SIZE][_RXBUFFER_SIZE - _STSIZE - sizeof(uint16_t)];
#endif
    stTxVFile TxVFile;
}stBTxVFile;

//------------------------------------------------- LT -----------------------------------------------------------
typedef struct {
    uint8_t sec;    // 0-59
    uint8_t min;    // 0-59
    uint8_t hour;   // 0-23
    uint8_t day;    // 1-31
    uint8_t month;  // 1-12
    uint16_t year;  // ej: 2026
} rtc_soft_t;

typedef enum
{
    RTC_WEEKDAY_SUNDAY = 0,
    RTC_WEEKDAY_MONDAY,
    RTC_WEEKDAY_TUESDAY,
    RTC_WEEKDAY_WEDNESDAY,
    RTC_WEEKDAY_THURSDAY,
    RTC_WEEKDAY_FRIDAY,
    RTC_WEEKDAY_SATURDAY
} rtc_weekday_t;


// Estructuras para hub-status
typedef struct __attribute__((packed))
{
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint16_t Seq;
    uint8_t HubID[6];
    uint8_t HubStatus;
    uint8_t HubErrsts;
    uint8_t HubEvent;   // <> 0 = Event
    uint32_t TimeRunning;

    int32_t latitude_e7;
    int32_t longitude_e7;
    rtc_soft_t rtc;
  
    uint8_t dmxseq;

    uint16_t FwVersion;
    uint16_t Crc;
}stRxLTHubStatus;



// Estructura de respuesta para HubStatus (podría devolver configuración?)
typedef struct __attribute__((packed))
{
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint32_t Seq;
    uint8_t SStatus;
    uint8_t SRequest;
    uint8_t SectorID;
    uint8_t DevAttached;
    uint8_t DevDisabled;
    uint8_t DevbitList[13];
    uint8_t HubVer[3];                   // Versión actual de NetHub (para actualizar)
    uint8_t LTVer[3];                   // Versión actual de LTX (para actualizar)

    uint8_t TxConfig;                   // Tiempo expresado en segundos para la transmisión de hubstatus
    rtc_soft_t rtc;                     // RTC propuesto
    
    uint16_t Crc;
}stTxLTHubStatus;


typedef struct
{
    uint8_t  enabled;

    uint8_t  start_hour;
    uint8_t  start_minute;
    uint8_t  end_hour;
    uint8_t  end_minute;

    uint8_t  days_mask;

    uint8_t action;

    uint8_t  r_g1;
    uint8_t  g_g1;
    uint8_t  b_g1;
    uint8_t  r_g2;
    uint8_t  g_g2;
    uint8_t  b_g2;
    uint8_t  r_g3;
    uint8_t  g_g3;
    uint8_t  b_g3;

    uint8_t  dimming;

} stCalendarEvent;


// Estructura de respuesta para LT_CMD_MDX_CFG
typedef struct __attribute__((packed))
{
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint32_t Seq;
    uint8_t MdxSeq;
    stCalendarEvent CalendarList[_MAXCALENDARLST];
    uint16_t Crc;
}stTxLTMdxCfg;


typedef struct __attribute__((packed))
{
    // st 2
    uint32_t CDaT;
    uint8_t free[3];
    uint8_t EqStatus; 
    uint8_t EqErrsts;
    uint8_t LowerVoltage;
    uint8_t PeakVoltage;
    uint8_t MainRestarts;
    uint32_t TimeCPURunning;
    uint16_t FwVersion;
}stRxLTDatast2;

typedef struct __attribute__((packed))
{
    uint8_t  currSTS;          // Estado actual 
    uint8_t  currV;            // Tensión actual
    uint16_t currI;            // Corriente actual + cospi
    uint8_t  TopV;             // Tensión máxima detectada
    uint8_t  LowerV;           // Tensión mínima detectada
    uint32_t timerunning;      // Tiempo de funcionamiento de las luminarias (Expresado en segundos)
    uint32_t t_01Wh;           // Para Tx 32bits (01Wh) (para convertir a Wh = 01Wh x 0.1)
    uint32_t r_01Wh;           // Para Tx 32bits (01Wh)
}stRxLTDatast1;


typedef struct __attribute__((packed))
{
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint16_t Seq;
    uint8_t HubID[6];
    uint8_t Group;
    uint8_t ListEqs;    // Cantidad de equipos listados 
/*-----------------Estructura de Equipos -----------------------------------*/
//stGroupStatus GroupStatus[25];
/*--------------------------------------------------------------------------*/    
//    uint16_t Crc;
}stRxLTStatus;


typedef struct __attribute__((packed))
{
    // terminar!!!
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint32_t Seq;
    uint8_t SStatus;
    uint8_t SRequest;
    uint16_t Crc;
}stTxLTStatus;


typedef struct __attribute__((packed))
{
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint8_t HubID[6];
    uint16_t framenr;
    uint8_t ver[3];
    uint16_t Crc;    
}stRxLTFwFrame;

typedef struct __attribute__((packed))
{
    uint8_t flag;
    uint16_t len;
    uint8_t Cmd;
    uint8_t HubID[6];
    uint8_t ver[3];
    uint16_t framesize;
    uint16_t framenr;
    uint16_t totframes;
    uint8_t buffer[_MAXFRAMEFWUPDATE];
    uint16_t Crc;    
}stTxLTFwFrame;

typedef enum
{
    HUB_STS_GNSS_RDY            = BIT0,
    HUB_STS_DTIME_SYNCRO_OK     = BIT1,
    HUB_STS_res2                = BIT2,
    HUB_STS_res3                = BIT3,
    HUB_STS_res4                = BIT4,
    HUB_STS_res5                = BIT5,
    HUB_STS_COM_SYNCHRONIZED    = BIT6,
    HUB_STS_DMX_ENABLED         = BIT7
}ehubstatus_t;


unsigned short CalcCrc16(unsigned char *pdata, unsigned short lg, unsigned short seed);
uint16_t crc_ccitt(const uint8_t *data, size_t len);
int _GetTimer1ms(void);
void _log(const char *format, ...); 




#endif
