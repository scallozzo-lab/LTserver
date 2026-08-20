//------------------------------------------------------------------ RCSERVER ----------------------------------------------------------------//
/*
 Servidor LTserver  
 Autor: SCALLOZZO                                                                 
  
    Ver 0.3 [19.08.2026] SCallozzo
    * Se comienza a agregar lectura y escritura de tablas

    Ver 0.2 [26.09.2025] SCallozzo
    * Se comienza adaptación de lectura de tablas (sector + devices)

    Ver 0.1 [25.07.2025] SCallozzo
    * Se migra version desde REDCAM
*/                                                                                              
//-------------------------------------------------------------------------------------------------------------------------------------------//

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include "rcserver.h"
#include "storefiles.h"
#include "db.h"
#include "loop2app.h"
#include "nethubbin.h"

int sockfd = 0;
stRCServer RCServer;
struct sockaddr_in servaddr, cliaddr;
static unsigned char RxBuffer[_RXBUFFER_SIZE];
static stBTxVFile BTxVFile[_MAXSTOREV];


int _GetTimer1ms(void)
{
    return clock() / (CLOCKS_PER_SEC / 1000);
}

// Signal handler for Ctrl-C (SIGINT)
void sigintHandler(int sig_num) 
{
    printf("Exiting...\n");
    close(sockfd);
    exit(EXIT_SUCCESS);
}

void _initlog(void)
{
    // Open the file in append mode
    FILE *file = fopen(_LOGOUTPUTNAME, "w");

    if (file == NULL) 
    {
        printf("[_initlog] Error opening file %s for appending!\n", _LOGOUTPUTNAME);
        return;
    }
    // Close the file
    else fclose(file);
}

void _log(const char *format, ...) 
{
    // Open the file in append mode
    FILE *file = fopen(_LOGOUTPUTNAME, "a");

    if (file == NULL) 
    {
        printf("[_log] Error opening file %s for appending!\n", _LOGOUTPUTNAME);
        return;
    }

    // Get current date and time
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Print formatted date and time to the file
    fprintf(file, "[%s] ", buffer);

    // Initialize the argument list
    va_list args;
    va_start(args, format);

    // Print formatted data to the file
    vfprintf(file, format, args);

    // Clean up the argument list
    va_end(args);

    // Close the file
    fclose(file);
}


void printIPAddress(struct sockaddr* sa) 
{
    char ip[INET6_ADDRSTRLEN];
    
    if (sa->sa_family == AF_INET) {
        struct sockaddr_in* sa_in = (struct sockaddr_in*)sa;
        inet_ntop(AF_INET, &(sa_in->sin_addr), ip, INET_ADDRSTRLEN);
    } else if (sa->sa_family == AF_INET6) {
        struct sockaddr_in6* sa_in6 = (struct sockaddr_in6*)sa;
        inet_ntop(AF_INET6, &(sa_in6->sin6_addr), ip, INET6_ADDRSTRLEN);
    } else {
        fprintf(stderr, "Unsupported address family\n");
        return;
    }
    printf("Resolved IP Address: %s\n", ip);
}

stTxVFile *_Look4BTxFree(uint8_t *eq, int *_idx)
{
    static uint8_t _Eq0[sizeof(BTxVFile[0].EqID)] = {0};

    for(int idx = 0; idx < _MAXSTOREV; idx++)    
        if(!memcmp(BTxVFile[idx].EqID, _Eq0, sizeof(BTxVFile[idx].EqID)))
        {    
            memcpy(BTxVFile[idx].EqID, eq, sizeof(BTxVFile[idx].EqID));
            BTxVFile[idx].Status |= 1;
            BTxVFile[idx].TxVFile.Seq = 0xfffffffelu;
            if(_idx) *_idx = idx;
            return &BTxVFile[idx].TxVFile;    
        }
    return 0;
}

stTxVFile *_Look4BTxPos(uint8_t *eq, uint32_t *_idx)
{
    for(uint32_t idx = 0; idx < _MAXSTOREV; idx++)    
        if(!memcmp(BTxVFile[idx].EqID, eq, sizeof(BTxVFile[idx].EqID)))
        {    
            if(_idx) *_idx = idx;
            return &BTxVFile[idx].TxVFile;    
        }
    return 0;
}

void _Look4BTxRelease(uint32_t idx)
{
    if(idx < _MAXSTOREV)
    {
        memset(BTxVFile[idx].EqID, 0, sizeof(BTxVFile[idx].EqID));
        BTxVFile[idx].Status &= ~1;
    }
}


int _InitRCServer(void)
{
    // Set up signal handler for Ctrl-C
    signal(SIGINT, sigintHandler);
    
    _initlog();
    printf("RCServer Version %s\n\r", _VERSION);
    _log("RCServer Version %s\n\r",_VERSION);

    // Abre un socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) 
    {
        _log("[_InitRCServer] Falla Creación Socket\n\r");
        perror("[_InitRCServer] Falla Creación Socket\n");
        exit(EXIT_FAILURE);
    }

    memset(&RCServer, 0, sizeof(RCServer));
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    memset(BTxVFile, 0, sizeof(BTxVFile));
    
    _InitStoreFiles();  
    _LoadNetHubBin();
    
#ifdef _TESTLOOP
    init_loop2app();
    test_loop();
#endif

    int r1;
    
    while(r1 = _Init_dbread()) 
    {
        if(r1 == _DB_STS_ERR_DBLAYOUT)
        {
            printf("[_InitRCServer] Error Initialización DB (Layout)\n");    
            _log("[_InitRCServer] Error Initialización DB (Layout)\n");
        }
        else if(r1 == _DB_STS_ERR_CONN)
        {
            printf("[_InitRCServer] Error Conexión DB\n");    
            _log("[_InitRCServer] Error Conexión DB\n");
        }
        else
        {
            printf("[_InitRCServer] Error Acceso DB\n");    
            _log("[_InitRCServer] Error Acceso DB\n");
        }
        sleep(10);
    }
    
    CalcCrc16(0, 0, 0);
    
    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(DEST_PORT);

    // Bind the socket with the server address
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) 
    {
        _log("[_InitRCServer] bind failed port:%d\n\r", DEST_PORT); 
        perror("[_InitRCServer] bind failed");
        exit(EXIT_FAILURE);
    }
    return 0;
}

int _Send2EQ(struct sockaddr_in *txaddr, uint8_t *txbuf, uint16_t txlen)
{
    //dest_addr.sin_port = htons(DEST_PORT);  // DNS port

    //dest_addr.sin_port = htons(cant);  // DNS port
   
    // Send the UDP packet
    if (sendto(sockfd, txbuf, txlen, 0, (struct sockaddr*)txaddr, sizeof(struct sockaddr)) == -1) 
    {
        perror("[_Send2EQ] Falla Envío\n");
        //close(sockfd);
        //exit(EXIT_FAILURE);
        return 1;
    }
    else return 0;
}

void _ProcIDLE(void)
{
    
}

void _ProcRx(struct sockaddr_in *rxaddr, uint8_t *Rxbuffer, uint16_t RxLen)
{
    // Extract the IP address
    int8_t ip_address[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(rxaddr->sin_addr), ip_address, INET_ADDRSTRLEN);
    printf("[_ProcRx] RxAddress: %s\n", ip_address);
    uint32_t *_SrcIp = (uint32_t*)&(rxaddr->sin_addr);
    
    switch(Rxbuffer[3])
    {
        case LT_CMD_HUB_STATUS:
        {
            stRxLTHubStatus *p_stRxHubStatus = (stRxLTHubStatus *)Rxbuffer;
            stTxLTHubStatus TxHubStatus = {0};

#ifdef _DEBUG_RCSERVER
            printf("[_ProcRx] - LT_CMD_HUB_STATUS\n");
            printf("p_stRxHubStatus->Seq %02X\n", p_stRxHubStatus->Seq);
            printf("p_stRxHubStatus->HubID %02X%02X%02X%02X%02X%02X\n", 
                                                                        p_stRxHubStatus->HubID[0],
                                                                        p_stRxHubStatus->HubID[1],
                                                                        p_stRxHubStatus->HubID[2],
                                                                        p_stRxHubStatus->HubID[3],
                                                                        p_stRxHubStatus->HubID[4],
                                                                        p_stRxHubStatus->HubID[5]);
            
            printf("p_stRxHubStatus->HubStatus %02X\n", p_stRxHubStatus->HubStatus);
            printf("p_stRxHubStatus->HubErrsts %02X\n", p_stRxHubStatus->HubErrsts);
            printf("p_stRxHubStatus->HubEvent %02X\n", p_stRxHubStatus->HubEvent);    
            printf("p_stRxHubStatus->TimeRunning (%d)Seg\n", p_stRxHubStatus->TimeRunning);
            printf("p_stRxHubStatus->FwVersion %04X\n", p_stRxHubStatus->FwVersion);
            printf("p_stRxHubStatus->Crc %04X\n", p_stRxHubStatus->Crc);
#endif            
        
            TxHubStatus.flag = 0xA5;
            TxHubStatus.len = sizeof(TxHubStatus);
            TxHubStatus.Cmd = Rxbuffer[3] | RC_CMD_SERVERSIDE;
            TxHubStatus.Seq = p_stRxHubStatus->Seq;
            TxHubStatus.SStatus = _CheckHubId(p_stRxHubStatus->HubID, sizeof(p_stRxHubStatus->HubID));
            
#ifdef _OPT_FIRMWARE_UPDATE_ENABLE
            TxHubStatus.SStatus |= SSTATUS_STS_FWUPDATE_ENABLE;
#endif            
            
            TxHubStatus.SRequest = 0;
            
            // ******* tomar desde base de datos *****
            TxHubStatus.DevbitList[0] = 0xff;
            TxHubStatus.DevbitList[0] = 0x1;
            
            TxHubStatus.DevAttached = 1;
            
            // Agrega la versión actual del binario para actualizar
            if(_GetNetHubFileInfo())
            {
                TxHubStatus.HubVer[0] = _GetNetHubFileInfo()->ver[0];      
                TxHubStatus.HubVer[1] = _GetNetHubFileInfo()->ver[1];      
                TxHubStatus.HubVer[2] = _GetNetHubFileInfo()->ver[2];
            }

            //TxHubStatus.Crc = CalcCrc16((uint8_t*)&TxHubStatus, sizeof(TxHubStatus) - sizeof(TxHubStatus.Crc), 0);
            TxHubStatus.Crc = crc_ccitt((uint8_t*)&TxHubStatus, sizeof(TxHubStatus) - sizeof(TxHubStatus.Crc));
            
            if(_Send2EQ(rxaddr, (uint8_t*)&TxHubStatus, sizeof(TxHubStatus)))
            {
                _log("[_ProcRx] ERROR-> Tx LT_CMD_HUB_STATUS a LT IP:%s\n\r", ip_address);
            }
#ifdef _DEBUG_RCSERVER            
            else
            {
                printf("[_ProcRx] - Tx-> LT_CMD_HUB_STATUS\n");
            }
#endif
        }
        break;

        case LT_CMD_STATUS:
        {
            stRxLTStatus *p_stRxLTStatus = (stRxLTStatus *)Rxbuffer;
         
#ifdef _DEBUG_RCSERVER
            printf("[_ProcRx] - LT_CMD_HUB_STATUS\n");

            printf("flag = %02X\n", p_stRxLTStatus->flag);
            printf("len = (%d)\n", p_stRxLTStatus->len);
            printf("Cmd = %02X\n", p_stRxLTStatus->Cmd);
            printf("Seq = (%d)\n", p_stRxLTStatus->Seq);
            printf("HubID %02X%02X%02X%02X%02X%02X\n", 
                                                        p_stRxLTStatus->HubID[0],
                                                        p_stRxLTStatus->HubID[1],
                                                        p_stRxLTStatus->HubID[2],
                                                        p_stRxLTStatus->HubID[3],
                                                        p_stRxLTStatus->HubID[4],
                                                        p_stRxLTStatus->HubID[5]);
            printf("Group = %02X\n", p_stRxLTStatus->Group);
            printf("ListEqs = (%d)\n", p_stRxLTStatus->ListEqs);
            
#endif

        }
        break;

        case LT_CMD_FW_FRAME:
        {      
            stRxLTFwFrame *p_RxLTFwFrame = (stRxLTFwFrame *)Rxbuffer;
            stTxLTFwFrame TxLTFwFrame = {0};
            uint16_t txlen = 0;

#ifdef _DEBUG_RCSERVER
            printf("[_ProcRx] - LT_CMD_FW_FRAME\n");
            printf("p_stRxHubStatus->Len %02X\n", p_RxLTFwFrame->len);
            printf("Cmd = %02X\n", p_RxLTFwFrame->Cmd);
            printf("p_RxLTFwFrame->HubID %02X%02X%02X%02X%02X%02X\n", 
                                                                        p_RxLTFwFrame->HubID[0],
                                                                        p_RxLTFwFrame->HubID[1],
                                                                        p_RxLTFwFrame->HubID[2],
                                                                        p_RxLTFwFrame->HubID[3],
                                                                        p_RxLTFwFrame->HubID[4],
                                                                        p_RxLTFwFrame->HubID[5]);
            printf("framenr = %04X\n", p_RxLTFwFrame->framenr);
            printf("ver = %02X%02X%02X\n", p_RxLTFwFrame->ver[0], p_RxLTFwFrame->ver[1], p_RxLTFwFrame->ver[2]);
#endif

            TxLTFwFrame.flag = 0xA5;
            //TxLTFwFrame.len = sizeof(TxLTFwFrame);
            TxLTFwFrame.Cmd = Rxbuffer[3] | RC_CMD_SERVERSIDE;
            TxLTFwFrame.HubID[0] = 1;
            TxLTFwFrame.HubID[1] = 1;
            TxLTFwFrame.HubID[2] = 1;
            TxLTFwFrame.HubID[3] = 1;
            TxLTFwFrame.HubID[4] = 1;
            TxLTFwFrame.HubID[5] = 1;
            
            TxLTFwFrame.framesize = _MAXFRAMEFWUPDATE;
            
            // Agrega la versión actual del binario para actualizar
            if(_GetNetHubFileInfo() && p_RxLTFwFrame->framenr <= _GetNetHubFileInfo()->totframes)
            {
                TxLTFwFrame.ver[0] = _GetNetHubFileInfo()->ver[0];      
                TxLTFwFrame.ver[1] = _GetNetHubFileInfo()->ver[1];      
                TxLTFwFrame.ver[2] = _GetNetHubFileInfo()->ver[2];
                memcpy(TxLTFwFrame.buffer, &_GetNetHubFileInfo()->filebuffer[p_RxLTFwFrame->framenr * _MAXFRAMEFWUPDATE], sizeof(TxLTFwFrame.buffer));
                TxLTFwFrame.framenr = p_RxLTFwFrame->framenr;
                TxLTFwFrame.totframes = _GetNetHubFileInfo()->totframes;
                txlen = sizeof(TxLTFwFrame);
            }
            // Sino error:
            else 
            {
#ifdef _DEBUG_RCSERVER
                printf("[_ProcRx] ERROR: framenr fuera de rango o no existe binario\n"); 
#endif                
                TxLTFwFrame.framenr = 0xffff;
                txlen = sizeof(TxLTFwFrame) - sizeof(TxLTFwFrame.buffer); 
            }

            TxLTFwFrame.len = txlen;

            TxLTFwFrame.Crc = crc_ccitt((uint8_t*)&TxLTFwFrame, txlen - sizeof(TxLTFwFrame.Crc));
       
            if(_Send2EQ(rxaddr, (uint8_t*)&TxLTFwFrame, txlen))
            {
                _log("[_ProcRx] ERROR-> Tx LT_CMD_FW_FRAME a LT IP:%s\n\r", ip_address);
            }
#ifdef _DEBUG_RCSERVER            
            else
            {
                printf("[_ProcRx] - Tx-> LT_CMD_FW_FRAME\n");
            }
#endif
        }
        break;


        case RC_CMD_STATUS:
        {
            stTxStatus TxStatus;
            stRxStatus *p_stRxStatus = (stRxStatus *)Rxbuffer;

#ifdef _DEBUG_RCSERVER
            printf("Rx Cmd RC_CMD_STATUS\n");
            printf("p_stRxStatus->Seq %04X\n", p_stRxStatus->Seq);
            printf("p_stRxStatus->EqID %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", 
                                                                                    p_stRxStatus->EqID[0],
                                                                                    p_stRxStatus->EqID[1],
                                                                                    p_stRxStatus->EqID[2],
                                                                                    p_stRxStatus->EqID[3],
                                                                                    p_stRxStatus->EqID[4],
                                                                                    p_stRxStatus->EqID[5],
                                                                                    p_stRxStatus->EqID[6],
                                                                                    p_stRxStatus->EqID[7],
                                                                                    p_stRxStatus->EqID[8],
                                                                                    p_stRxStatus->EqID[9]);
            printf("p_stRxStatus->FyH %02d/%02d/%04d %02d:%02d:%02d\n", 
                                                                        p_stRxStatus->FyH.day,
                                                                        p_stRxStatus->FyH.month,
                                                                        p_stRxStatus->FyH.year + 2000,
                                                                        p_stRxStatus->FyH.hours,
                                                                        p_stRxStatus->FyH.minutes,
                                                                        p_stRxStatus->FyH.seconds);
            printf("p_stRxStatus->EqStatus %02X\n", p_stRxStatus->EqStatus);
            printf("p_stRxStatus->EqErrsts %02X\n", p_stRxStatus->EqErrsts);
            printf("p_stRxStatus->TCh1 (%04d)\n", p_stRxStatus->TCh1);
            printf("p_stRxStatus->TCh2 (%04d)\n", p_stRxStatus->TCh2);
            printf("p_stRxStatus->TotalVid (%04d)\n", p_stRxStatus->TotalVid);
            printf("p_stRxStatus->PendigTxVid (%04d)\n", p_stRxStatus->PendingTxVid);
            printf("p_stRxStatus->RestartCnt (%02d)\n", p_stRxStatus->RestartCnt);
            printf("p_stRxStatus->TimeRunning (%d)-%luhs\n", p_stRxStatus->TimeRunning, p_stRxStatus->TimeRunning / 3600lu);
#endif

            TxStatus.Cmd = Rxbuffer[0] | RC_CMD_SERVERSIDE;
            TxStatus.Seq = p_stRxStatus->Seq;
            TxStatus.SStatus = _CheckEq(p_stRxStatus->EqID, sizeof(p_stRxStatus->EqID));
            TxStatus.SRequest = 0;
            TxStatus.Crc = CalcCrc16((uint8_t*)&TxStatus, sizeof(TxStatus) - sizeof(TxStatus.Crc), 0);
            
            if(_Send2EQ(rxaddr, (uint8_t*)&TxStatus, sizeof(TxStatus)))
            {
                _log("[_ProcRx] ERROR-> Tx RC_CMD_STATUS a RedCam IP:%s\n\r", ip_address);
            }
        
#ifdef _DEBUG_RCSERVER
            printf("Rx Cmd RC_CMD_STATUS\n");
            printf("p_stRxStatus->Seq %04X\n", p_stRxStatus->Seq);
            printf("p_stRxStatus->EqID %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", 
                                                                                    p_stRxStatus->EqID[0],
                                                                                    p_stRxStatus->EqID[1],
                                                                                    p_stRxStatus->EqID[2],
                                                                                    p_stRxStatus->EqID[3],
                                                                                    p_stRxStatus->EqID[4],
                                                                                    p_stRxStatus->EqID[5],
                                                                                    p_stRxStatus->EqID[6],
                                                                                    p_stRxStatus->EqID[7],
                                                                                    p_stRxStatus->EqID[8],
                                                                                    p_stRxStatus->EqID[9]);
            printf("p_stRxStatus->FyH %02d/%02d/%04d %02d:%02d:%02d\n", 
                                                                        p_stRxStatus->FyH.day,
                                                                        p_stRxStatus->FyH.month,
                                                                        p_stRxStatus->FyH.year + 2000,
                                                                        p_stRxStatus->FyH.hours,
                                                                        p_stRxStatus->FyH.minutes,
                                                                        p_stRxStatus->FyH.seconds);
            printf("p_stRxStatus->EqStatus %02X\n", p_stRxStatus->EqStatus);
            printf("p_stRxStatus->EqErrsts %02X\n", p_stRxStatus->EqErrsts);
            printf("p_stRxStatus->TCh1 (%04d)\n", p_stRxStatus->TCh1);
            printf("p_stRxStatus->TCh2 (%04d)\n", p_stRxStatus->TCh2);
            printf("p_stRxStatus->TotalVid (%04d)\n", p_stRxStatus->TotalVid);
            printf("p_stRxStatus->PendigTxVid (%04d)\n", p_stRxStatus->PendingTxVid);
            printf("p_stRxStatus->RestartCnt (%02d)\n", p_stRxStatus->RestartCnt);
            printf("p_stRxStatus->TimeRunning (%d)-%luhs\n", p_stRxStatus->TimeRunning, p_stRxStatus->TimeRunning / 3600lu);
#endif
          
            if(TxStatus.SStatus) 
                printf("[_ProcRx] ATENCION-> IDEQ NO encontrado!\n");
        }
        break;
        
        case RC_CMD_VIDEOFILE:
        {
            stTxVFile TxVFile;
            stRxVFile *p_stRxVFile = (stRxVFile *)Rxbuffer;
            int idx = 0;
            
            stTxVFile *bTx = _Look4BTxPos(p_stRxVFile->EqID, &idx);
            if(bTx == 0)
                bTx = _Look4BTxFree(p_stRxVFile->EqID, &idx);
            if(bTx)
            {
                BTxVFile[idx].WTimer = _MAXVFILETIMEOUT; 
#ifndef _BULKMODE                
                if(bTx->Seq == p_stRxVFile->Seq)
                {
                    printf("Reintento RC_CMD_VIDEOFILE Seq (%d)\n", p_stRxVFile->Seq);    
                    if(_Send2EQ(rxaddr, (uint8_t*)bTx, sizeof(TxVFile)));
                    break;
                }    
#endif            
            }
            else 
            {
                printf("[_ProcRx] ERROR Sin Espacio (_Look4BTxFree)\n");                
                _log("[_ProcRx] ERROR Sin Espacio (_Look4BTxFree)\n");                
                break;
            }
#ifdef _DEBUG_RCSERVER    
            printf("RX RC_CMD_VIDEOFILE\n");
            
            printf("Seq = (%d)\n", p_stRxVFile->Seq);
            printf("EqID = %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", 
                                                                        p_stRxVFile->EqID[0],
                                                                        p_stRxVFile->EqID[1],
                                                                        p_stRxVFile->EqID[2],
                                                                        p_stRxVFile->EqID[3],
                                                                        p_stRxVFile->EqID[4],
                                                                        p_stRxVFile->EqID[5],
                                                                        p_stRxVFile->EqID[6],
                                                                        p_stRxVFile->EqID[7],
                                                                        p_stRxVFile->EqID[8],
                                                                        p_stRxVFile->EqID[9]);
            printf("FName = %s\n", p_stRxVFile->FName);
            printf("FLen = (%d)\n", p_stRxVFile->FLen);
            printf("Boffset = %X\n", p_stRxVFile->Boffset);
            printf("BSize = (%d)\n", p_stRxVFile->BSize);
            printf("LBlock = (%d)\n", p_stRxVFile->LBlock);
            printf("TxfrId = %X\n", p_stRxVFile->TxfrId);
            printf("BlockNr = (%d)\n", p_stRxVFile->BlockNr);
            printf("FileCRC = (%04X)\n", p_stRxVFile->FileCRC);
#endif
            TxVFile.Cmd = Rxbuffer[0] | RC_CMD_SERVERSIDE;
            TxVFile.Seq = p_stRxVFile->Seq;
            memcpy((uint8_t*)TxVFile.EqID, p_stRxVFile->EqID, sizeof(TxVFile.EqID));
            memcpy((uint8_t*)TxVFile.FName, p_stRxVFile->FName, sizeof(TxVFile.FName));
            TxVFile.Boffset = p_stRxVFile->Boffset;
            TxVFile.TxfrId = p_stRxVFile->TxfrId;
            uint8_t *ptargetfolder = 0;
            
            if(p_stRxVFile->Boffset == 0 && (p_stRxVFile->LBlock == LBLOCK_TXBULK_ACK || p_stRxVFile->LBlock == LBLOCK_CLOSE))
            {    
                ptargetfolder = _Findtargetfolder(p_stRxVFile->EqID, sizeof(p_stRxVFile->EqID));
                if(ptargetfolder == 0)
                {    
                    printf("[_ProcRx] Error->Findtargetfolder for = %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", 
                                                                                                                p_stRxVFile->EqID[0],
                                                                                                                p_stRxVFile->EqID[1],
                                                                                                                p_stRxVFile->EqID[2],
                                                                                                                p_stRxVFile->EqID[3],
                                                                                                                p_stRxVFile->EqID[4],
                                                                                                                p_stRxVFile->EqID[5],
                                                                                                                p_stRxVFile->EqID[6],
                                                                                                                p_stRxVFile->EqID[7],
                                                                                                                p_stRxVFile->EqID[8],
                                                                                                                p_stRxVFile->EqID[9]);
                                            
                    _log("[_ProcRx] Error->Findtargetfolder for = %02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", 
                                                                                                                p_stRxVFile->EqID[0],
                                                                                                                p_stRxVFile->EqID[1],
                                                                                                                p_stRxVFile->EqID[2],
                                                                                                                p_stRxVFile->EqID[3],
                                                                                                                p_stRxVFile->EqID[4],
                                                                                                                p_stRxVFile->EqID[5],
                                                                                                                p_stRxVFile->EqID[6],
                                                                                                                p_stRxVFile->EqID[7],
                                                                                                                p_stRxVFile->EqID[8],
                                                                                                                p_stRxVFile->EqID[9]);
                }
            }

            if(p_stRxVFile->BlockNr < _BULK_SIZE)
                memcpy(&BTxVFile[idx].BulkBuffer[p_stRxVFile->BlockNr], p_stRxVFile->BBuffer, sizeof(BTxVFile[idx].BulkBuffer[p_stRxVFile->BlockNr]));   
            else
                printf("Fuera de rango!!!!!\n");

            if(p_stRxVFile->LBlock == LBLOCK_TXBULK_ACK || p_stRxVFile->LBlock == LBLOCK_CLOSE)
            {
                // Libera el vector si es un inicio de bloque
                uint32_t idxtmp = _Look4Pos(*_SrcIp);
                if(p_stRxVFile->Boffset == 0 && idxtmp)
                    _Look4Release(idxtmp - 1); 
   
                TxVFile.FStatus = _StoreFile(p_stRxVFile->FName, 
                                                ptargetfolder, 
                                                //p_stRxVFile->BBuffer, 
                                                &BTxVFile[idx].BulkBuffer[0],
                                                p_stRxVFile->BSize, 
                                                p_stRxVFile->Boffset, 
                                                p_stRxVFile->LBlock, 
                                                p_stRxVFile->BlockNr,
                                                *_SrcIp,
                                                p_stRxVFile->TxfrId,
                                                p_stRxVFile->FileCRC,
                                                &TxVFile.BlockNr,
                                                &TxVFile.FileCRC);
#ifdef _DEBUG_RCSERVER    
                printf("Tx FStatus = (%d)\n", TxVFile.FStatus);  
                printf("Target = (%X)\n", ptargetfolder);  
                //printf("Gen BlockNr = (%d)\n", TxVFile.BlockNr);
                
                //if(TxVFile.BlockNr != p_stRxVFile->BlockNr)
                //    printf("Warning Gen Block %d <> BlockNr %d\n", TxVFile.BlockNr, p_stRxVFile->BlockNr);
                
                printf("Gen FileCRC = (%04X)\n", TxVFile.FileCRC);
#endif
                // Si es primer o último bloque devuelve FACK    
                if(p_stRxVFile->LBlock == LBLOCK_FIRST || p_stRxVFile->LBlock == LBLOCK_CLOSE || p_stRxVFile->LBlock == LBLOCK_TXBULK_ACK)
                {
                    TxVFile.Crc = CalcCrc16((uint8_t*)&TxVFile, sizeof(TxVFile) - sizeof(TxVFile.Crc), 0);

                    if(bTx) memcpy(bTx, &TxVFile, sizeof(TxVFile));
                    
                    if(_Send2EQ(rxaddr, (uint8_t*)&TxVFile, sizeof(TxVFile)))
                    {
                        _log("[_ProcRx] ERROR-> Tx RC_CMD_VIDEOFILE a RedCam IP:%s\n\r", ip_address);
                    }
                }
            }
        }
        break;

        default:
            printf("[_ProcRx] Rx Cmd Desconocido %X\n", Rxbuffer[0]);
    }
}

void _ProcTxVFileTimeout(void)
{
    static uint8_t Div = 0;

    // Cada 1Seg
    if(Div++ >= 100)
    {
        for(uint32_t idx = 0; idx < _MAXSTOREV; idx++)    
        {
            if(BTxVFile[idx].Status & 1)
            {
                if(BTxVFile[idx].WTimer) BTxVFile[idx].WTimer--;
                else
                {    
                    printf("[_ProcTxVFileTimeout] Vector VFileTx liberado (%d)\n", idx);
                    _log("[_ProcTxVFileTimeout] Vector VFileTx liberado (%d)\n", idx);
                    _Look4BTxRelease(idx);
                }
            }    
        }    
        Div = 0;
    }
}
        
void _Proc10msFuncs(void)
{
    //Process StoreFiles Timeouts
    _ProcStFilesTimeout();
    // Process TxVFiles Timeouts
    _ProcTxVFileTimeout();

    if(RCServer.dbreadtim) RCServer.dbreadtim--;
    else
    {
        int rcheck = _dbcheck_devices(&devices_info);
        int r = 0;
        if(rcheck == _DB_DEVICES_CHANGED) r = _dbread_bulk();
        // Lee en forma masiva base de datos, si devolvió error...
        if(r)
        {
            printf("[_Proc10msFuncs] Falla acceso DB ltdb %d\n", r);   
            _log("[_Proc10msFuncs] ERROR-> Falla acceso DB ltdb %d\n\r", r);
        }   
        // test
        else
        {
            // Sino sin error de lectura...

        }
        RCServer.dbreadtim = _TMAXDBREAD;
    }
}

#ifndef _TEST_DB
int main(void) 
{
    int len, nbytes;
    _InitRCServer();

    printf("RCServer activo puerto: %d...\n", DEST_PORT);
    _log("RCServer activo puerto: %d...\n", DEST_PORT);

    while (1) 
    {
        len = sizeof(cliaddr);
        nbytes = recvfrom(sockfd, (char *)RxBuffer, sizeof(RxBuffer), MSG_DONTWAIT, (struct sockaddr *)&cliaddr, &len);
        if(nbytes && (nbytes <= sizeof(RxBuffer)))
        {
            uint16_t RxCrc = *((uint16_t*)&RxBuffer[nbytes - 2]); 
            //uint16_t Crc = CalcCrc16(RxBuffer, nbytes - 2, 0);
            uint16_t Crc = crc_ccitt(RxBuffer, nbytes - 2);
            Crc = (Crc<<8)|(Crc>>8);
            if(Crc == RxCrc)
            {    
                if(RxBuffer[0] == 0xA5)
                    _ProcRx(&cliaddr, RxBuffer, nbytes);
                else 
                    printf("[main] Rx Error tipo %02X\n", RxBuffer[0]);
            }
            else 
                printf("[main] Rx Error de CRC->%04X-%04X cmd:%02X\n", RxCrc, Crc, RxBuffer[0]);
        }
        else 
        {
            static int Tim1ms = 0;
            static int8_t xdiv = 0;
            
            _ProcIDLE();

            if((_GetTimer1ms() - Tim1ms) >= 1)
            {
                if(xdiv++ >= 9)
                {
                    _Proc10msFuncs();
                    xdiv = 0;    
                }
                Tim1ms = _GetTimer1ms();
            }
        }
    }
    close(sockfd);
    return 0;
}
#endif
