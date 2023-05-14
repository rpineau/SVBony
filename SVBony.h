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
#include <sstream>
#include <iomanip>
#include <fstream>
#include <map>
#include <thread>

#ifndef SB_WIN_BUILD
#include <unistd.h>
#endif

#include "../../licensedinterfaces/sberrorx.h"
#include "../../licensedinterfaces/loggerinterface.h"
#include "../../licensedinterfaces/sleeperinterface.h"

#include "SVBCameraSDK.h"
#include "StopWatch.h"

// #define PLUGIN_DEBUG    2

#define PLUGIN_VERSION      1.8
#define BUFFER_LEN 128
#define PLUGIN_OK   0
#define MAX_NB_BIN  16

#define VAL_NOT_AVAILABLE           0xDEADBEEF

#define MAX_DATA_TIMEOUT    50

typedef struct _camera_info {
    int     cameraId;
    SVB_SN  Sn;
    char    model[BUFFER_LEN];
} camera_info_t;


class CSVBony {
public:
    CSVBony();
    ~CSVBony();


    void        setUserConf(bool bUserConf);

    int         Connect(int nCameraId);
    void        Disconnect(bool bTrunCoolerOff);
    void        setCameraId(int nCameraId);
    void        getCameraId(int &nCcameraId);
    int         getCameraIdFromSerial(int &nCameraId, std::string sSerial);
    int         getCameraSerialFromID(int nCameraId, std::string &sSerial);
    void        getCameraNameFromID(int nCameraId, std::string &sName);
    
    void        getCameraName(std::string &sName);
    void        setCameraSerial(std::string sSerial);
    void        getCameraSerial(std::string &sSerial);

    int         listCamera(std::vector<camera_info_t>  &cameraIdList);

    void        getFirmwareVersion(std::string &sVersion);

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
    void        getFlip(std::string &sFlipMode);

    int         getGain(long &nMin, long &nMax, long &nValue);
    int         setGain(long nGain);
    int         getGamma(long &nMin, long &nMax, long &nValue);
    int         setGamma(long nGamma);
    int         getGammaContrast(long &nMin, long &nMax, long &nValue);
    int         setGammaContrast(long nGammaContrast);
    int         getWB_R(long &nMin, long &nMax, long &nValue, bool &bIsAuto);
    int         setWB_R(long nWB_R, bool bIsAuto = SVB_FALSE);
    int         getWB_G(long &nMin, long &nMax, long &nValue, bool &bIsAuto);
    int         setWB_G(long nWB_G, bool bIsAuto = SVB_FALSE);
    int         getWB_B(long &nMin, long &nMax, long &nValue, bool &bIsAuto);
    int         setWB_B(long nWB_B, bool bIsAuto = SVB_FALSE);
    int         getFlip(long &nMin, long &nMax, long &nValue);
    int         setFlip(long nFlip);
    int         getSpeedMode(long &nMin, long &nMax, long &nValue);
    int         setSpeedMode(long nFlip);
    int         getContrast(long &nMin, long &nMax, long &nValue);
    int         setContrast(long nContrast);
    int         getSharpness(long &nMin, long &nMax, long &nValue);
    int         setSharpness(long nSharpness);
    int         getSaturation(long &nMin, long &nMax, long &nValue);
    int         setSaturation(long nSaturation);
    int         getBlackLevel(long &nMin, long &nMax, long &nValue);
    int         setBlackLevel(long nBlackLevel);
    int         getBadPixelCorrection(bool &bEnabled);
    int         setBadPixelCorrection(bool bEnbaled);


    int         setROI(int nLeft, int nTop, int nWidth, int nHeight);
    int         clearROI(void);

    bool        isFameAvailable();
    
    uint32_t    getBitDepth();
    int         getFrame(int nHeight, int nMemWidth, unsigned char* frameBuffer);

    SVB_ERROR_CODE        restartCamera();

    int         RelayActivate(const int nXPlus, const int nXMinus, const int nYPlus, const int nYMinus, const bool bSynchronous, const bool bAbort);

    int         getNbGainInList();
    std::string getGainFromListAtIndex(int nIndex);
    void        rebuildGainList();
#ifdef PLUGIN_DEBUG
    void        log(std::string logString);
#endif
    
protected:
    
    SVB_ERROR_CODE          getControlValues(SVB_CONTROL_TYPE nControlType, long &nMin, long &nMax, long &nValue, SVB_BOOL &bIsAuto);
    SVB_ERROR_CODE          setControlValue(SVB_CONTROL_TYPE nControlType, long nValue, SVB_BOOL bAuto=SVB_FALSE);

    void                    buildGainList(long nMin, long nMax, long nValue);

    bool                    m_bSetUserConf;
    int                     m_nCameraNum;

    SVB_CAMERA_PROPERTY     m_cameraProperty;
    SVB_CAMERA_PROPERTY_EX  m_CameraPorpertyEx;

    SVB_IMG_TYPE            m_nVideoMode;
    int                     m_nControlNums;
    std::vector<SVB_CONTROL_CAPS> m_ControlList;

    std::vector<std::string>    m_GainList;
    int                     m_nNbGainValue;

    long                    m_nGain;
    long                    m_nExposureMs;
    long                    m_nGamma;
    long                    m_nGammaContrast;
    long                    m_nWbR;
    bool                    m_bR_Auto;
    long                    m_nWbG;
    bool                    m_bG_Auto;
    long                    m_nWbB;
    bool                    m_bB_Auto;
    long                    m_nFlip;
    long                    m_nSpeedMode;
    long                    m_nContrast;
    long                    m_nSharpness;
    long                    m_nSaturation;
    long                    m_nAutoExposureTarget;
    long                    m_nBlackLevel;
    bool                    m_bBadPixelCorrectionEnabled;
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
    SVB_GUIDE_DIRECTION     m_confGuideDir;

    // temperature
    CStopWatch              m_TemperatureTimer;
    bool                    m_bTempertureSupported;
    bool                    m_bPulseGuidingSupported;
    double                  m_dTemperature;
    double                  m_dSetPoint;
    double                  m_dPower;
    bool                    m_dCoolerEnabled;

    

#ifdef PLUGIN_DEBUG
    // timestamp for logs
    const std::string getTimeStamp();
    std::ofstream m_sLogFile;
    std::string m_sLogfilePath;
#endif

};
#endif /* SVBony_hpp */
