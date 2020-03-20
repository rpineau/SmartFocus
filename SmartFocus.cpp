//
//  SmartFocus.cpp
//
//  Created by Rodolphe Pineau on 2020/03/17.
//  SmartFocus X2 plugin


#include "SmartFocus.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>
#ifdef SB_MAC_BUILD
#include <unistd.h>
#endif
#ifdef SB_WIN_BUILD
#include <time.h>
#endif


CSmartFocus::CSmartFocus()
{

    m_pSerx = NULL;

    m_bDebugLog = false;
    m_bIsConnected = false;

    m_nCurPos = 0;
    m_nTargetPos = 0;
    m_nPosLimit = 65535;
    m_bMoving = false;

    m_CmdTimer.Reset();

    
#ifdef PLUGIN_DEBUG
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\SmartFocusLog.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/SmartFocusLog.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/SmartFocusLog.txt";
#endif
    Logfile = fopen(m_sLogfilePath.c_str(), "w");
#endif

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [SmartFocus::SmartFocus] version %.3f build 2020_03_19_1445.\n", timestamp, DRIVER_VERSION);
    fprintf(Logfile, "[%s] SmartFocus Constructor Called\n", timestamp);
    fflush(Logfile);
#endif

    
}

CSmartFocus::~CSmartFocus()
{
#ifdef	PLUGIN_DEBUG
    // Close LogFile
    if (Logfile) fclose(Logfile);
#endif
}

int CSmartFocus::Connect(const char *pszPort)
{
    int nErr = PLUGIN_OK;
    int nStatus;

    if(!m_pSerx)
        return ERR_COMMNOLINK;

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CSmartFocus::Connect Called %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif

    // 9600 8N1
    nErr = m_pSerx->open(pszPort, 9600, SerXInterface::B_NOPARITY, "-DTR_CONTROL 1");
    if( nErr == 0)
        m_bIsConnected = true;
    else
        m_bIsConnected = false;

    if(!m_bIsConnected)
        return nErr;

    m_pSleeper->sleep(2000);

#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CSmartFocus::Connect connected to %s\n", timestamp, pszPort);
	fflush(Logfile);
#endif
	

    // get status so we can figure out what device we are connecting to.
#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
	fprintf(Logfile, "[%s] CSmartFocus::Connect getting device status\n", timestamp);
	fflush(Logfile);
#endif
    nErr = getDeviceStatus(nStatus);
    if(nErr) {
		m_bIsConnected = false;
#ifdef PLUGIN_DEBUG
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] CSmartFocus::Connect **** ERROR **** getting device status\n", timestamp);
		fflush(Logfile);
#endif
        return nErr;
    }
    // m_globalStatus.deviceType now contains the device type
    return nErr;
}

void CSmartFocus::Disconnect()
{
    if(m_bIsConnected && m_pSerx)
        m_pSerx->close();
 
	m_bIsConnected = false;
}

#pragma mark move commands
int CSmartFocus::haltFocuser()
{
    int nErr;
    unsigned char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    nErr = Command((unsigned char *)"s", 1, szResp, 1, SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    m_nTargetPos = m_nCurPos;
    m_bMoving = false;
    
    return nErr;
}

int CSmartFocus::gotoPosition(int nPos)
{
    int nErr = PLUGIN_OK;
    unsigned char szCmd[SERIAL_BUFFER_SIZE];
    unsigned char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;


    if (nPos>m_nPosLimit)
        return ERR_LIMITSEXCEEDED;

    // don't send any other commands while moving.
    if(m_bMoving) {
        return ERR_COMMANDINPROGRESS;
    }

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CSmartFocus::gotoPosition goto position  : %d\n", timestamp, nPos);
    fflush(Logfile);
#endif
    
    szCmd[0] = 'g';
    szCmd[1] = (nPos & 0xff00) >> 8;
    szCmd[2] = (nPos & 0x00ff);
    
    nErr = Command(szCmd, 3, szResp, 1, SERIAL_BUFFER_SIZE);
    if(nErr) {
    #ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CSmartFocus::gotoPosition goto error  : %d\n", timestamp, nErr);
        fflush(Logfile);
    #endif
        return nErr;
    }
    m_nTargetPos = nPos;
    m_bMoving = true;
    
    return nErr;
}

int CSmartFocus::moveRelativeToPosision(int nSteps)
{
    int nErr;

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CSmartFocus::moveRelativeToPosision goto relative position  : %d\n", timestamp, nSteps);
    fflush(Logfile);
#endif

    m_nTargetPos = m_nCurPos + nSteps;
    nErr = gotoPosition(m_nTargetPos);
    return nErr;
}

#pragma mark command complete functions

int CSmartFocus::isGoToComplete(bool &bComplete)
{
    int nErr = PLUGIN_OK;
    unsigned char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
    
    if(!m_bMoving) {
        bComplete = true;
        return nErr;
    }
            
    bComplete = false;
    nErr = readResponse(szResp, 1, SERIAL_BUFFER_SIZE);
    if(nErr)    // probably a timeout
        return PLUGIN_OK;

    if(szResp[0] == 'r')
        return ERR_CMDFAILED;
    
    if(szResp[0] == 'c') {
        m_bMoving = false;
        bComplete = true;
    }
        

#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CSmartFocus::isGoToComplete bComplete : %s\n", timestamp, bComplete?"True":"False");
    fprintf(Logfile, "[%s] CSmartFocus::isGoToComplete m_bMoving : %s\n", timestamp, m_bMoving?"True":"False");
    fflush(Logfile);
#endif

    return nErr;
}


#pragma mark getters and setters
int CSmartFocus::getDeviceStatus(int &nStatus)
{
    int nErr;
    unsigned char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;
	

    nErr = Command((const unsigned char*)"t", 1, szResp, 2,  SERIAL_BUFFER_SIZE);
    if(nErr)
        return nErr;
    nStatus = szResp[1];
    return nErr;
}

int CSmartFocus::getFirmwareVersion(char *pszVersion, int nStrMaxLen)
{
    int nErr = PLUGIN_OK;
    unsigned char szResp[SERIAL_BUFFER_SIZE];
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    if(!m_bIsConnected)
        return NOT_CONNECTED;

    // don't send any other commands while moving.
    if(m_bMoving) {
        return ERR_COMMANDINPROGRESS;
    }

    nErr = Command((const unsigned char*)"b", 1, szResp, 2, SERIAL_BUFFER_SIZE);
    if(nErr) {
        #ifdef PLUGIN_DEBUG
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] CSmartFocus::getFirmwareVersion ERROR : %d\n", timestamp, nErr);
            fflush(Logfile);
        #endif
        return nErr;
    }
    
#ifdef PLUGIN_DEBUG
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CSmartFocus::getFirmwareVersion szResp : %s\n", timestamp, szResp);
    fflush(Logfile);
#endif

    strncpy(pszVersion, (char *)szResp+1, 1);
    return nErr;
}


int CSmartFocus::getPosition(int &nPosition)
{
    int nErr = PLUGIN_OK;
    unsigned char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    // don't send any other commands while moving.
    if(m_bMoving) {
        nPosition = m_nCurPos;
        return nErr;
    }
    
    nErr = Command((const unsigned char*)"p", 1, szResp, 3,  SERIAL_BUFFER_SIZE);
    if(nErr) {
        if(m_bMoving) {
            nPosition = m_nCurPos;
            nErr = PLUGIN_OK;
        }
        return nErr;
    }
    if(szResp[0] == 'p') {
        nPosition = (((int(szResp[1])<<8)&0xff00) | (int(szResp[2])&0x00ff)) & 0x0000ffff;
        m_nCurPos = nPosition;
    }
    
    #ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CSmartFocus::getPosition m_nCurPos : %d\n", timestamp, m_nCurPos);
        fflush(Logfile);
    #endif

    return nErr;
}

// there is no real sync on the JMI smart focus, only set zero position
int CSmartFocus::syncMotorPosition(int nPos)
{
    int nErr = PLUGIN_OK;
    unsigned char szResp[SERIAL_BUFFER_SIZE];

	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    // don't send any other commands while moving.
    if(m_bMoving) {
        return ERR_COMMANDINPROGRESS;
    }


    if(nPos != 0)
        return ERR_CMDFAILED;
    
    nErr = Command((const unsigned char*)"z", 1, szResp, 1, SERIAL_BUFFER_SIZE);
    printf("[syncMotorPosition] szResp = %s\n", szResp);
    if(nErr)
        return nErr;

    m_nCurPos = 0;
    return nErr;
}



int CSmartFocus::getPosLimit()
{
    return m_nPosLimit;
}

void CSmartFocus::setPosLimit(int nLimit)
{
    m_nPosLimit = nLimit;
}


#pragma mark command and response functions

int CSmartFocus::Command(const unsigned char *pszszCmd, int nCmdSize, unsigned char *pszResult, int nResultLen, int nResultMaxLen)
{
    int nErr = PLUGIN_OK;
    unsigned char szResp[SERIAL_BUFFER_SIZE];
    unsigned long  ulBytesWrite;
    int dDelayMs;
#ifdef PLUGIN_DEBUG
    unsigned char cHexMessage[LOG_BUFFER_SIZE];
#endif
    
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    
    // do we need to wait ?
    if(m_CmdTimer.GetElapsedSeconds()<CMD_WAIT_INTERVAL) {
        dDelayMs = CMD_WAIT_INTERVAL - int(m_CmdTimer.GetElapsedSeconds() *1000);
        if(dDelayMs>0)
            m_pSleeper->sleep(dDelayMs);
    }
    
    m_pSerx->purgeTxRx();
#ifdef PLUGIN_DEBUG
	ltime = time(NULL);
	timestamp = asctime(localtime(&ltime));
	timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] CSmartFocus::Command command lenght = %d\n", timestamp, nCmdSize);
    fprintf(Logfile, "[%s] CSmartFocus::Command Sending %c\n", timestamp, pszszCmd[0]);
    if(nCmdSize>1) {
        fprintf(Logfile, "[%s] CSmartFocus::Command command parameters =  %d\n", timestamp, (((int(pszszCmd[1])<<8)&0xff00) | (int(pszszCmd[2])&0x00ff)) & 0x0000ffff);
    }
    hexdump(pszszCmd, cHexMessage, nCmdSize, LOG_BUFFER_SIZE);
    fprintf(Logfile, "[%s] CSmartFocus::Command Sending '%s'\n", timestamp, cHexMessage);

    fflush(Logfile);
#endif
    nErr = m_pSerx->writeFile((void *)pszszCmd, nCmdSize, ulBytesWrite);
    m_pSerx->flushTx();

    if(nErr){
        return nErr;
    }

    if(pszResult) {
        memset(pszResult, 0, nResultMaxLen);
        // read response
        nErr = readResponse(szResp, nResultLen, SERIAL_BUFFER_SIZE);
        if(nErr){
        }
#ifdef PLUGIN_DEBUG
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] CSmartFocus::Command response \"%c\"\n", timestamp, szResp[0]);
        if(nResultLen>1) {
            fprintf(Logfile, "[%s] CSmartFocus::Command response parameters = %d\n", timestamp, (((int(szResp[1])<<8)&0xff00) | (int(szResp[2])&0x00ff)) & 0x0000ffff);
            
        }
        hexdump(szResp, cHexMessage, nResultLen, LOG_BUFFER_SIZE);
        fprintf(Logfile, "[%s] CSmartFocus::Command received '%s'\n", timestamp, cHexMessage);

        fflush(Logfile);
#endif
        memcpy(pszResult, szResp, nResultLen);
#ifdef PLUGIN_DEBUG
		ltime = time(NULL);
		timestamp = asctime(localtime(&ltime));
		timestamp[strlen(timestamp) - 1] = 0;
		fprintf(Logfile, "[%s] CSmartFocus::Command 1st byte response copied to pszResult : \"%c\"\n", timestamp, pszResult[0]);
        if(nResultLen>1) {
            fprintf(Logfile, "[%s] CSmartFocus::Command copied response parameters = %d\n", timestamp, (((int(pszResult[1])<<8)&0xff00) | (int(pszResult[2])&0x00ff)) & 0x0000ffff );
        }
        hexdump(szResp, cHexMessage, nResultLen, LOG_BUFFER_SIZE);
        fprintf(Logfile, "[%s] CSmartFocus::Command copied to pszResult (hex) : '%s'\n", timestamp, cHexMessage);

		fflush(Logfile);
#endif
    }
    return nErr;
}

int CSmartFocus::readResponse(unsigned char *pszRespBuffer, int nResultLen, int nBufferLen)
{
    int nErr = PLUGIN_OK;
    unsigned long ulBytesRead = 0;
	
	if(!m_bIsConnected)
		return ERR_COMMNOLINK;

    memset(pszRespBuffer, 0, (size_t) nBufferLen);
    if(nResultLen > nBufferLen)
        return ERR_CMDFAILED;
    
    nErr = m_pSerx->readFile(pszRespBuffer, nResultLen, ulBytesRead, MAX_TIMEOUT);
    if(nErr) {
        return nErr;
    }

    if (ulBytesRead < nResultLen) {// timeout
#ifdef PLUGIN_DEBUG
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] CSmartFocus::readResponse timeout\n", timestamp);
        fflush(Logfile);
#endif
        nErr = ERR_NORESPONSE;
        return nErr;
    }

    return nErr;
}

#ifdef PLUGIN_DEBUG
void CSmartFocus::hexdump(const unsigned char* pszInputBuffer, unsigned char *pszOutputBuffer, int nInputBufferSize, int nOutpuBufferSize)
{
    unsigned char *pszBuf = pszOutputBuffer;
    int nIdx=0;

    memset(pszOutputBuffer, 0, nOutpuBufferSize);
    for(nIdx=0; nIdx < nInputBufferSize && pszBuf < (pszOutputBuffer + nOutpuBufferSize -3); nIdx++){
        snprintf((char *)pszBuf,4,"%02X ", pszInputBuffer[nIdx]);
        pszBuf+=3;
    }
}
#endif
