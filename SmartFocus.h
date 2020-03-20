//
//  SmartFocus.h
//
//  Created by Rodolphe Pineau on 2020/03/17.
//  SmartFocus X2 plugin

#ifndef __SMARTFOCUS__
#define __SMARTFOCUS__
#include <math.h>
#include <string.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <exception>
#include <typeinfo>
#include <stdexcept>

#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/serxinterface.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"

#include "StopWatch.h"

// #define PLUGIN_DEBUG 2

#define DRIVER_VERSION      1.0

#define SERIAL_BUFFER_SIZE 32
#define MAX_TIMEOUT 500
#define LOG_BUFFER_SIZE 256

#define CMD_WAIT_INTERVAL 200

enum SmartFocus_Errors    {PLUGIN_OK = 0, NOT_CONNECTED, ND_CANT_CONNECT, PLUGIN_BAD_CMD_RESPONSE, COMMAND_FAILED};
enum MotorDir       {NORMAL = 0 , REVERSE};
enum MotorStatus    {IDLE = 0, MOVING};


class CSmartFocus
{
public:
    CSmartFocus();
    ~CSmartFocus();

    int         Connect(const char *pszPort);
    void        Disconnect(void);
    bool        IsConnected(void) { return m_bIsConnected; };

    void        SetSerxPointer(SerXInterface *p) { m_pSerx = p; };
    void        setSleeper(SleeperInterface *pSleeper) { m_pSleeper = pSleeper; };

    // move commands
    int         haltFocuser();
    int         gotoPosition(int nPos);
    int         moveRelativeToPosision(int nSteps);

    // command complete functions
    int         isGoToComplete(bool &bComplete);

    // getter and setter
    void        setDebugLog(bool bEnable) {m_bDebugLog = bEnable; };

    int         getDeviceStatus(int &nStatus);

    int         getFirmwareVersion(char *pszVersion, int nStrMaxLen);
    int         getPosition(int &nPosition);
    int         syncMotorPosition(int nPos);
    int         getPosLimit(void);
    void        setPosLimit(int nLimit);

protected:

    int             Command(const unsigned char *pszCmd, int nCmdSize, unsigned char *pszResult, int nResultLen, int nResultMaxLen);
    int             readResponse(unsigned char *pszRespBuffer, int nResultLen, int nBufferLen);

    SerXInterface   *m_pSerx;
    SleeperInterface    *m_pSleeper;

    bool            m_bDebugLog;
    bool            m_bIsConnected;
    char            m_szFirmwareVersion[SERIAL_BUFFER_SIZE];
    char            m_szLogBuffer[LOG_BUFFER_SIZE];

    int             m_nCurPos;
    int             m_nTargetPos;
    int             m_nPosLimit;
    bool            m_bMoving;
    
    CStopWatch      m_CmdTimer;
    
#ifdef PLUGIN_DEBUG
    void            hexdump(const unsigned char* pszInputBuffer, unsigned char *pszOutputBuffer, int nInputBufferSize, int nOutpuBufferSize);
    std::string m_sLogfilePath;
	// timestamp for logs
	char *timestamp;
	time_t ltime;
	FILE *Logfile;	  // LogFile
#endif

};

#endif //__SMARTFOCUS__
