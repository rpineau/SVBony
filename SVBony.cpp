//
//  SVBony.cpp
//
//  Created by Rodolphe Pineau on 06/12/2017
//  Copyright © 2017 RTI-Zone. All rights reserved.
//

#include "SVBony.h"


CSVBony::CSVBony()
{

    m_pframeBuffer = NULL;
    m_bConnected = false;
    m_nCameraID = 0;
    m_bAbort = false;
    m_nCurrentBin = 1;
    m_nVideoMode = SVB_IMG_RAW12;
    m_nNbBitToShift = 4;
    m_dCaptureLenght = 0;
    m_bCapturerunning = false;
    m_pSleeper = nullptr;
    m_nNbBin = 1;
    m_SupportedBins[0] = 1;

    m_nROILeft = 0;
    m_nROITop = 0;
    m_nROIWidth = 0;
    m_nROIHeight = 0;

    m_nReqROILeft = 0;
    m_nReqROITop = 0;
    m_nReqROIWidth = 0;
    m_nReqROIHeight = 0;

    m_nControlNums = 0;
    m_ControlList.clear();
    
    m_nGain = 10;
    m_nExposureMs = (1 * 1000000);
    m_nGamma = 100;
    m_nGammaConstrast = 100;
    m_nWbR = 130;
    m_nWbG = 80;
    m_nWbB = 160;
    m_nFlip = 0;
    m_nSpeedMode = 0; // low speed
    m_nContrast = 50;
    m_nSharpness = 0;
    m_nSaturation = 100;
    m_nAutoExposureTarget = 0;
    m_nBlackLevel = 0;

    
    memset(m_szCameraName,0,BUFFER_LEN);
#ifdef PLUGIN_DEBUG
#if defined(SB_WIN_BUILD)
    m_sLogfilePath = getenv("HOMEDRIVE");
    m_sLogfilePath += getenv("HOMEPATH");
    m_sLogfilePath += "\\SVBonyLog.txt";
#elif defined(SB_LINUX_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/SVBonyLog.txt";
#elif defined(SB_MAC_BUILD)
    m_sLogfilePath = getenv("HOME");
    m_sLogfilePath += "/SVBonyLog.txt";
#endif
    Logfile = fopen(m_sLogfilePath.c_str(), "w");
#endif

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony] Version %3.2f build 2021_01_31_1800.\n", timestamp, PLUGIN_VERSION);
    fprintf(Logfile, "[%s] [CSVBony] Constructor Called.\n", timestamp);
    fflush(Logfile);
#endif
    
}

CSVBony::~CSVBony()
{

    if(m_pframeBuffer)
        free(m_pframeBuffer);


}

int CSVBony::Connect(int nCameraID)
{
    int nErr = PLUGIN_OK;
    int i;
    SVB_ERROR_CODE ret;
    SVB_CONTROL_CAPS    Caps;

    if(nCameraID)
        m_nCameraID = nCameraID;
    else {
        // check if there is at least one camera connected to the system
        if(SVBGetNumOfConnectedCameras() == 1) {
            std::vector<camera_info_t> tCameraIdList;
            listCamera(tCameraIdList);
            if(tCameraIdList.size()) {
                m_nCameraID = tCameraIdList[0].cameraId;
                m_sCameraSerial.assign((char *)tCameraIdList[0].Sn.id);
            }
            else
                return ERR_NODEVICESELECTED;
        }
        else
            return ERR_NODEVICESELECTED;
    }
    
    ret = SVBOpenCamera(m_nCameraID);
    if (ret != SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::Connect] Error connecting to camera ID %d serial %s, Error = %d\n", timestamp, m_nCameraID, m_sCameraSerial.c_str(), ret);
        fflush(Logfile);
#endif
        return ERR_NORESPONSE;
        }

    getCameraNameFromID(m_nCameraID, m_sCameraName);

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::Connect] Connected to camera ID %d serial %s\n", timestamp, m_nCameraID, m_sCameraSerial.c_str());
        fflush(Logfile);
#endif

    
    ret = SVBGetCameraProperty(m_nCameraID, &m_cameraProperty);
    if (ret != SVB_SUCCESS)
    {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::Connect] SVBGetCameraProperty Error : %d\n", timestamp, ret);
        fflush(Logfile);
#endif
        SVBCloseCamera(m_nCameraID);
        return ERR_CMDFAILED;
    }

    m_nMaxWidth = (int)m_cameraProperty.MaxWidth;
    m_nMaxHeight = (int)m_cameraProperty.MaxHeight;
    m_bIsColorCam = m_cameraProperty.IsColorCam;
    m_nBayerPattern = m_cameraProperty.BayerPattern;
    m_nMaxBitDepth = m_cameraProperty.MaxBitDepth;
    m_nNbBitToShift = 16 - m_cameraProperty.MaxBitDepth;
    m_nNbBin = 0;
    m_nCurrentBin = 0;
    
    for (i = 0; i < MAX_NB_BIN; i++)
    {
        m_SupportedBins[i] = m_cameraProperty.SupportedBins[i];
        if (m_cameraProperty.SupportedBins[i] == 0) {
            break;
        }
        if(m_SupportedBins[i] == 1)
            m_nCurrentBin = 1;  // set default bin to 1 if available.
        m_nNbBin++;
    }

    if(!m_nCurrentBin)
        m_nCurrentBin = m_SupportedBins[0]; // if bin 1 was not availble .. use first bin in the array

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::Connect] Camera properties :\n", timestamp);
    fprintf(Logfile, "[%s] [CSVBony::Connect] m_nMaxWidth     : %d\n", timestamp, m_nMaxWidth);
    fprintf(Logfile, "[%s] [CSVBony::Connect] m_nMaxHeight    : %d\n", timestamp, m_nMaxHeight);
    fprintf(Logfile, "[%s] [CSVBony::Connect] m_bIsColorCam   : %s\n", timestamp, m_bIsColorCam?"Yes":"No");
    fprintf(Logfile, "[%s] [CSVBony::Connect] m_nBayerPattern : %d\n", timestamp, m_nBayerPattern);
    fprintf(Logfile, "[%s] [CSVBony::Connect] m_nMaxBitDepth  : %d\n", timestamp, m_nMaxBitDepth);
    fprintf(Logfile, "[%s] [CSVBony::Connect] m_nNbBitToShift : %d\n", timestamp, m_nNbBitToShift);
    fprintf(Logfile, "[%s] [CSVBony::Connect] m_nNbBin        : %d\n", timestamp, m_nNbBin);
    fflush(Logfile);
#endif

    float pixelSize;
    ret = SVBGetSensorPixelSize(m_nCameraID, &pixelSize);
    if (ret != SVB_SUCCESS && ret != SVB_ERROR_UNKNOW_SENSOR_TYPE)
    {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::Connect] SVBGetSensorPixelSize Error : %d\n", timestamp, ret);
        fflush(Logfile);
#endif
        SVBCloseCamera(m_nCameraID);
        return ERR_CMDFAILED;
    }
    if(ret == SVB_ERROR_UNKNOW_SENSOR_TYPE) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::Connect] m_dPixelSize    : Unknown !!!!!!! \n", timestamp);
    fflush(Logfile);
#endif

    }
    else {
        m_dPixelSize = (double)pixelSize;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::Connect] m_dPixelSize    : %3.2f\n", timestamp, m_dPixelSize);
        fflush(Logfile);
#endif
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::Connect] Setting ROI to max size\n", timestamp);
    fflush(Logfile);
#endif

    ret = SVBSetROIFormat(m_nCameraID, 0, 0, m_nMaxWidth, m_nMaxHeight, 1);
    if (ret != SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::Connect] SVBSetROIFormat Error : %d\n", timestamp, ret);
        fflush(Logfile);
#endif
        SVBCloseCamera(m_nCameraID);
        return ERR_CMDFAILED;
    }
    ret = SVBGetROIFormat(m_nCameraID, &m_nROILeft, &m_nROITop, &m_nROIWidth, &m_nROIHeight, &m_nCurrentBin);
    if (ret != SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::Connect] SVBGetROIFormat Error : %d\n", timestamp, ret);
        fflush(Logfile);
#endif
        SVBCloseCamera(m_nCameraID);
        return ERR_CMDFAILED;
    }

    ret = SVBGetNumOfControls(m_nCameraID, &m_nControlNums);

    for (i = 0; i < m_nControlNums; i++)
    {
        ret = SVBGetControlCaps(m_nCameraID, i, &Caps);
        if (ret != SVB_SUCCESS) {
            continue;
        }
        m_ControlList.push_back(Caps);
    }

    // set default values
    ret = SVBSetControlValue(m_nCameraID, SVB_GAIN, m_nGain, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_EXPOSURE, m_nExposureMs, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_GAMMA, m_nGamma, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_CONTRAST, m_nContrast, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_SHARPNESS, m_nSharpness, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_SATURATION, m_nSaturation, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_WB_R, m_nWbR, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_WB_G, m_nWbG, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_WB_B, m_nWbB, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_FRAME_SPEED_MODE , m_nSpeedMode, SVB_FALSE);
    
    ret = SVBSetOutputImageType(m_nCameraID, m_nVideoMode);
    ret = SVBSetCameraMode(m_nCameraID, SVB_MODE_TRIG_SOFT);
    
    ret = SVBStartVideoCapture(m_nCameraID);
    if(ret!=SVB_SUCCESS)
        nErr =ERR_CMDFAILED;

    m_bCapturerunning = true;
    m_bConnected = true;
    return nErr;
}

void CSVBony::Disconnect()
{

    
    if(m_pframeBuffer) {
        free(m_pframeBuffer);
        m_pframeBuffer = NULL;
    }
    SVBStopVideoCapture(m_nCameraID);
    SVBCloseCamera(m_nCameraID);
    m_bConnected = false;

}

void CSVBony::setCameraId(int nCameraId)
{
    m_nCameraID = nCameraId;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [setCameraId] nCameraId = %d\n", timestamp, nCameraId);
    fflush(Logfile);
#endif
}

void CSVBony::getCameraId(int &nCameraId)
{
    nCameraId = m_nCameraID;
}

void CSVBony::getCameraIdFromSerial(int &nCameraId, std::string sSerial)
{
    SVB_ERROR_CODE ret;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [getCameraIdFromSerial] sSerial = %s\n", timestamp, sSerial.c_str());
    fflush(Logfile);
#endif
    nCameraId = 0;
    
    int cameraNum = SVBGetNumOfConnectedCameras();
    for (int i = 0; i < cameraNum; i++)
    {
        ret = SVBGetCameraInfo(&m_CameraInfo, i);
        if (ret == SVB_SUCCESS)
        {
            if(sSerial==m_CameraInfo.CameraSN) {
                nCameraId = m_CameraInfo.CameraID;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                ltime = time(NULL);
                timestamp = asctime(localtime(&ltime));
                timestamp[strlen(timestamp) - 1] = 0;
                fprintf(Logfile, "[%s] [getCameraIdFromSerial] nCameraId = %d\n", timestamp, nCameraId);
                fflush(Logfile);
#endif
            }
        }
    }
}

void CSVBony::getCameraSerialFromID(int nCameraId, std::string &sSerial)
{

    SVB_ERROR_CODE ret;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [getCameraSerialFromID] nCameraId = %d\n", timestamp, nCameraId);
    fflush(Logfile);
#endif

    int cameraNum = SVBGetNumOfConnectedCameras();
    for (int i = 0; i < cameraNum; i++)
    {
        ret = SVBGetCameraInfo(&m_CameraInfo, i);
        if (ret == SVB_SUCCESS)
        {
            if(nCameraId==m_CameraInfo.CameraID) {
                sSerial.assign(m_CameraInfo.CameraSN);
                break;
            }
        }
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [getCameraSerialFromID] camera id %d SN: %s\n", timestamp, nCameraId, sSerial.c_str());
    fflush(Logfile);
#endif
}

void CSVBony::getCameraNameFromID(int nCameraId, std::string &sName)
{

    SVB_ERROR_CODE ret;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [getCameraNameFromID] nCameraId = %d\n", timestamp, nCameraId);
    fflush(Logfile);
#endif

    int cameraNum = SVBGetNumOfConnectedCameras();
    for (int i = 0; i < cameraNum; i++)
    {
        ret = SVBGetCameraInfo(&m_CameraInfo, i);
        if (ret == SVB_SUCCESS)
        {
            if(nCameraId==m_CameraInfo.CameraID) {
                sName.assign(m_CameraInfo.FriendlyName);
                break;
            }
        }
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [getCameraNameFromID] camera id %d name : %s\n", timestamp, nCameraId, sName.c_str());
    fflush(Logfile);
#endif
}

void CSVBony::setCameraSerial(std::string sSerial)
{
    m_sCameraSerial.assign(sSerial);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [setCameraId] m_sCameraSerial = %s\n", timestamp, m_sCameraSerial.c_str());
    fflush(Logfile);
#endif
}

void CSVBony::getCameraSerial(std::string &sSerial)
{
    sSerial.assign(m_sCameraSerial);
}

void CSVBony::getCameraName(std::string &sName)
{
    sName.assign(m_sCameraName);
}

int CSVBony::listCamera(std::vector<camera_info_t>  &cameraIdList)
{
    int nErr = PLUGIN_OK;
    camera_info_t   tCameraInfo;
    cameraIdList.clear();

    // list camera connected to the system
    int cameraNum = SVBGetNumOfConnectedCameras();

    SVB_ERROR_CODE ret;
    for (int i = 0; i < cameraNum; i++)
    {
        ret = SVBGetCameraInfo(&m_CameraInfo, i);
        if (ret == SVB_SUCCESS)
        {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [listCamera] Friendly name: %s\n", timestamp, m_CameraInfo.FriendlyName);
        fprintf(Logfile, "[%s] [listCamera] Port type: %s\n", timestamp, m_CameraInfo.PortType);
        fprintf(Logfile, "[%s] [listCamera] SN: %s\n", timestamp, m_CameraInfo.CameraSN);
        fprintf(Logfile, "[%s] [listCamera] Device ID: 0x%x\n", timestamp, m_CameraInfo.DeviceID);
        fprintf(Logfile, "[%s] [listCamera] Camera ID: %d\n", timestamp, m_CameraInfo.CameraID);
        fflush(Logfile);
#endif
        tCameraInfo.cameraId = m_CameraInfo.CameraID;
        strncpy(tCameraInfo.model, m_CameraInfo.FriendlyName, BUFFER_LEN);
        strncpy((char *)tCameraInfo.Sn.id, m_CameraInfo.CameraSN, sizeof(SVB_ID));
            
        cameraIdList.push_back(tCameraInfo);
        }
    }
    
    return nErr;
}

int CSVBony::getNumBins()
{
    return m_nNbBin;
}

int CSVBony::getBinFromIndex(int nIndex)
{
    if(nIndex>(m_nNbBin-1))
        return 1;
    
    return m_SupportedBins[nIndex];        
}


int CSVBony::startCaputure(double dTime)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;
    long nValue;
    SVB_BOOL isAuto;
    int nTimeout;
    m_bAbort = false;

    nTimeout = 0;
    nValue = SVB_EXP_WORKING;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::startCaputure] Start Capture\n", timestamp);
    fprintf(Logfile, "[%s] [CSVBony::startCaputure] Waiting for camera to be idle\n", timestamp);
    fflush(Logfile);
#endif
/*
 // this not working .. nValue is always returning 1000000
    while (true){
        ret =  SVBGetControlValue(m_nCameraID, SVB_EXPOSURE, &nValue, &isAuto);
        if(ret!=SVB_SUCCESS)
            return ERR_CMDFAILED;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::startCaputure] nValue = %d\n", timestamp, nValue);
        fflush(Logfile);
#endif

        if(nValue == SVB_EXP_IDLE)
            break;
        nTimeout++;
        if(nTimeout > 5)
            return ERR_CMDFAILED;
        m_pSleeper->sleep(100);
    }
 */
    
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::startCaputure] Starting capture\n", timestamp);
    fflush(Logfile);
#endif
    // set exposure time (s -> us)
    ret = SVBSetControlValue(m_nCameraID, SVB_EXPOSURE , (double)(dTime * 1000000), SVB_FALSE);
    if(ret!=SVB_SUCCESS)
        return ERR_CMDFAILED;

    if(!m_bCapturerunning) {
        ret = SVBStartVideoCapture(m_nCameraID);
        if(ret!=SVB_SUCCESS)
            nErr =ERR_CMDFAILED;
        m_bCapturerunning = true;
    }

    // soft trigger
    ret = SVBSendSoftTrigger(m_nCameraID);
    if(ret!=SVB_SUCCESS)
        nErr =ERR_CMDFAILED;

    m_dCaptureLenght = dTime;
    m_ExposureTimer.Reset();
    return nErr;
}

int CSVBony::stopCaputure()
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;

    ret = SVBStopVideoCapture(m_nCameraID);
    if(ret!=SVB_SUCCESS)
        nErr =ERR_CMDFAILED;

    m_bCapturerunning = false;
    
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::stopCaputure] capture stoppeds\n", timestamp);
    fflush(Logfile);
#endif

    m_pSleeper->sleep(500);

    return nErr;
}

void CSVBony::abortCapture(void)
{
    m_bAbort = true;
    stopCaputure();
}

int CSVBony::getTemperture(double &dTemp)
{
    int nErr = PLUGIN_OK;
    // no temperature data for now.
    dTemp = -100;
    return nErr;
}


int CSVBony::getWidth()
{
    return m_nMaxWidth;
}

int CSVBony::getHeight()
{
    return m_nMaxHeight;
}

double CSVBony::getPixelSize()
{
    return m_dPixelSize;
}

void CSVBony::setBinSize(int nBin)
{
    m_nCurrentBin = nBin;
}

bool CSVBony::isCameraColor()
{
    return m_bIsColorCam;
}

void CSVBony::getBayerPattern(std::string &sBayerPattern)
{
    if(m_bIsColorCam) {
        switch(m_nBayerPattern) {
            case SVB_BAYER_RG:
                sBayerPattern.assign("RGGB");
                break;
            case SVB_BAYER_BG:
                sBayerPattern.assign("BGGR");
                break;
            case SVB_BAYER_GR:
                sBayerPattern.assign("GRBG");
                break;
            case SVB_BAYER_GB:
                sBayerPattern.assign("GBRG");
                break;
        }
    }
}

void CSVBony::getGain(std::string &sGain)
{
    sGain = std::to_string(m_nGain);
}

void CSVBony::getGamma(std::string &sGamma)
{
    sGamma = std::to_string(m_nGamma);
}

void CSVBony::getGammaContrast(std::string &sGammaContrast)
{
    sGammaContrast = std::to_string(m_nGammaConstrast);
}

void CSVBony::getWB_R(std::string &sWB_R)
{
    sWB_R = std::to_string(m_nWbR);
}

void CSVBony::getWB_G(std::string &sWB_G)
{
    sWB_G = std::to_string(m_nWbG);
}

void CSVBony::getWB_B(std::string &sWB_B)
{
    sWB_B = std::to_string(m_nWbB);
}

void CSVBony::getFlip(std::string &sFlip)
{
    sFlip = std::to_string(m_nFlip);
}

void CSVBony::getContrast(std::string &sContrast)
{
    sContrast = std::to_string(m_nContrast);
}

void CSVBony::getSharpness(std::string &sSharpness)
{
    sSharpness = std::to_string(m_nSharpness);
}

void CSVBony::getSaturation(std::string &sSaturation)
{
    sSaturation = std::to_string(m_nSaturation);
}

void CSVBony::getBlackLevel(std::string &sBlackLevel)
{
    sBlackLevel = std::to_string(m_nBlackLevel);
}


void CSVBony::getGain(long &nMin, long &nMax, long &nValue)
{
    int ret;
    ret = getControlValues(SVB_GAIN, nMin, nMax, nValue);
    if(ret != PLUGIN_OK) {
        return;
    }
}

int CSVBony::setGain(long nGain)
{
    int nErr = PLUGIN_OK;
    
    return nErr;
}

void CSVBony::getGamma(long &nMin, long &nMax, long &nValue)
{
    int ret;
    ret = getControlValues(SVB_GAMMA, nMin, nMax, nValue);
    if(ret != PLUGIN_OK) {
        return;
    }
}

int CSVBony::setGamma(long nGamma)
{
    int nErr = PLUGIN_OK;
    
    return nErr;
}

void CSVBony::getGammaContrast(long &nMin, long &nMax, long &nValue)
{
    int ret;
    ret = getControlValues(SVB_GAMMA_CONTRAST, nMin, nMax, nValue);
    if(ret != PLUGIN_OK) {
        return;
    }
}

int CSVBony::setGammaContrast(long nGammaContrast)
{
    int nErr = PLUGIN_OK;
    
    return nErr;
}

void CSVBony::getWB_R(long &nMin, long &nMax, long &nValue)
{
    int ret;
    ret = getControlValues(SVB_WB_R, nMin, nMax, nValue);
    if(ret != PLUGIN_OK) {
        return;
    }
}

int CSVBony::setWB_R(long nWB_R)
{
    int nErr = PLUGIN_OK;
    
    return nErr;
}

void CSVBony::getWB_G(long &nMin, long &nMax, long &nValue)
{
    int ret;
    ret = getControlValues(SVB_WB_G, nMin, nMax, nValue);
    if(ret != PLUGIN_OK) {
        return;
    }
}

int CSVBony::setWB_G(long nWB_G)
{
    int nErr = PLUGIN_OK;
    
    return nErr;
}

void CSVBony::getWB_B(long &nMin, long &nMax, long &nValue)
{
    int ret;
    ret = getControlValues(SVB_WB_B, nMin, nMax, nValue);
    if(ret != PLUGIN_OK) {
        return;
    }
}

int CSVBony::setWB_B(long nWB_B)
{
    int nErr = PLUGIN_OK;
    
    return nErr;
}

void CSVBony::getFlip(long &nMin, long &nMax, long &nValue)
{
    int ret;
    ret = getControlValues(SVB_FLIP, nMin, nMax, nValue);
    if(ret != PLUGIN_OK) {
        return;
    }
}

int CSVBony::setFlip(long nFlip)
{
    int nErr = PLUGIN_OK;
    
    return nErr;
}

void CSVBony::getContrast(long &nMin, long &nMax, long &nValue)
{
    int ret;
    ret = getControlValues(SVB_CONTRAST, nMin, nMax, nValue);
    if(ret != PLUGIN_OK) {
        return;
    }
}

int CSVBony::setContrast(long nContrast)
{
    int nErr = PLUGIN_OK;
    
    return nErr;
}

void CSVBony::getSharpness(long &nMin, long &nMax, long &nValue)
{
    int ret;
    ret = getControlValues(SVB_SHARPNESS, nMin, nMax, nValue);
    if(ret != PLUGIN_OK) {
        return;
    }
}

int CSVBony::setSharpness(long nSharpness)
{
    int nErr = PLUGIN_OK;
    
    return nErr;
}

void CSVBony::getSaturation(long &nMin, long &nMax, long &nValue)
{
    int ret;
    ret = getControlValues(SVB_SATURATION, nMin, nMax, nValue);
    if(ret != PLUGIN_OK) {
        return;
    }
}

int CSVBony::setSaturation(long nSaturation)
{
    int nErr = PLUGIN_OK;
    
    return nErr;
}

void CSVBony::getBlackLevel(long &nMin, long &nMax, long &nValue)
{
    int ret;
    ret = getControlValues(SVB_BLACK_LEVEL, nMin, nMax, nValue);
    if(ret != PLUGIN_OK) {
        return;
    }
}

int CSVBony::setBlackLevel(long nBlackLevel)
{
    int nErr = PLUGIN_OK;
    
    return nErr;
}

int CSVBony::getControlValues(SVB_CONTROL_TYPE nControlType, long &nMin, long &nMax, long &nValue)
{
    SVB_ERROR_CODE ret;
    SVB_BOOL isAuto;
    int i;
    
    nValue = -1;
    nMin = -1;
    nMax = -1;
    
    ret = SVBGetControlValue(m_nCameraID, nControlType, &nValue, &isAuto);
    if(ret != SVB_SUCCESS)
        return ERR_CMDFAILED;
    // look for min,max in the control caps
    for(i = 0; i< m_nControlNums; i++) {
        if(m_ControlList.at(i).ControlType == nControlType) {
            nMin = m_ControlList.at(i).MinValue;
            nMax = m_ControlList.at(i).MaxValue;
            break;
        }
    }
    return ret;
}


int CSVBony::setROI(int nLeft, int nTop, int nWidth, int nHeight)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;
    int nNewLeft = 0;
    int nNewTop = 0;
    int nNewWidth = 0;
    int nNewHeight = 0;


    m_nReqROILeft = nLeft;
    m_nReqROITop = nTop;
    m_nReqROIWidth = nWidth;
    m_nReqROIHeight = nHeight;

    // X
    if( m_nReqROILeft % 8 != 0)
        nNewLeft = (m_nReqROILeft/8) * 8;  // round to lower 8 pixel. boundary
    else
        nNewLeft = m_nReqROILeft;

    // W
    if( (m_nReqROIWidth % 8 != 0) || (nLeft!=nNewLeft)) {// Adjust width to upper 8 boundary or if the left border changed we need to adjust the width
        nNewWidth = (( (m_nReqROIWidth + (nNewLeft%8)) /8) + 1) * 8;
        if ((nNewLeft + nNewWidth) > m_nMaxWidth) {
            nNewLeft -=8;
            if(nNewLeft<0) {
                nNewLeft = 0;
                nNewWidth = m_nMaxWidth;
            }
        }
    }
    else
        nNewWidth = m_nReqROIWidth;

    // Y
    if( m_nReqROITop % 2 != 0)
        nNewTop = (m_nReqROITop/2) * 2;  // round to lower even pixel.
    else
        nNewTop = m_nReqROITop;

    // H
    if( (m_nReqROIHeight % 2 != 0) || (nTop!=nNewTop)) {// Adjust height to lower 2 boundary or if the top changed we need to adjust the height
        nNewHeight = (((m_nReqROIHeight + (nNewTop%2))/2) + 1) * 2;
        if((nNewTop + nNewHeight) > m_nMaxHeight) {
            nNewTop -=2;
            if(nNewTop <0) {
                nNewTop = 0;
                nNewHeight = m_nMaxHeight;
            }
        }
    }
    else
        nNewHeight = m_nReqROIHeight;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setROI] nLeft          : %d\n", timestamp, nLeft);
    fprintf(Logfile, "[%s] [CSVBony::setROI] nTop           : %d\n", timestamp, nTop);
    fprintf(Logfile, "[%s] [CSVBony::setROI] nWidth         : %d\n", timestamp, nWidth);
    fprintf(Logfile, "[%s] [CSVBony::setROI] nHeight        : %d\n", timestamp, nHeight);
    
    fprintf(Logfile, "[%s] [CSVBony::setROI] nNewLeft      : %d\n", timestamp, nNewLeft);
    fprintf(Logfile, "[%s] [CSVBony::setROI] nNewTop       : %d\n", timestamp, nNewTop);
    fprintf(Logfile, "[%s] [CSVBony::setROI] nNewWidth     : %d\n", timestamp, nNewWidth);
    fprintf(Logfile, "[%s] [CSVBony::setROI] nNewHeight    : %d\n", timestamp, nNewHeight);

    fflush(Logfile);
#endif

    
    if( m_nROILeft == nNewLeft && m_nROITop == nNewTop && m_nROIWidth == nNewWidth && m_nROIHeight == nNewHeight) {
        return nErr; // no change since last ROI change request
    }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setROI] Requested x, y, w, h : %d, %d, %d, %d\n", timestamp, nLeft, nTop, nWidth, nHeight);
    fprintf(Logfile, "[%s] [CSVBony::setROI] Set to    x, y, w, h : %d, %d, %d, %d\n", timestamp, nNewLeft, nNewTop, nNewWidth, nNewHeight);
    fflush(Logfile);
#endif
    if(m_pframeBuffer) {
        free(m_pframeBuffer);
        m_pframeBuffer = NULL;
    }
    SVBStopVideoCapture(m_nCameraID);
    SVBCloseCamera(m_nCameraID);
    m_bCapturerunning = false;

    ret = SVBOpenCamera(m_nCameraID);
    if (ret != SVB_SUCCESS) {
        m_bConnected = false;
        return ERR_CMDFAILED;
    }
    
    // set default values
    ret = SVBSetControlValue(m_nCameraID, SVB_GAIN, m_nGain, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_EXPOSURE, m_nExposureMs, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_GAMMA, m_nGamma, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_CONTRAST, m_nContrast, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_SHARPNESS, m_nSharpness, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_SATURATION, m_nSaturation, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_WB_R, m_nWbR, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_WB_G, m_nWbG, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_WB_B, m_nWbB, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_FRAME_SPEED_MODE , m_nSpeedMode, SVB_FALSE);
    
    ret = SVBSetOutputImageType(m_nCameraID, m_nVideoMode);
    ret = SVBSetCameraMode(m_nCameraID, SVB_MODE_TRIG_SOFT);
    
    ret = SVBSetROIFormat(m_nCameraID, nNewLeft, nNewTop, nNewWidth, nNewHeight, m_nCurrentBin);
    if(ret!=SVB_SUCCESS)
        return ERR_CMDFAILED;

    m_nROILeft = nNewLeft;
    m_nROITop = nNewTop;
    m_nROIWidth = nNewWidth;
    m_nROIHeight = nNewHeight;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::setROI] new ROI set\n", timestamp);
        fflush(Logfile);
#endif
    ret = SVBStartVideoCapture(m_nCameraID);
    if(ret!=SVB_SUCCESS)
        return ERR_CMDFAILED;
    m_bCapturerunning = true;

    return nErr;
}

int CSVBony::clearROI()
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;
    
    ret = SVBSetROIFormat(m_nCameraID, 0, 0, m_nMaxWidth/m_nCurrentBin, m_nMaxHeight/m_nCurrentBin, m_nCurrentBin);
    if(ret!=SVB_SUCCESS)
        nErr =ERR_CMDFAILED;
    return nErr;
}


bool CSVBony::isFameAvailable()
{
    bool bFrameAvailable = false;
    // SVB_ERROR_CODE ret;
    // long nValue;
    // SVB_BOOL isAuto;
    
    if(m_ExposureTimer.GetElapsedSeconds() > m_dCaptureLenght)
        bFrameAvailable = true;

    /*
     This doeasn't work. nValue always report 10009
    ret =  SVBGetControlValue(m_nCameraID, SVB_EXPOSURE, &nValue, &isAuto);
    if(ret != SVB_SUCCESS) {
        return false;
    }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::isFameAvailable] nValue = %d\n", timestamp, nValue);
    fflush(Logfile);
#endif

    if(nValue == SVB_EXP_SUCCESS)
        bFrameAvailable = true;
     */

    return bFrameAvailable;
}

uint32_t CSVBony::getBitDepth()
{
    return 16; // m_nMaxBitDepth;
}


int CSVBony::getFrame(int nHeight, int nMemWidth, unsigned char* frameBuffer)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;
    int sizeReadFromCam;
    unsigned char* imgBuffer = nullptr;
    int i = 0;
    uint16_t *buf;
    int srcMemWidth;
    
    if(!frameBuffer)
        return ERR_POINTER;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::getFrame] nHeight          : %d\n", timestamp, nHeight);
    fprintf(Logfile, "[%s] [CSVBony::getFrame] nMemWidth        : %d\n", timestamp, nMemWidth);
    fprintf(Logfile, "[%s] [CSVBony::getFrame] m_nROILeft       : %d\n", timestamp, m_nROILeft);
    fprintf(Logfile, "[%s] [CSVBony::getFrame] m_nReqROILeft    : %d\n", timestamp, m_nReqROILeft);
    fprintf(Logfile, "[%s] [CSVBony::getFrame] m_nROITop        : %d\n", timestamp, m_nROITop);
    fprintf(Logfile, "[%s] [CSVBony::getFrame] m_nReqROITop     : %d\n", timestamp, m_nReqROITop);
    fprintf(Logfile, "[%s] [CSVBony::getFrame] m_nROIWidth      : %d\n", timestamp, m_nROIWidth);
    fprintf(Logfile, "[%s] [CSVBony::getFrame] m_nReqROIWidth   : %d\n", timestamp, m_nReqROIWidth);
    fprintf(Logfile, "[%s] [CSVBony::getFrame] m_nROIHeight     : %d\n", timestamp, m_nROIHeight);
    fprintf(Logfile, "[%s] [CSVBony::getFrame] m_nReqROIHeight  : %d\n", timestamp, m_nReqROIHeight);
    fprintf(Logfile, "[%s] [CSVBony::getFrame] getBitDepth()/8  : %d\n", timestamp, getBitDepth()/8);
    fflush(Logfile);
#endif

    // do we need to extract data as ROI was re-aligned to match SVBony specs of heigth%2 and width%8
    if(m_nROIWidth != m_nReqROIWidth || m_nROIHeight != m_nReqROIHeight) {
        // me need to extract the data so we allocate a buffer
        srcMemWidth = m_nROIWidth * (getBitDepth()/8);
        imgBuffer = (unsigned char*)malloc(m_nROIHeight * srcMemWidth);
    }
    else {
        imgBuffer = frameBuffer;
        srcMemWidth = nMemWidth;
    }

    sizeReadFromCam = m_nROIHeight * srcMemWidth;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::getFrame] srcMemWidth      : %d\n", timestamp,srcMemWidth);
    fprintf(Logfile, "[%s] [CSVBony::getFrame] nMemWidth        : %d\n", timestamp,nMemWidth);
    fprintf(Logfile, "[%s] [CSVBony::getFrame] sizeReadFromCam  : %d\n", timestamp,sizeReadFromCam);
    fflush(Logfile);
#endif

    ret = SVBGetVideoData(m_nCameraID, imgBuffer, sizeReadFromCam, 100);
    if(ret!=SVB_SUCCESS) {
        // wait and retry
        m_pSleeper->sleep(1000);
        ret = SVBGetVideoData(m_nCameraID, imgBuffer, sizeReadFromCam, 100);
        if(ret!=SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CSVBony::getFrame] SVBGetVideoData error %d\n", timestamp, ret);
            fflush(Logfile);
#endif
            return ERR_RXTIMEOUT;
        }
    }

    // shift data
    buf = (uint16_t *)imgBuffer;
    for(int i=0; i<sizeReadFromCam/2; i++)
        buf[i] = buf[i]<<m_nNbBitToShift;

    if(imgBuffer != frameBuffer) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::getFrame] copying (%d,%d,%d,%d) => (%d,%d,%d,%d)\n", timestamp,
                                                                                            m_nROILeft,    m_nROITop,    m_nROIWidth,    m_nROIHeight,
                                                                                            m_nReqROILeft, m_nReqROITop, m_nReqROIWidth, m_nReqROIHeight);
        fprintf(Logfile, "[%s] [CSVBony::getFrame] srcMemWidth  =>  nMemWidth   : %d => %d\n", timestamp, srcMemWidth, nMemWidth);
        fprintf(Logfile, "[%s] [CSVBony::getFrame] sizeReadFromCam  : %d\n", timestamp, sizeReadFromCam);
        fprintf(Logfile, "[%s] [CSVBony::getFrame] size to TSX      : %d\n", timestamp, nHeight * nMemWidth);
        fflush(Logfile);
#endif
        // copy every line from source buffer newly aligned into TSX buffer cutting at nMemWidth
        for(i=0; i<nHeight; i++) {
            memcpy(frameBuffer+(i*nMemWidth), imgBuffer+(i*srcMemWidth), nMemWidth);
        }
        free(imgBuffer);
    }
    return nErr;
}
