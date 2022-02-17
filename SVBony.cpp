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

    m_nROILeft = -1;
    m_nROITop = -1;
    m_nROIWidth = -1;
    m_nROIHeight = -1;

    m_nReqROILeft = -1;
    m_nReqROITop = -1;
    m_nReqROIWidth = -1;
    m_nReqROIHeight = -1;

    m_nControlNums = -1;
    m_ControlList.clear();
    
    m_nGain = -1;
    m_nExposureMs = (1 * 1000000);
    m_nGamma = -1;
    m_nGammaContrast = 0;
    m_nWbR = -1;
    m_nWbG = -1;
    m_nWbB = -1;
    m_nFlip = -1;
    m_nSpeedMode = -1; // low speed
    m_nContrast = -1;
    m_nSharpness = -1;
    m_nSaturation = -1;
    m_nAutoExposureTarget = -1;
    m_nBlackLevel = -1;

    m_bTempertureSupported = false;

    m_TemperatureTimer.Reset();

    m_dTemperature = -100;
    m_dPower = 0;
    m_dSetPoint = m_dTemperature;
    m_dCoolerEnabled = false;

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

#pragma mark - Camera access

int CSVBony::Connect(int nCameraID)
{
    int nErr = PLUGIN_OK;
    int i;
    SVB_ERROR_CODE ret;
    SVB_CONTROL_CAPS    Caps;

    m_bConnected = false;

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

    ret = SVBSetAutoSaveParam(m_nCameraID, SVB_FALSE);
    if (ret != SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::Connect] Error seting autosave to false, Error = %d\n", timestamp, ret);
        fflush(Logfile);
#endif
    }

    // turn off automatic exposure
    ret = SVBSetControlValue(m_nCameraID, SVB_EXPOSURE , 1000000, SVB_FALSE);

    m_bConnected = true;
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
        m_bConnected = false;
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
    
    for (i = 0; i < MAX_NB_BIN; i++) {
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
    switch(m_nMaxBitDepth) {
        case 8 :
            m_nVideoMode = SVB_IMG_RAW8;
            break;
        case 12 :
            m_nVideoMode = SVB_IMG_RAW12;
            break;
        case 14 :
            m_nVideoMode = SVB_IMG_RAW14;
            break;
        case 16 :
            m_nVideoMode = SVB_IMG_RAW16;
            break;
    }

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
        m_bConnected = false;
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
        m_bConnected = false;
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
        m_bConnected = false;
        return ERR_CMDFAILED;
    }

    ret = SVBGetNumOfControls(m_nCameraID, &m_nControlNums);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::Connect] m_nControlNums : %d\n", timestamp, m_nControlNums);
    fflush(Logfile);
#endif

    for (i = 0; i < m_nControlNums; i++) {
        ret = SVBGetControlCaps(m_nCameraID, i, &Caps);
        if (ret != SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            ltime = time(NULL);
            timestamp = asctime(localtime(&ltime));
            timestamp[strlen(timestamp) - 1] = 0;
            fprintf(Logfile, "[%s] [CSVBony::Connect] Error getting caps : %d\n", timestamp, ret);
            fflush(Logfile);
#endif
            continue;
        }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::Connect] ********************************************************\n", timestamp);
        fprintf(Logfile, "[%s] [CSVBony::Connect] ControlCaps Name              : %s\n", timestamp, Caps.Name);
        fprintf(Logfile, "[%s] [CSVBony::Connect] ControlCaps Description       : %s\n", timestamp, Caps.Description);
        fprintf(Logfile, "[%s] [CSVBony::Connect] ControlCaps MinValue          : %lu\n", timestamp, Caps.MinValue);
        fprintf(Logfile, "[%s] [CSVBony::Connect] ControlCaps MaxValue          : %lu\n", timestamp, Caps.MaxValue);
        fprintf(Logfile, "[%s] [CSVBony::Connect] ControlCaps IsAutoSupported   : %s\n", timestamp, Caps.IsAutoSupported?"Yes":"No");
        fprintf(Logfile, "[%s] [CSVBony::Connect] ControlCaps IsWritable        : %s\n", timestamp, Caps.IsWritable?"Yes":"No");
        fflush(Logfile);
#endif
        m_ControlList.push_back(Caps);
    }

    // get Extended properties for newer camera
#ifdef COOLER_SUPPORT
    ret = SVBGetCameraPropertyEx(m_nCameraID, &m_CameraPorpertyEx);
    m_bTempertureSupported = m_CameraPorpertyEx.bSupportControlTemp;
    m_bPulseGuidingSupported = m_CameraPorpertyEx.bSupportPulseGuide;
#endif
    
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::Connect] ********************************************************\n", timestamp);
    fprintf(Logfile, "[%s] [CSVBony::Connect] m_bTempertureSupported             : %s\n", timestamp, m_bTempertureSupported?"Yes":"No");
    fprintf(Logfile, "[%s] [CSVBony::Connect] m_bPulseGuidingSupported             : %s\n", timestamp, m_bPulseGuidingSupported?"Yes":"No");
    fflush(Logfile);
#endif

    // set default values
    setGain(m_nGain);
    setGamma(m_nGamma);
    setGammaContrast(m_nGammaContrast);
    setWB_R(m_nWbR, m_bR_Auto);
    setWB_G(m_nWbG, m_bG_Auto);
    setWB_B(m_nWbB, m_bB_Auto);
    setFlip(m_nFlip);
    setSpeedMode(m_nSpeedMode);
    setContrast(m_nContrast);
    setSharpness(m_nSharpness);
    setSaturation(m_nSaturation);
    setBlackLevel(m_nBlackLevel);

    rebuildGainList();

    ret = SVBSetOutputImageType(m_nCameraID, m_nVideoMode);
    ret = SVBSetCameraMode(m_nCameraID, SVB_MODE_TRIG_SOFT);
    
    ret = SVBStartVideoCapture(m_nCameraID);
    if(ret!=SVB_SUCCESS) {
        m_bConnected = false;
        nErr =ERR_CMDFAILED;
    }
    m_bCapturerunning = true;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::Connect] Connected : %s\n", timestamp, m_bConnected?"Yes":"No");
    fflush(Logfile);
#endif

    return nErr;
}

void CSVBony::Disconnect(bool bTrunCoolerOff)
{

    
    if(m_pframeBuffer) {
        free(m_pframeBuffer);
        m_pframeBuffer = NULL;
    }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [Disconnect] Disconnceting from CameraId = %d\n", timestamp, m_nCameraID);
    fflush(Logfile);
#endif
    if(bTrunCoolerOff)
        setCoolerTemperature(false, 15);

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

void CSVBony::getFirmwareVersion(std::string &sVersion)
{
    std::string sTmp;
    sTmp.assign(SVBGetSDKVersion());
    sVersion = "SDK version " + sTmp;
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

#pragma mark - Camera capture

int CSVBony::startCaputure(double dTime)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;
    int nTimeout;
    m_bAbort = false;

    nTimeout = 0;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::startCaputure] Start Capture\n", timestamp);
    fprintf(Logfile, "[%s] [CSVBony::startCaputure] Waiting for camera to be idle\n", timestamp);
    fflush(Logfile);
#endif
    
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

SVB_ERROR_CODE CSVBony::restartCamera()
{
    SVB_ERROR_CODE ret;

    SVBStopVideoCapture(m_nCameraID);
    if(m_pframeBuffer) {
        free(m_pframeBuffer);
        m_pframeBuffer = NULL;
    }
    SVBCloseCamera(m_nCameraID);
    m_bCapturerunning = false;

    ret = SVBOpenCamera(m_nCameraID);
    if (ret != SVB_SUCCESS)
        m_bConnected = false;

    // turn off automatic exposure
    ret = SVBSetControlValue(m_nCameraID, SVB_EXPOSURE , 1000000, SVB_FALSE);
    ret = SVBSetAutoSaveParam(m_nCameraID, SVB_FALSE);
    ret = SVBSetCameraMode(m_nCameraID, SVB_MODE_TRIG_SOFT);

    return ret;
}

#pragma mark - Camera controls

int CSVBony::getTemperture(double &dTemp, double &dPower, double &dSetPoint, bool &bEnabled)
{
    int nErr = PLUGIN_OK;
    long nMin, nMax, nValue;
    SVB_BOOL bTmp;

    if(m_TemperatureTimer.GetElapsedSeconds()<1) {
        dTemp = m_dTemperature;
        dPower = m_dPower;
        dSetPoint = m_dSetPoint;
        bEnabled = m_dCoolerEnabled;
        return nErr;
    }
    m_TemperatureTimer.Reset();
    if(m_bTempertureSupported) {
#ifdef COOLER_SUPPORT
        getControlValues(SVB_CURRENT_TEMPERATURE, nMin, nMax, nValue, bTmp);
        m_dTemperature = double(nValue)/10.0;
        getControlValues(SVB_TARGET_TEMPERATURE, nMin, nMax, nValue, bTmp);
        m_dSetPoint = double(nValue)/10.0;
        getControlValues(SVB_COOLER_POWER, nMin, nMax, nValue, bTmp);
        m_dPower = double(nValue);
        getControlValues(SVB_COOLER_ENABLE, nMin, nMax, nValue, bTmp);
        m_dCoolerEnabled = (nValue==1?true:false);
        dTemp = m_dTemperature;
        dPower = m_dPower;
        dSetPoint = m_dSetPoint;
        bEnabled = m_dCoolerEnabled;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::getTemperture] dTemp = %3.2f\n", timestamp, dTemp);
        fprintf(Logfile, "[%s] [CSVBony::getTemperture] dSetPoint = %3.2f\n", timestamp, dSetPoint);
        fprintf(Logfile, "[%s] [CSVBony::getTemperture] dPower = %3.2f\n", timestamp, dPower);
        fprintf(Logfile, "[%s] [CSVBony::getTemperture] bEnabled = %s\n", timestamp, bEnabled?"Yes":"No");
        fflush(Logfile);
#endif
#endif

    }
    else {
        dTemp = -100;
        dPower = 0;
        dSetPoint = dTemp;
        bEnabled = false;
    }
    return nErr;
}

int CSVBony::setCoolerTemperature(bool bOn, double dTemp)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE  ret;
    long nTemp;

    if(m_bTempertureSupported) {
        nTemp = int(dTemp*10);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::setCoolerTemperature] Setting temperature to %3.2f\n", timestamp, dTemp);
        fprintf(Logfile, "[%s] [CSVBony::setCoolerTemperature] Converted temperature : %ld\n", timestamp, nTemp);
        fprintf(Logfile, "[%s] [CSVBony::setCoolerTemperature] Turning cooler %s\n", timestamp, bOn?"On":"Off");
        fflush(Logfile);
#endif
#ifdef COOLER_SUPPORT
        ret = setControlValue(SVB_TARGET_TEMPERATURE, nTemp);
        if(ret != SVB_SUCCESS)
            nErr = ERR_CMDFAILED;
        ret = setControlValue(SVB_COOLER_ENABLE, bOn?1:0);
        if(ret != SVB_SUCCESS)
            nErr = ERR_CMDFAILED;
#endif

    }
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

void CSVBony::getFlip(std::string &sFlipMode)
{
    switch(m_nFlip) {
        case SVB_FLIP_NONE :
            sFlipMode.assign("None");
            break;
        case SVB_FLIP_HORIZ :
            sFlipMode.assign("Horizontal");
            break;
        case SVB_FLIP_VERT :
            sFlipMode.assign("Vertical");
            break;
        case SVB_FLIP_BOTH :
            sFlipMode.assign("both horizontal and vertical");
            break;
        default:
            sFlipMode.clear();
            break;
    }
}


void CSVBony::getGain(long &nMin, long &nMax, long &nValue)
{
    SVB_BOOL bTmp;

    getControlValues(SVB_GAIN, nMin, nMax, nValue, bTmp);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::getGain] Gain is %ld\n", timestamp, nValue);
    fflush(Logfile);
#endif
}

int CSVBony::setGain(long nGain)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;

    m_nGain = nGain;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setGain] Gain set to %ld\n", timestamp, m_nGain);
    fflush(Logfile);
#endif

    ret = setControlValue(SVB_GAIN, m_nGain);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

void CSVBony::getGamma(long &nMin, long &nMax, long &nValue)
{
    SVB_BOOL bIsAuto;
    getControlValues(SVB_GAMMA, nMin, nMax, nValue, bIsAuto);
}

int CSVBony::setGamma(long nGamma)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;
    
    m_nGamma = nGamma;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setGamma] Gamma set to %ld\n", timestamp, m_nGamma);
    fflush(Logfile);
#endif

    ret = setControlValue(SVB_GAMMA, m_nGamma);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

void CSVBony::getGammaContrast(long &nMin, long &nMax, long &nValue)
{
    SVB_BOOL bIsAuto;
    getControlValues(SVB_GAMMA_CONTRAST, nMin, nMax, nValue, bIsAuto);
}

int CSVBony::setGammaContrast(long nGammaContrast)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;

    m_nGammaContrast = nGammaContrast;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setGammaContrast] GammaContrats set to %ld\n", timestamp, m_nGammaContrast);
    fflush(Logfile);
#endif
    
    ret = setControlValue(SVB_GAMMA_CONTRAST, m_nGammaContrast);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

void CSVBony::getWB_R(long &nMin, long &nMax, long &nValue, bool &bIsAuto)
{
    SVB_BOOL bTmp;
    getControlValues(SVB_WB_R, nMin, nMax, nValue, bTmp);
    bIsAuto = (bool)bTmp;
    
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::getWB_R] WB_R is %ld\n", timestamp, nValue);
    fprintf(Logfile, "[%s] [CSVBony::getWB_R] bIsAuto is %s\n", timestamp, bIsAuto?"True":"False");
    fflush(Logfile);
#endif
    
}

int CSVBony::setWB_R(long nWB_R, bool bIsAuto)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;
    
    m_nWbR = nWB_R;
    m_bR_Auto = bIsAuto;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setWB_R] WB_R set to %ld\n", timestamp, m_nWbR);
    fprintf(Logfile, "[%s] [CSVBony::setWB_R] WB_R m_bR_Auto %s\n", timestamp, m_bR_Auto?"True":"False");
    fflush(Logfile);
#endif
    
    ret = setControlValue(SVB_WB_R, m_nWbR, bIsAuto?SVB_TRUE:SVB_FALSE);
    if(ret != SVB_SUCCESS) {
        nErr = ERR_CMDFAILED;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::setWB_R] WB_R set ERROR  %d\n", timestamp, ret);
        fflush(Logfile);
#endif
    }
    return nErr;
}

void CSVBony::getWB_G(long &nMin, long &nMax, long &nValue, bool &bIsAuto)
{
    SVB_BOOL bTmp;
    getControlValues(SVB_WB_G, nMin, nMax, nValue, bTmp);
    bIsAuto = (bool)bTmp;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::getWB_G] WB_G is %ld\n", timestamp, nValue);
    fprintf(Logfile, "[%s] [CSVBony::getWB_G] bIsAuto is %s\n", timestamp, bIsAuto?"True":"False");
    fflush(Logfile);
#endif
}

int CSVBony::setWB_G(long nWB_G, bool bIsAuto)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;

    m_nWbG = nWB_G;
    m_bG_Auto = bIsAuto;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setWB_G] WB_G set to %ld\n", timestamp, m_nWbG);
    fprintf(Logfile, "[%s] [CSVBony::setWB_G] WB_G bIsAuto %s\n", timestamp, m_bG_Auto?"True":"False");
    fflush(Logfile);
#endif
    
    ret = setControlValue(SVB_WB_G, m_nWbG, bIsAuto?SVB_TRUE:SVB_FALSE);
    if(ret != SVB_SUCCESS) {
        nErr = ERR_CMDFAILED;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::setWB_G] WB_G set ERROR  %d\n", timestamp, ret);
        fflush(Logfile);
#endif
    }
    return nErr;
}

void CSVBony::getWB_B(long &nMin, long &nMax, long &nValue, bool &bIsAuto)
{
    SVB_BOOL bTmp;
    getControlValues(SVB_WB_B, nMin, nMax, nValue, bTmp);
    bIsAuto = (bool)bTmp;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::getWB_B] WB_B is %ld\n", timestamp, nValue);
    fprintf(Logfile, "[%s] [CSVBony::getWB_B] bIsAuto is %s\n", timestamp, bIsAuto?"True":"False");
    fflush(Logfile);
#endif
}

int CSVBony::setWB_B(long nWB_B, bool bIsAuto)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;

    m_nWbB = nWB_B;
    m_bB_Auto = bIsAuto;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setWB_B] WB_B set to %ld\n", timestamp, m_nWbB);
    fprintf(Logfile, "[%s] [CSVBony::setWB_B] WB_B m_bB_Auto %s\n", timestamp, m_bB_Auto?"True":"False");
    fflush(Logfile);
#endif

    ret = setControlValue(SVB_WB_B, m_nWbB, bIsAuto?SVB_TRUE:SVB_FALSE);
    if(ret != SVB_SUCCESS){
        nErr = ERR_CMDFAILED;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::setWB_B] WB_B set ERROR  %d\n", timestamp, ret);
        fflush(Logfile);
#endif
    }

    return nErr;
}

void CSVBony::getFlip(long &nMin, long &nMax, long &nValue)
{
    SVB_BOOL bIsAuto;
    getControlValues(SVB_FLIP, nMin, nMax, nValue, bIsAuto);
}

int CSVBony::setFlip(long nFlip)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;
    
    m_nFlip = nFlip;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setFlip] Flip set to %ld\n", timestamp, m_nFlip);
    fflush(Logfile);
#endif
    
    ret = setControlValue(SVB_FLIP, m_nFlip);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

void CSVBony::getSpeedMode(long &nMin, long &nMax, long &nValue)
{
    SVB_BOOL bIsAuto = SVB_FALSE;
    getControlValues(SVB_FRAME_SPEED_MODE, nMin, nMax, nValue, bIsAuto);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::getSpeedMode] speed mode set to %ld\n", timestamp, nValue);
    fprintf(Logfile, "[%s] [CSVBony::getSpeedMode] speed mode nMin %ld\n", timestamp, nMin);
    fprintf(Logfile, "[%s] [CSVBony::getSpeedMode] speed mode nMax %ld\n", timestamp, nMax);
    fflush(Logfile);
#endif

}

int CSVBony::setSpeedMode(long nSpeed)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;
    
    m_nSpeedMode = nSpeed;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setSpeedMode] SpeedMode set to %ld\n", timestamp, m_nSpeedMode);
    fflush(Logfile);
#endif
    
    ret = setControlValue(SVB_FRAME_SPEED_MODE, m_nSpeedMode);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

void CSVBony::getContrast(long &nMin, long &nMax, long &nValue)
{
    SVB_BOOL bIsAuto;
    getControlValues(SVB_CONTRAST, nMin, nMax, nValue, bIsAuto);
}

int CSVBony::setContrast(long nContrast)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;
    
    m_nContrast = nContrast;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setContrast] Contrast set to %ld\n", timestamp, m_nContrast);
    fflush(Logfile);
#endif

    ret = setControlValue(SVB_CONTRAST, m_nContrast);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

void CSVBony::getSharpness(long &nMin, long &nMax, long &nValue)
{
    SVB_BOOL bIsAuto;
    getControlValues(SVB_SHARPNESS, nMin, nMax, nValue, bIsAuto);
}

int CSVBony::setSharpness(long nSharpness)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;

    m_nSharpness = nSharpness;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setSharpness] Sharpness set to %ld\n", timestamp, m_nSharpness);
    fflush(Logfile);
#endif
    
    ret = setControlValue(SVB_SHARPNESS, m_nSharpness);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

void CSVBony::getSaturation(long &nMin, long &nMax, long &nValue)
{
    SVB_BOOL bIsAuto;
    getControlValues(SVB_SATURATION, nMin, nMax, nValue, bIsAuto);
}

int CSVBony::setSaturation(long nSaturation)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;

    m_nSaturation = nSaturation;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setSaturation] Saturation set to %ld\n", timestamp, m_nSaturation);
    fflush(Logfile);
#endif
    
    ret = setControlValue(SVB_SATURATION, m_nSaturation);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

void CSVBony::getBlackLevel(long &nMin, long &nMax, long &nValue)
{
    SVB_BOOL bIsAuto;
    getControlValues(SVB_BLACK_LEVEL, nMin, nMax, nValue, bIsAuto);
}

int CSVBony::setBlackLevel(long nBlackLevel)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;
    
    m_nBlackLevel = nBlackLevel;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setBlackLevel] Black level set to %ld\n", timestamp, m_nBlackLevel);
    fflush(Logfile);
#endif
    
    ret = setControlValue(SVB_BLACK_LEVEL, m_nBlackLevel);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;
 
    return nErr;
}

SVB_ERROR_CODE CSVBony::setControlValue(SVB_CONTROL_TYPE nControlType, long nValue, SVB_BOOL bAuto)
{
    SVB_ERROR_CODE ret;
    
    if(!m_bConnected)
        return SVB_SUCCESS;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::setControlValue] nControlType = %d\n", timestamp, nControlType);
    fprintf(Logfile, "[%s] [CSVBony::setControlValue] nValue       = %ld\n", timestamp, nValue);
    fprintf(Logfile, "[%s] [CSVBony::setControlValue] bAuto        = %s\n", timestamp, bAuto == SVB_TRUE? "Yes":"No");
    fprintf(Logfile, "[%s] [CSVBony::setControlValue] ************************\n\n", timestamp);
    fflush(Logfile);
#endif

    ret = SVBSetControlValue(m_nCameraID, nControlType, nValue, bAuto);
    if(ret != SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::setControlValue] Error setting value !!!! : %d\n", timestamp, ret);
        fflush(Logfile);
#endif

    }
    return ret;
}


SVB_ERROR_CODE CSVBony::getControlValues(SVB_CONTROL_TYPE nControlType, long &nMin, long &nMax, long &nValue, SVB_BOOL &bIsAuto)
{
    SVB_ERROR_CODE ret;
    int i;
    
    nValue = -1;
    nMin = -1;
    nMax = -1;
    
    ret = SVBGetControlValue(m_nCameraID, nControlType, &nValue, &bIsAuto);
    if(ret != SVB_SUCCESS)
        return ret;
    // look for min,max in the control caps
    for(i = 0; i< m_nControlNums; i++) {
        if(m_ControlList.at(i).ControlType == nControlType) {
            nMin = m_ControlList.at(i).MinValue;
            nMax = m_ControlList.at(i).MaxValue;
            break;
        }
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    ltime = time(NULL);
    timestamp = asctime(localtime(&ltime));
    timestamp[strlen(timestamp) - 1] = 0;
    fprintf(Logfile, "[%s] [CSVBony::getControlValues] nControlType = %d\n", timestamp, nControlType);
    fprintf(Logfile, "[%s] [CSVBony::getControlValues] nMin         = %ld\n", timestamp, nMin);
    fprintf(Logfile, "[%s] [CSVBony::getControlValues] nMax         = %ld\n", timestamp, nMax);
    fprintf(Logfile, "[%s] [CSVBony::getControlValues] nValue       = %ld\n", timestamp, nValue);
    fprintf(Logfile, "[%s] [CSVBony::getControlValues] bIsAuto      = %s\n", timestamp, bIsAuto == SVB_TRUE? "Yes":"No");
    fprintf(Logfile, "[%s] [CSVBony::getControlValues] ************************\n\n", timestamp);
    fflush(Logfile);
#endif

    return ret;
}

#pragma mark - Camera frame

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
    
    fprintf(Logfile, "[%s] [CSVBony::setROI] nNewLeft       : %d\n", timestamp, nNewLeft);
    fprintf(Logfile, "[%s] [CSVBony::setROI] nNewTop        : %d\n", timestamp, nNewTop);
    fprintf(Logfile, "[%s] [CSVBony::setROI] nNewWidth      : %d\n", timestamp, nNewWidth);
    fprintf(Logfile, "[%s] [CSVBony::setROI] nNewHeight     : %d\n", timestamp, nNewHeight);

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

    ret = SVBSetROIFormat(m_nCameraID, nNewLeft, nNewTop, nNewWidth, nNewHeight, m_nCurrentBin);
    if(ret!=SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::setROI] Setting new ROI failled\n", timestamp);
        fflush(Logfile);
#endif
        return ERR_CMDFAILED;
    }
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

    if(m_ExposureTimer.GetElapsedSeconds() > m_dCaptureLenght)
        bFrameAvailable = true;

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
    int timeout = 0;

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

    while(true) {
        ret = SVBGetVideoData(m_nCameraID, imgBuffer, sizeReadFromCam, 100);
        if(ret!=SVB_SUCCESS) {
            timeout++;
            if(timeout > MAX_DATA_TIMEOUT) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                ltime = time(NULL);
                timestamp = asctime(localtime(&ltime));
                timestamp[strlen(timestamp) - 1] = 0;
                fprintf(Logfile, "[%s] [CSVBony::getFrame] SVBGetVideoData error %d\n", timestamp, ret);
                fflush(Logfile);
#endif
                return ERR_RXTIMEOUT;
            }
            continue;
        }
        else
            break; // we have an image
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


#pragma mark - Camera relay


int CSVBony::RelayActivate(const int nXPlus, const int nXMinus, const int nYPlus, const int nYMinus, const bool bSynchronous, const bool bAbort)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;
    SVB_BOOL bCanPulse = SVB_FALSE;
    SVB_GUIDE_DIRECTION nDir = SVB_GUIDE_NORTH;
    int nDurration = 0;
    
    ret = SVBCanPulseGuide(m_nCameraID, &bCanPulse);
    if(bCanPulse) {
        if(!bAbort) {
            nDurration = 5000;
            if(nXPlus != 0 && nXMinus ==0)
                nDir = SVB_GUIDE_WEST;
            if(nXPlus == 0 && nXMinus !=0)
                nDir = SVB_GUIDE_EAST;
            
            if(nYPlus != 0 && nYMinus ==0)
                nDir = SVB_GUIDE_SOUTH;
            if(nYPlus == 0 && nYMinus !=0)
                nDir = SVB_GUIDE_NORTH;
        }
        else {
            nDurration = 0; // should stop the pulse
            nDir = SVB_GUIDE_NORTH;
        }
        ret = SVBPulseGuide(m_nCameraID, nDir, nDurration); // hopefully this is not blocking !!! 
        if(ret!=SVB_SUCCESS)
            nErr =ERR_CMDFAILED;
    }
    else
        nErr = ERR_NOT_IMPL;
    return nErr;

}

#pragma mark - helper functions

void CSVBony::buildGainList(long nMin, long nMax, long nValue)
{
    long i = 0;
    int nStep = 1;
    m_GainList.clear();
    m_nNbGainValue = 0;

    if(nMin != nValue) {
        m_GainList.push_back(std::to_string(nValue));
        m_nNbGainValue++;
    }

    nStep = int(float(nMax-nMin)/10);
    for(i=nMin; i<nMax; i+=nStep) {
        m_GainList.push_back(std::to_string(i));
        m_nNbGainValue++;
    }
    m_GainList.push_back(std::to_string(nMax));
    m_nNbGainValue++;
}
int CSVBony::getNbGainInList()
{
    return m_nNbGainValue;
}

void CSVBony::rebuildGainList()
{
    long nMin, nMax, nVal;
    getGain(nMin, nMax, nVal);
    buildGainList(nMin, nMax, nVal);
}

std::string CSVBony::getGainFromListAtIndex(int nIndex)
{
    if(nIndex<m_GainList.size())
        return m_GainList.at(nIndex);
    else
        return std::string("N/A");
}
