//
//  SVBony.hpp
//
//  Created by Rodolphe Pineau on 06/12/2017
//  Copyright © 2017 RTI-Zone. All rights reserved.
//

#ifndef SVBony_hpp
#define SVBony_hpp

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <map>

#ifndef SB_WIN_BUILD
#include <unistd.h>
#endif

#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"

#include "SVBCameraSDK.h"
#include "StopWatch.h"

#define PLUGIN_DEBUG    3
#define PLUGIN_VERSION      1.00
#define BUFFER_LEN 128
#define PLUGIN_OK   0
#define MAX_NB_BIN  16
typedef struct _camere_info {
    int     cameraId;
    SVB_SN  Sn;
    char    model[BUFFER_LEN];
} camera_info_t;


class CSVBony {
public:
    CSVBony();
    ~CSVBony();

    void        setSleeper(SleeperInterface *pSleeper) { m_pSleeper = pSleeper; };

    int         Connect(int nCameraId);
    void        Disconnect(void);
    void        setCameraId(int nCameraId);
    void        getCameraId(int &nCcameraId);
    void        getCameraIdFromSerial(int &nCameraId, std::string sSerial);
    void        getCameraSerialFromID(int nCameraId, std::string &sSerial);
    void        getCameraNameFromID(int nCameraId, std::string &sName);
    
    void        getCameraName(std::string &sName);
    void        setCameraSerial(std::string sSerial);
    void        getCameraSerial(std::string &sSerial);

    int         listCamera(std::vector<camera_info_t>  &cameraIdList);

    int         getNumBins();
    int         getBinFromIndex(int nIndex);
    
    int         startCaputure(double dTime);
    int         stopCaputure();
    void        abortCapture(void);

    int         getTemperture(double &dTemp, double &dPower, double &dSetPoint, bool &bEnabled);
    int         setCoolerTemperature(bool bOn, double dTemp);
    int         getWidth();
    int         getHeight();
    double      getPixelSize();
    void        setBinSize(int nBin);
    
    bool        isCameraColor();
    void        getBayerPattern(std::string &sBayerPattern);
    void        getGain(std::string &sGain);
    void        getGamma(std::string &sGamma);
    void        getGammaContrast(std::string &sGammaContrast);
    void        getWB_R(std::string &sWB_R);
    void        getWB_G(std::string &sWB_G);
    void        getWB_B(std::string &sWB_B);
    void        getFlip(std::string &sFlip);
    void        getContrast(std::string &sContrast);
    void        getSharpness(std::string &sSharpness);
    void        getSaturation(std::string &sSaturation);
    void        getBlackLevel(std::string &sBlackLevel);

    void        getGain(long &nMin, long &nMax, long &nValue);
    int         setGain(long nGain);
    void        getGamma(long &nMin, long &nMax, long &nValue);
    int         setGamma(long nGamma);
    void        getGammaContrast(long &nMin, long &nMax, long &nValue);
    int         setGammaContrast(long nGammaContrast);
    void        getWB_R(long &nMin, long &nMax, long &nValue);
    int         setWB_R(long nWB_R);
    void        getWB_G(long &nMin, long &nMax, long &nValue);
    int         setWB_G(long nWB_G);
    void        getWB_B(long &nMin, long &nMax, long &nValue);
    int         setWB_B(long nWB_B);
    void        getFlip(long &nMin, long &nMax, long &nValue);
    int         setFlip(long nFlip);
    void        getContrast(long &nMin, long &nMax, long &nValue);
    int         setContrast(long nContrast);
    void        getSharpness(long &nMin, long &nMax, long &nValue);
    int         setSharpness(long nSharpness);
    void        getSaturation(long &nMin, long &nMax, long &nValue);
    int         setSaturation(long nSaturation);
    void        getBlackLevel(long &nMin, long &nMax, long &nValue);
    int         setBlackLevel(long nBlackLevel);

    int         setROI(int nLeft, int nTop, int nWidth, int nHeight);
    int         clearROI(void);

    bool        isFameAvailable();
    
    uint32_t    getBitDepth();
    int         getFrame(int nHeight, int nMemWidth, unsigned char* frameBuffer);


protected:
    
    int         getControlValues(SVB_CONTROL_TYPE nControlType, long &nMin, long &nMax, long &nValue);

    SleeperInterface    *m_pSleeper;

    
    SVB_CAMERA_INFO         m_CameraInfo;
    SVB_CAMERA_PROPERTY     m_cameraProperty;
    SVB_IMG_TYPE            m_nVideoMode;
    int                     m_nControlNums;
    std::vector<SVB_CONTROL_CAPS> m_ControlList;
    
    long                    m_nGain;
    long                    m_nExposureMs;
    long                    m_nGamma;
    long                    m_nGammaConstrast;
    long                    m_nWbR;
    long                    m_nWbG;
    long                    m_nWbB;
    long                    m_nFlip;
    long                    m_nSpeedMode;
    long                    m_nContrast;
    long                    m_nSharpness;
    long                    m_nSaturation;
    long                    m_nAutoExposureTarget;
    long                    m_nBlackLevel;

    double                  m_dPixelSize;
    int                     m_nMaxWidth;
    int                     m_nMaxHeight;
    bool                    m_bIsColorCam;
    int                     m_nBayerPattern;
    int                     m_nMaxBitDepth;
    int                     m_nNbBin;
    int                     m_SupportedBins[MAX_NB_BIN];
    void                    setCameraFeatures(void);
    int                     m_nCurrentBin;
    
    bool                    m_bConnected;
 
    unsigned char *         m_pframeBuffer;
    int                     m_nCameraID;
    std::string             m_sCameraName;
    std::string             m_sCameraSerial;
    
    char                    m_szCameraName[BUFFER_LEN];
    bool                    m_bDeviceIsUSB;
    bool                    m_bAbort;
    std::map<int,bool>      m_mAvailableFrameRate;
    bool                    m_bCapturerunning;
    int                     m_nNbBitToShift;
    
    CStopWatch              m_ExposureTimer;
    double                  m_dCaptureLenght;
    
    int                     m_nROILeft;
    int                     m_nROITop;
    int                     m_nROIWidth;
    int                     m_nROIHeight;

    int                     m_nReqROILeft;
    int                     m_nReqROITop;
    int                     m_nReqROIWidth;
    int                     m_nReqROIHeight;

#ifdef PLUGIN_DEBUG
    std::string m_sLogfilePath;
    // timestamp for logs
    char *timestamp;
    time_t ltime;
    FILE *Logfile;      // LogFile
#endif

};
#endif /* SVBony_hpp */
