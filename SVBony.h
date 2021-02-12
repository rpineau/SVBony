//
//  SVBony.hpp
//  IIDC
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

#include "SVBCameraSDK.h"

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

typedef struct camResolution {
    uint32_t            nWidth;
    uint32_t            nHeight;
} camResolution_t;

class CSVBony {
public:
    CSVBony();
    ~CSVBony();

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

    int         getTemperture(double &dTemp);
    int         getWidth();
    int         getHeight();
    double      getPixelSize();
    void        setBinSize(int nBin);
    
    bool        isCameraColor();
    void        getBayerPattern(std::string &sBayerPattern);
    int         setROI(int nLeft, int nTop, int nRight, int nBottom);
    int         clearROI(void);

    bool        isFameAvailable();
    
    uint32_t    getBitDepth();
    int         getFrame(int nHeight, int nMemWidth, unsigned char* frameBuffer);

    int         getResolutions(std::vector<camResolution_t> &vResList);
    // int         setFeature(dc1394feature_t tFeature, uint32_t nValue, dc1394feature_mode_t tMode);
    // int         getFeature(dc1394feature_t tFeature, uint32_t &nValue, uint32_t nMin, uint32_t nMax,  dc1394feature_mode_t &tMode);

protected:
    SVB_CAMERA_INFO         m_CameraInfo;
    SVB_CAMERA_PROPERTY     m_cameraProperty;
    
    double                  m_dPixelSize;
    int                     m_nMaxWidth;
    int                     m_nMaxHeight;
    bool                    m_bIsColorCam;
    int                     m_nBayerPattern;
    int                     m_nMaxBitDepth;
    int                     m_nNbBin;
    int                     m_SupportedBins[MAX_NB_BIN];
    void                    setCameraFeatures(void);
    int                     setCameraExposure(double dTime);
    int                     m_nCurrentBin;
    
    std::vector<camResolution_t>         m_vResList;
    camResolution_t         m_tCurrentResolution;
    bool                    m_bNeedSwap;
    bool                    m_bNeedDepthFix;
    uint8_t                 m_nDepthBitLeftShift;


    bool                    m_bConnected;
    bool                    m_bFrameAvailable;
    bool                    m_bIsPGR;

    unsigned char *         m_pframeBuffer;
    int                     m_nCameraID;
    std::string             m_sCameraName;
    std::string             m_sCameraSerial;
    
    char                    m_szCameraName[BUFFER_LEN];
    bool                    m_bDeviceIsUSB;
    bool                    m_bAbort;
    std::map<int,bool>      m_mAvailableFrameRate;

#ifdef PLUGIN_DEBUG
    std::string m_sLogfilePath;
    // timestamp for logs
    char *timestamp;
    time_t ltime;
    FILE *Logfile;      // LogFile
#endif

};
#endif /* SVBony_hpp */
