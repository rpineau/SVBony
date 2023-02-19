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
    m_nCameraNum = 0;
    m_nCameraID = 0;
    m_bAbort = false;
    m_nCurrentBin = 1;
    m_nVideoMode = SVB_IMG_RAW12;
    m_nNbBitToShift = 4;
    m_dCaptureLenght = 0;
    m_bCapturerunning = false;
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
    m_bBadPixelCorrectionEnabled = false;

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
    m_sLogFile.open(m_sLogfilePath, std::ios::out |std::ios::trunc);
#endif

    std::string sSDKVersion;
    getFirmwareVersion(sSDKVersion);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CSVBony] Version " << std::fixed << std::setprecision(2) << PLUGIN_VERSION << " build " << __DATE__ << " " << __TIME__ << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CSVBony] " << sSDKVersion << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [CSVBony] Constructor Called." << std::endl;
    m_sLogFile.flush();
#endif


    std::vector<camera_info_t> tCameraIdList;
    listCamera(tCameraIdList);
}

CSVBony::~CSVBony()
{
    if(m_pframeBuffer)
        free(m_pframeBuffer);
}

#pragma mark - Camera access
void CSVBony::setUserConf(bool bUserConf)
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]" << " [setUserConf]  Set m_bSetUserConf to : " << (bUserConf?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif
    m_bSetUserConf = bUserConf;
}

int CSVBony::Connect(int nCameraID)
{
    int nErr = PLUGIN_OK;
    int i;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_CONTROL_CAPS    Caps;
    long nMin, nMax;

    m_bConnected = false;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Connect called" << std::endl;
    m_sLogFile.flush();
#endif


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

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Connecting to camera ID  " << m_nCameraID << " Serial " << m_sCameraSerial << std::endl;
    m_sLogFile.flush();
#endif

    ret = SVBOpenCamera(m_nCameraID);
    if (ret != SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Error connecting to camera ID  " << m_nCameraID << " Serial " << m_sCameraSerial << " , ret = " << ret << std::endl;
        m_sLogFile.flush();
#endif
        return ERR_NORESPONSE;
        }

    m_bConnected = true;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    SVB_BOOL bNeedUpgrade;
    char minVersion[1024];
    SVBIsCameraNeedToUpgrade(m_nCameraID, &bNeedUpgrade, minVersion);
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] bNeedUpgrade : " << (bNeedUpgrade?"Yes":"No") << " , min Version " << minVersion << std::endl;
    m_sLogFile.flush();
#endif

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Disabling autosave params" << std::endl;
    m_sLogFile.flush();
#endif

    ret = SVBSetAutoSaveParam(m_nCameraID, SVB_FALSE);
    if (ret != SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Error seting autosave to false, Error = " << ret << std::endl;
        m_sLogFile.flush();
#endif
    }


#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Turn off autoexposure" << std::endl;
    m_sLogFile.flush();
#endif

    ret = SVBSetControlValue(m_nCameraID, SVB_EXPOSURE , 1000000, SVB_FALSE);

    getCameraNameFromID(m_nCameraID, m_sCameraName);

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Connected to camera ID  " << m_nCameraID << " Serial " << m_sCameraSerial << std::endl;
    m_sLogFile.flush();
#endif

    ret = SVBGetCameraProperty(m_nCameraID, &m_cameraProperty);
    if (ret != SVB_SUCCESS)
    {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] SVBGetCameraProperty Error = " << ret << std::endl;
        m_sLogFile.flush();
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

    if(m_cameraProperty.MaxBitDepth >8) {
        m_nVideoMode = SVB_IMG_RAW10;
        m_nNbBitToShift = 6;
        for(i=0; i<8;i++) {
            if(m_cameraProperty.SupportedVideoFormat[i] == SVB_IMG_END)
                break;
            if(m_cameraProperty.SupportedVideoFormat[i] == SVB_IMG_RAW12 && m_nVideoMode < SVB_IMG_RAW12 ) {
                m_nVideoMode = SVB_IMG_RAW12;
                m_nNbBitToShift = 4;
            }
            if(m_cameraProperty.SupportedVideoFormat[i] == SVB_IMG_RAW14 && m_nVideoMode < SVB_IMG_RAW14 ) {
                m_nVideoMode = SVB_IMG_RAW14;
                m_nNbBitToShift = 2;
            }
            if(m_cameraProperty.SupportedVideoFormat[i] == SVB_IMG_RAW16 && m_nVideoMode < SVB_IMG_RAW16 ) {
                m_nVideoMode = SVB_IMG_RAW16;
                m_nNbBitToShift = 0;
                break;
            }
        }
    }
    else {
        m_nVideoMode = SVB_IMG_RAW8;
        m_nNbBitToShift = 8;
    }

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
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Camera properties :" << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_nMaxWidth       :" << m_nMaxWidth << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_nMaxHeight      :" << m_nMaxHeight << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_bIsColorCam     :" << (m_bIsColorCam?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_nBayerPattern   :" << m_nBayerPattern << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_nMaxBitDepth    :" << m_nMaxBitDepth << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_nVideoMode      :" << m_nVideoMode << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_nNbBitToShift   :" << m_nNbBitToShift << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_nNbBin          :" << m_nNbBin << std::endl;
    m_sLogFile.flush();
#endif

    float pixelSize;
    ret = SVBGetSensorPixelSize(m_nCameraID, &pixelSize);
    if (ret != SVB_SUCCESS && ret != SVB_ERROR_UNKNOW_SENSOR_TYPE)
    {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] SVBGetSensorPixelSize Error = " << ret << std::endl;
        m_sLogFile.flush();
#endif
        SVBCloseCamera(m_nCameraID);
        m_bConnected = false;
        return ERR_CMDFAILED;
    }
    if(ret == SVB_ERROR_UNKNOW_SENSOR_TYPE) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_dPixelSize Unknown, Error = " << ret << std::endl;
        m_sLogFile.flush();
#endif
        m_dPixelSize = 0;
    }
    else {
        m_dPixelSize = (double)pixelSize;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_dPixelSize = " << m_dPixelSize << std::endl;
        m_sLogFile.flush();
#endif
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Setting ROI to max size." << std::endl;
    m_sLogFile.flush();
#endif

    ret = SVBSetROIFormat(m_nCameraID, 0, 0, m_nMaxWidth, m_nMaxHeight, 1);
    if (ret != SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] SVBSetROIFormat  Error = " << ret << std::endl;
        m_sLogFile.flush();
#endif
        SVBCloseCamera(m_nCameraID);
        m_bConnected = false;
        return ERR_CMDFAILED;
    }
    ret = SVBGetROIFormat(m_nCameraID, &m_nROILeft, &m_nROITop, &m_nROIWidth, &m_nROIHeight, &m_nCurrentBin);
    if (ret != SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] SVBGetROIFormat  Error = " << ret << std::endl;
        m_sLogFile.flush();
#endif
        SVBCloseCamera(m_nCameraID);
        m_bConnected = false;
        return ERR_CMDFAILED;
    }

    ret = SVBGetNumOfControls(m_nCameraID, &m_nControlNums);

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_nControlNums : " << m_nControlNums << std::endl;
    m_sLogFile.flush();
#endif

    for (i = 0; i < m_nControlNums; i++) {
        ret = SVBGetControlCaps(m_nCameraID, i, &Caps);
        if (ret != SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
            m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Error getting caps "<< i <<", Error = " << ret << std::endl;
            m_sLogFile.flush();
#endif
            continue;
        }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] ********************************************************" << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] ControlCaps Name            : " << Caps.Name << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] ControlCaps Description     : " << Caps.Description << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] ControlCaps MinValue        : " << Caps.MinValue << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] ControlCaps MaxValue        : " << Caps.MaxValue << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] ControlCaps IsAutoSupported : " << (Caps.IsAutoSupported?"Yes":"No") << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] ControlCaps IsWritable      : " << (Caps.IsWritable?"Yes":"No") << std::endl;
        m_sLogFile.flush();
#endif

        m_ControlList.push_back(Caps);
    }

    // get Extended properties for newer camera
    ret = SVBGetCameraPropertyEx(m_nCameraID, &m_CameraPorpertyEx);
    m_bTempertureSupported = m_CameraPorpertyEx.bSupportControlTemp;
    m_bPulseGuidingSupported = m_CameraPorpertyEx.bSupportPulseGuide;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] ********************************************************" << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_bTempertureSupported   : " << (m_bTempertureSupported?"Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] m_bPulseGuidingSupported : " << (m_bPulseGuidingSupported?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif


#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Setting default properties" << std::endl;
    m_sLogFile.flush();
#endif

    if(m_bSetUserConf) {
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
        setBadPixelCorrection(m_bBadPixelCorrectionEnabled);
    }
    else
    {
        getGain(nMin, nMax, m_nGain);
        getGamma(nMin, nMax, m_nGamma);
        getGammaContrast(nMin, nMax, m_nGammaContrast);
        getWB_R(nMin, nMax,m_nWbR, m_bR_Auto);
        getWB_G(nMin, nMax,m_nWbG, m_bG_Auto);
        getWB_B(nMin, nMax,m_nWbB, m_bB_Auto);
        getFlip(nMin, nMax, m_nFlip);
        getSpeedMode(nMin, nMax, m_nSpeedMode);
        getContrast(nMin, nMax, m_nContrast);
        getSharpness(nMin, nMax, m_nSharpness);
        getSaturation(nMin, nMax, m_nSaturation);
        getBlackLevel(nMin, nMax, m_nBlackLevel);
        setBadPixelCorrection(m_bBadPixelCorrectionEnabled);
    }

    rebuildGainList();

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Setting amera image type and mode"  << std::endl;
    m_sLogFile.flush();
#endif
    ret = SVBSetOutputImageType(m_nCameraID, m_nVideoMode);
    ret = SVBSetCameraMode(m_nCameraID, SVB_MODE_TRIG_SOFT);


    ret = SVBStartVideoCapture(m_nCameraID);
    if(ret!=SVB_SUCCESS) {
        m_bConnected = false;
        nErr =ERR_CMDFAILED;
    }
    m_bCapturerunning = true;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Connect] Connected : " << ( m_bConnected?"Yes":"No") << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}

void CSVBony::Disconnect(bool bTrunCoolerOff)
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [Disconnect] Disconnceting from camera ID  " << m_nCameraID << " Serial " << m_sCameraSerial << std::endl;
    m_sLogFile.flush();
#endif
    if(bTrunCoolerOff)
        setCoolerTemperature(false, 15);

    SVBStopVideoCapture(m_nCameraID);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    SVBCloseCamera(m_nCameraID);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_bConnected = false;

    if(m_pframeBuffer) {
        free(m_pframeBuffer);
        m_pframeBuffer = NULL;
    }
}

void CSVBony::setCameraId(int nCameraId)
{
    m_nCameraID = nCameraId;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setCameraId] Camera ID : " << m_nCameraID << std::endl;
    m_sLogFile.flush();
#endif
}

void CSVBony::getCameraId(int &nCameraId)
{
    nCameraId = m_nCameraID;
}

void CSVBony::getCameraIdFromSerial(int &nCameraId, std::string sSerial)
{
    SVB_ERROR_CODE ret = SVB_SUCCESS;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getCameraIdFromSerial] sSerial ID : " << sSerial << std::endl;
    m_sLogFile.flush();
#endif

    if(!m_bConnected) {
        nCameraId = 0;
        m_nCameraNum = SVBGetNumOfConnectedCameras();
        for (int i = 0; i < m_nCameraNum; i++)
        {
            ret = SVBGetCameraInfo(&m_CameraInfo, i);
            if (ret == SVB_SUCCESS)
            {
                if(sSerial==m_CameraInfo.CameraSN) {
                    nCameraId = m_CameraInfo.CameraID;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getCameraIdFromSerial] nCameraId : " << nCameraId << std::endl;
                    m_sLogFile.flush();
#endif
                }
            }
        }
    }
    else {
        nCameraId = m_CameraInfo.CameraID;
    }
}

void CSVBony::getCameraSerialFromID(int nCameraId, std::string &sSerial)
{

    SVB_ERROR_CODE ret = SVB_SUCCESS;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getCameraSerialFromID] nCameraId : " << nCameraId << std::endl;
    m_sLogFile.flush();
#endif
    if(!m_bConnected) {
        m_nCameraNum = SVBGetNumOfConnectedCameras();
        for (int i = 0; i < m_nCameraNum; i++)
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
    }
    else {
        sSerial.assign(m_CameraInfo.CameraSN);
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getCameraSerialFromID] camera id " << nCameraId << " SN : " << sSerial << std::endl;
    m_sLogFile.flush();
#endif
}

void CSVBony::getCameraNameFromID(int nCameraId, std::string &sName)
{

    SVB_ERROR_CODE ret = SVB_SUCCESS;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getCameraNameFromID] nCameraId : " << nCameraId << std::endl;
    m_sLogFile.flush();
#endif
    if(!m_bConnected) {
        m_nCameraNum = SVBGetNumOfConnectedCameras();
        for (int i = 0; i < m_nCameraNum; i++)
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
    }
    else {
        sName.assign(m_CameraInfo.FriendlyName);
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getCameraNameFromID] camera id " << nCameraId << " name : " << sName << std::endl;
    m_sLogFile.flush();
#endif
}

void CSVBony::setCameraSerial(std::string sSerial)
{
    m_sCameraSerial.assign(sSerial);
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setCameraSerial] m_sCameraSerial : " << m_sCameraSerial << std::endl;
    m_sLogFile.flush();
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
    SVB_ERROR_CODE ret = SVB_SUCCESS;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [listCamera] called" << std::endl;
    m_sLogFile.flush();
#endif

    cameraIdList.clear();

    // list camera connected to the system
    if(!m_bConnected) {
        m_nCameraNum = SVBGetNumOfConnectedCameras();
        for (int i = 0; i < m_nCameraNum; i++)
        {
            ret = SVBGetCameraInfo(&m_CameraInfo, i);
            if (ret == SVB_SUCCESS)
            {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [listCamera] Friendly name : " << m_CameraInfo.FriendlyName << std::endl;
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [listCamera] Port type     : " << m_CameraInfo.PortType << std::endl;
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [listCamera] SN            : " << m_CameraInfo.CameraSN << std::endl;
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [listCamera] Device ID     : " << m_CameraInfo.DeviceID << std::endl;
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [listCamera] Camera ID     : " << m_CameraInfo.CameraID << std::endl;
                m_sLogFile.flush();
#endif
                tCameraInfo.cameraId = m_CameraInfo.CameraID;
                strncpy(tCameraInfo.model, m_CameraInfo.FriendlyName, BUFFER_LEN);
                strncpy((char *)tCameraInfo.Sn.id, m_CameraInfo.CameraSN, sizeof(SVB_ID));

                cameraIdList.push_back(tCameraInfo);
            }
        }
    }
    else {
        tCameraInfo.cameraId = m_CameraInfo.CameraID;
        strncpy(tCameraInfo.model, m_CameraInfo.FriendlyName, BUFFER_LEN);
        strncpy((char *)tCameraInfo.Sn.id, m_CameraInfo.CameraSN, sizeof(SVB_ID));
        cameraIdList.push_back(tCameraInfo);
    }
    return nErr;
}

void CSVBony::getFirmwareVersion(std::string &sVersion)
{
    std::stringstream ssTmp;
    std::string sTmp;
    char szFirmware[256];

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFirmwareVersion] called " << std::endl;
    m_sLogFile.flush();
#endif

    ssTmp << "SDK "<<SVBGetSDKVersion();
    if(m_bConnected) {
        SVBGetCameraFirmwareVersion(m_nCameraID,szFirmware);
        ssTmp << ", Firmware " << szFirmware;
    }
    sVersion.assign(ssTmp.str());

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFirmwareVersion] Firmware : " << szFirmware << std::endl;
    m_sLogFile.flush();
#endif

}

int CSVBony::getNumBins()
{
    return m_nNbBin;
}

int CSVBony::getBinFromIndex(int nIndex)
{
    if(!m_bConnected)
        return 1;

    if(nIndex>(m_nNbBin-1))
        return 1;

    return m_SupportedBins[nIndex];        
}

#pragma mark - Camera capture

int CSVBony::startCaputure(double dTime)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    int nTimeout;
    m_bAbort = false;

    nTimeout = 0;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [startCaputure] called " << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [startCaputure] Starting capture" << std::endl;
    m_sLogFile.flush();
#endif
    // set exposure time (s -> us)
    ret = SVBSetControlValue(m_nCameraID, SVB_EXPOSURE , long(dTime * 1000000), SVB_FALSE);
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
    SVB_ERROR_CODE ret = SVB_SUCCESS;

    ret = SVBStopVideoCapture(m_nCameraID);
    if(ret!=SVB_SUCCESS)
        nErr =ERR_CMDFAILED;

    m_bCapturerunning = false;
    
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [stopCaputure] Capture stopped" << std::endl;
    m_sLogFile.flush();
#endif
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::this_thread::yield();
    return nErr;
}

void CSVBony::abortCapture(void)
{
    m_bAbort = true;
    stopCaputure();
}

SVB_ERROR_CODE CSVBony::restartCamera()
{
    SVB_ERROR_CODE ret = SVB_SUCCESS;

    SVBStopVideoCapture(m_nCameraID);
    SVBCloseCamera(m_nCameraID);
    m_bCapturerunning = false;
    if(m_pframeBuffer) {
        free(m_pframeBuffer);
        m_pframeBuffer = NULL;
    }

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
    SVB_ERROR_CODE  ret;
    if(m_TemperatureTimer.GetElapsedSeconds()<1) {
        dTemp = m_dTemperature;
        dPower = m_dPower;
        dSetPoint = m_dSetPoint;
        bEnabled = m_dCoolerEnabled;
        return nErr;
    }
    m_TemperatureTimer.Reset();
    ret = getControlValues(SVB_CURRENT_TEMPERATURE, nMin, nMax, nValue, bTmp);
    if(ret != SVB_SUCCESS) {
        dTemp = -100;
        dPower = 0;
        dSetPoint = dTemp;
        bEnabled = false;
        return nErr;
    }
    m_dTemperature = double(nValue)/10.0;
    dTemp = m_dTemperature;
    if(m_bTempertureSupported) {
        getControlValues(SVB_TARGET_TEMPERATURE, nMin, nMax, nValue, bTmp);
        m_dSetPoint = double(nValue)/10.0;
        getControlValues(SVB_COOLER_POWER, nMin, nMax, nValue, bTmp);
        m_dPower = double(nValue);
        getControlValues(SVB_COOLER_ENABLE, nMin, nMax, nValue, bTmp);
        m_dCoolerEnabled = (nValue==1?true:false);
        dPower = m_dPower;
        dSetPoint = m_dSetPoint;
        bEnabled = m_dCoolerEnabled;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getTemperture] dTemp     : " << dTemp << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getTemperture] dSetPoint : " << dSetPoint << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getTemperture] dPower    : " << dPower << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getTemperture] bEnabled  : " << (bEnabled?"Yes":"No") << std::endl;
        m_sLogFile.flush();
#endif
    }
    else {
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
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setCoolerTemperature] Setting temperature to : " << dTemp << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setCoolerTemperature] Converted temperature  : " << nTemp << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setCoolerTemperature] Turning cooler         : " << (bOn?"On":"Off") << std::endl;
        m_sLogFile.flush();
#endif
        ret = setControlValue(SVB_TARGET_TEMPERATURE, nTemp);
        if(ret != SVB_SUCCESS)
            nErr = ERR_CMDFAILED;
        ret = setControlValue(SVB_COOLER_ENABLE, bOn?1:0);
        if(ret != SVB_SUCCESS)
            nErr = ERR_CMDFAILED;
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
    else
        sBayerPattern.assign("MONO");
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


int CSVBony::getGain(long &nMin, long &nMax, long &nValue)
{
    int nErr = PLUGIN_OK;
    SVB_BOOL bTmp;
    SVB_ERROR_CODE ret = SVB_SUCCESS;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getGain] Called"<< std::endl;
    m_sLogFile.flush();
#endif

    ret = getControlValues(SVB_GAIN, nMin, nMax, nValue, bTmp);
    if(ret) {
        return VAL_NOT_AVAILABLE;
    }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getGain] Gain is " << nValue << std::endl;
    m_sLogFile.flush();
#endif
    return nErr;
}

int CSVBony::setGain(long nGain)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;

    m_nGain = nGain;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getGain] Gain set to " << nGain << std::endl;
    m_sLogFile.flush();
#endif
    ret = setControlValue(SVB_GAIN, m_nGain);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

int CSVBony::getGamma(long &nMin, long &nMax, long &nValue)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_BOOL bIsAuto;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getGamma] Called"<< std::endl;
    m_sLogFile.flush();
#endif
    ret = getControlValues(SVB_GAMMA, nMin, nMax, nValue, bIsAuto);
    if(ret) {
        return VAL_NOT_AVAILABLE;
    }
    return nErr;
}

int CSVBony::setGamma(long nGamma)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    
    m_nGamma = nGamma;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setGamma] Gamma set to " << nGamma << std::endl;
    m_sLogFile.flush();
#endif

    ret = setControlValue(SVB_GAMMA, m_nGamma);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

int CSVBony::getGammaContrast(long &nMin, long &nMax, long &nValue)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_BOOL bIsAuto;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getGammaContrast] Called"<< std::endl;
    m_sLogFile.flush();
#endif

    ret = getControlValues(SVB_GAMMA_CONTRAST, nMin, nMax, nValue, bIsAuto);
    if(ret != SVB_SUCCESS) {
        return VAL_NOT_AVAILABLE;
    }
    return nErr;
}

int CSVBony::setGammaContrast(long nGammaContrast)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;

    m_nGammaContrast = nGammaContrast;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setGammaContrast] GammaContrats set to " << nGammaContrast << std::endl;
    m_sLogFile.flush();
#endif
    ret = setControlValue(SVB_GAMMA_CONTRAST, m_nGammaContrast);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

int CSVBony::getWB_R(long &nMin, long &nMax, long &nValue, bool &bIsAuto)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_BOOL bTmp;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getWB_R] Called"<< std::endl;
    m_sLogFile.flush();
#endif
    ret = getControlValues(SVB_WB_R, nMin, nMax, nValue, bTmp);
    if(ret != SVB_SUCCESS) {
        return VAL_NOT_AVAILABLE;
    }

    bIsAuto = (bool)bTmp;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getWB_R] WB_R is " << nValue << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getWB_R] bIsAuto is " << (bIsAuto?"True":"False") << std::endl;
    m_sLogFile.flush();
#endif

    return nErr;
}

int CSVBony::setWB_R(long nWB_R, bool bIsAuto)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    
    m_nWbR = nWB_R;
    m_bR_Auto = bIsAuto;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setWB_R] WB_R set to " << m_nWbR << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setWB_R] WB_R bIsAuto set to " << (m_bR_Auto?"True":"False") << std::endl;
    m_sLogFile.flush();
#endif
    ret = setControlValue(SVB_WB_R, m_nWbR, bIsAuto?SVB_TRUE:SVB_FALSE);
    if(ret != SVB_SUCCESS) {
        nErr = ERR_CMDFAILED;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setWB_R] WB_R set ERROR : " << ret << std::endl;
        m_sLogFile.flush();
#endif
    }
    return nErr;
}

int CSVBony::getWB_G(long &nMin, long &nMax, long &nValue, bool &bIsAuto)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_BOOL bTmp;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getWB_G] Called"<< std::endl;
    m_sLogFile.flush();
#endif
    ret = getControlValues(SVB_WB_G, nMin, nMax, nValue, bTmp);
    if(ret != SVB_SUCCESS) {
        return VAL_NOT_AVAILABLE;
    }

    bIsAuto = (bool)bTmp;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getWB_G] WB_G is " << nValue << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getWB_G] bIsAuto is " << (bIsAuto?"True":"False") << std::endl;
    m_sLogFile.flush();
#endif
    return nErr;
}

int CSVBony::setWB_G(long nWB_G, bool bIsAuto)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;

    m_nWbG = nWB_G;
    m_bG_Auto = bIsAuto;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setWB_G] WB_G set to " << m_nWbG << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setWB_G] WB_G bIsAuto set to " << (m_bG_Auto?"True":"False") << std::endl;
    m_sLogFile.flush();
#endif
    ret = setControlValue(SVB_WB_G, m_nWbG, bIsAuto?SVB_TRUE:SVB_FALSE);
    if(ret != SVB_SUCCESS) {
        nErr = ERR_CMDFAILED;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setWB_G] WB_G set ERROR : " << ret << std::endl;
        m_sLogFile.flush();
#endif
    }
    return nErr;
}

int CSVBony::getWB_B(long &nMin, long &nMax, long &nValue, bool &bIsAuto)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_BOOL bTmp;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getWB_B] Called"<< std::endl;
    m_sLogFile.flush();
#endif
    ret = getControlValues(SVB_WB_B, nMin, nMax, nValue, bTmp);
    if(ret != SVB_SUCCESS) {
        return VAL_NOT_AVAILABLE;
    }

    bIsAuto = (bool)bTmp;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getWB_B] WB_B is " << nValue << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getWB_B] bIsAuto is " << (bIsAuto?"True":"False") << std::endl;
    m_sLogFile.flush();
#endif
    return nErr;
}

int CSVBony::setWB_B(long nWB_B, bool bIsAuto)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;

    m_nWbB = nWB_B;
    m_bB_Auto = bIsAuto;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setWB_B] WB_B set to " << m_nWbB << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setWB_B] WB_B bIsAuto set to " << (m_bB_Auto?"True":"False") << std::endl;
    m_sLogFile.flush();
#endif
    ret = setControlValue(SVB_WB_B, m_nWbB, bIsAuto?SVB_TRUE:SVB_FALSE);
    if(ret != SVB_SUCCESS){
        nErr = ERR_CMDFAILED;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setWB_B] WB_B set ERROR : " << ret << std::endl;
        m_sLogFile.flush();
#endif
    }

    return nErr;
}

int CSVBony::getFlip(long &nMin, long &nMax, long &nValue)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_BOOL bIsAuto;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFlip] Called"<< std::endl;
    m_sLogFile.flush();
#endif
    ret = getControlValues(SVB_FLIP, nMin, nMax, nValue, bIsAuto);
    if(ret != SVB_SUCCESS) {
        return VAL_NOT_AVAILABLE;
    }
    return nErr;
}

int CSVBony::setFlip(long nFlip)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    
    m_nFlip = nFlip;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setFlip] Flip set to " << m_nFlip << std::endl;
    m_sLogFile.flush();
#endif
    ret = setControlValue(SVB_FLIP, m_nFlip);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

int CSVBony::getSpeedMode(long &nMin, long &nMax, long &nValue)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_BOOL bIsAuto = SVB_FALSE;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSpeedMode] Called"<< std::endl;
    m_sLogFile.flush();
#endif
    ret = getControlValues(SVB_FRAME_SPEED_MODE, nMin, nMax, nValue, bIsAuto);
    if(ret != SVB_SUCCESS) {
        return VAL_NOT_AVAILABLE;
    }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSpeedMode] speed mode is " << nValue << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSpeedMode] speed mode nMin is " << nMin << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSpeedMode] speed mode nMax is " << nMax << std::endl;
    m_sLogFile.flush();
#endif
    return nErr;
}

int CSVBony::setSpeedMode(long nSpeed)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    
    m_nSpeedMode = nSpeed;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setSpeedMode] Called"<< std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setSpeedMode] SpeedMode set to " << nSpeed << std::endl;
    m_sLogFile.flush();
#endif
    ret = setControlValue(SVB_FRAME_SPEED_MODE, m_nSpeedMode);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

int CSVBony::getContrast(long &nMin, long &nMax, long &nValue)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_BOOL bIsAuto = SVB_FALSE;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getContrast] Called"<< std::endl;
    m_sLogFile.flush();
#endif
    ret = getControlValues(SVB_CONTRAST, nMin, nMax, nValue, bIsAuto);
    if(ret != SVB_SUCCESS) {
        return VAL_NOT_AVAILABLE;
    }
    return nErr;
}

int CSVBony::setContrast(long nContrast)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    
    m_nContrast = nContrast;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setContrast] Called"<< std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setContrast] Contrast set to " << m_nContrast << std::endl;
    m_sLogFile.flush();
#endif
    ret = setControlValue(SVB_CONTRAST, m_nContrast);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

int CSVBony::getSharpness(long &nMin, long &nMax, long &nValue)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_BOOL bIsAuto = SVB_FALSE;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSharpness] Called"<< std::endl;
    m_sLogFile.flush();
#endif
    ret = getControlValues(SVB_SHARPNESS, nMin, nMax, nValue, bIsAuto);
    if(ret != SVB_SUCCESS) {
        return VAL_NOT_AVAILABLE;
    }
    return nErr;
}

int CSVBony::setSharpness(long nSharpness)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;

    m_nSharpness = nSharpness;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setSharpness] Called"<< std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setSharpness] Sharpness set to " << m_nSharpness << std::endl;
    m_sLogFile.flush();
#endif
    ret = setControlValue(SVB_SHARPNESS, m_nSharpness);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

int CSVBony::getSaturation(long &nMin, long &nMax, long &nValue)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_BOOL bIsAuto = SVB_FALSE;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getSaturation] Called"<< std::endl;
    m_sLogFile.flush();
#endif
    ret = getControlValues(SVB_SATURATION, nMin, nMax, nValue, bIsAuto);
    if(ret != SVB_SUCCESS) {
        return VAL_NOT_AVAILABLE;
    }
    return nErr;
}

int CSVBony::setSaturation(long nSaturation)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;

    m_nSaturation = nSaturation;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setSaturation] Called"<< std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setSaturation] Saturation set to " << m_nSaturation << std::endl;
    m_sLogFile.flush();
#endif
    ret = setControlValue(SVB_SATURATION, m_nSaturation);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}

int CSVBony::getBlackLevel(long &nMin, long &nMax, long &nValue)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_BOOL bIsAuto = SVB_FALSE;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getBlackLevel] Called"<< std::endl;
    m_sLogFile.flush();
#endif
    ret = getControlValues(SVB_BLACK_LEVEL, nMin, nMax, nValue, bIsAuto);
    if(ret != SVB_SUCCESS) {
        return VAL_NOT_AVAILABLE;
    }
    return nErr;
}

int CSVBony::setBlackLevel(long nBlackLevel)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    
    m_nBlackLevel = nBlackLevel;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setBlackLevel] Called"<< std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setBlackLevel] Black level set to " << m_nBlackLevel << std::endl;
    m_sLogFile.flush();
#endif
    ret = setControlValue(SVB_BLACK_LEVEL, m_nBlackLevel);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;
 
    return nErr;
}


int CSVBony::getBadPixelCorrection(bool &bEnabled)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    SVB_BOOL bIsAuto = SVB_FALSE;
    long nMin, nMax, nValue;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getBadPixelCorrection] Called"<< std::endl;
    m_sLogFile.flush();
#endif

    // older SDK
    //bEnabled = false;
    //return nErr;
    // newer SDK
    ret = getControlValues(SVB_BAD_PIXEL_CORRECTION_ENABLE, nMin, nMax, nValue, bIsAuto);
    if(ret != SVB_SUCCESS) {
        return VAL_NOT_AVAILABLE;
    }
    m_bBadPixelCorrectionEnabled = (nValue==1?true:false);
    bEnabled = m_bBadPixelCorrectionEnabled;
    return nErr;
}

int CSVBony::setBadPixelCorrection(bool bEnabled)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;

    m_bBadPixelCorrectionEnabled = bEnabled;
    // old SDK
    // return nErr;

    // new SDK
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setBadPixelCorrection] Called"<< std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setBadPixelCorrection] Bad pixel correction " << (bEnabled == SVB_TRUE? "Enabled":"Disabled") << std::endl;
    m_sLogFile.flush();
#endif
    ret = setControlValue(SVB_BAD_PIXEL_CORRECTION_ENABLE, m_bBadPixelCorrectionEnabled?1:0);
    if(ret != SVB_SUCCESS)
        nErr = ERR_CMDFAILED;

    return nErr;
}


SVB_ERROR_CODE CSVBony::setControlValue(SVB_CONTROL_TYPE nControlType, long nValue, SVB_BOOL bAuto)
{
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    
    if(!m_bConnected)
        return SVB_SUCCESS;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setControlValue] nControlType = " << nControlType << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setControlValue] nValue       = " << nValue << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setControlValue] bAuto        = " << (bAuto == SVB_TRUE? "Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setControlValue] ************************" << std::endl<< std::endl;
    m_sLogFile.flush();
#endif


    ret = SVBSetControlValue(m_nCameraID, nControlType, nValue, bAuto);
    if(ret != SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setControlValue] Error setting value !!!! : " << ret << std::endl;
        m_sLogFile.flush();
#endif
    }
    return ret;
}


SVB_ERROR_CODE CSVBony::getControlValues(SVB_CONTROL_TYPE nControlType, long &nMin, long &nMax, long &nValue, SVB_BOOL &bIsAuto)
{
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    int nControlID;
    bool bControlAvailable = false;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getControlValues] Called"<< std::endl;
    m_sLogFile.flush();
#endif
    // look for the control
    for(nControlID = 0; nControlID< m_nControlNums; nControlID++) {
        if(m_ControlList.at(nControlID).ControlType == nControlType) {
            bControlAvailable = true;
            break;
        }
    }

    nValue = VAL_NOT_AVAILABLE;
    nMin = VAL_NOT_AVAILABLE;
    nMax = VAL_NOT_AVAILABLE;

    if(!bControlAvailable)
        return SVB_ERROR_INVALID_CONTROL_TYPE;

    ret = SVBGetControlValue(m_nCameraID, nControlType, &nValue, &bIsAuto);
    if(ret != SVB_SUCCESS){
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getControlValues] Error, ret = " << ret << std::endl;
        m_sLogFile.flush();
#endif
        return ret;
    }
    // look for min,max in the control caps
    nMin = m_ControlList.at(nControlID).MinValue;
    nMax = m_ControlList.at(nControlID).MaxValue;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getControlValues] nControlType = " << nControlType << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getControlValues] nMin         = " << nMin << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getControlValues] nMax         = " << nMax << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getControlValues] nValue       = " << nValue << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getControlValues] bIsAuto      = " << (bIsAuto == SVB_TRUE? "Yes":"No") << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getControlValues] ************************" << std::endl<< std::endl;
    m_sLogFile.flush();
#endif
    return ret;
}

#pragma mark - Camera frame

int CSVBony::setROI(int nLeft, int nTop, int nWidth, int nHeight)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
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
    if( (m_nReqROIWidth % 8 != 0) || (nLeft!=nNewLeft)) {// Adjust width to upper 4 boundary or if the left border changed we need to adjust the width
        nNewWidth = (( (m_nReqROIWidth + (nNewLeft%4)) /8) + 1) * 8;
        if ((nNewLeft + nNewWidth) > int(m_nMaxWidth/m_nCurrentBin)) {
            nNewLeft -=8;
            if(nNewLeft<0) {
                nNewLeft = 0;
                nNewWidth = nNewWidth - 8;
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
        if((nNewTop + nNewHeight) > int(m_nMaxHeight/m_nCurrentBin)) {
            nNewTop -=2;
            if(nNewTop <0) {
                nNewTop = 0;
                nNewHeight = nNewHeight - 2;
            }
        }
    }
    else
        nNewHeight = m_nReqROIHeight;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setROI] nLeft      : " << nLeft << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setROI] nTop       : " << nTop << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setROI] nWidth     : " << nWidth << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setROI] nHeight    :" << nHeight << std::endl;

    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setROI] nNewLeft   : " << nNewLeft << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setROI] nNewTop    : " << nNewTop << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setROI] nNewWidth  : " << nNewWidth << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setROI] nNewHeight : " << nNewHeight << std::endl;
    m_sLogFile.flush();
#endif

    if( m_nROILeft == nNewLeft && m_nROITop == nNewTop && m_nROIWidth == nNewWidth && m_nROIHeight == nNewHeight) {
        return nErr; // no change since last ROI change request
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setROI] Requested x, y, w, h : " << nLeft << ", " << nTop << ", " << nWidth << ", " << nHeight << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setROI] Set to    x, y, w, h : " << nNewLeft << ", " << nNewTop << ", " << nNewWidth << ", " << nNewHeight << std::endl;
    m_sLogFile.flush();
#endif

    ret = SVBSetROIFormat(m_nCameraID, nNewLeft, nNewTop, nNewWidth, nNewHeight, m_nCurrentBin);
    if(ret!=SVB_SUCCESS) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setROI] Setting new ROI failled, ret = " << ret << std::endl;
        m_sLogFile.flush();
#endif
        return ERR_CMDFAILED;
    }
    m_nROILeft = nNewLeft;
    m_nROITop = nNewTop;
    m_nROIWidth = nNewWidth;
    m_nROIHeight = nNewHeight;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [setROI] New ROI set" << std::endl;
    m_sLogFile.flush();
#endif
    return nErr;
}

int CSVBony::clearROI()
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    
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
    SVB_ERROR_CODE ret = SVB_SUCCESS;
    int sizeReadFromCam;
    unsigned char* imgBuffer = nullptr;
    int i = 0;
    uint16_t *buf;
    int srcMemWidth;
    int copyWidth = 0;
    int copyHeight = 0;

    int timeout = 0;

    if(!frameBuffer) {
        stopCaputure();
        return ERR_POINTER;
    }

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] nHeight         : " << nHeight << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] nMemWidth       : " << nMemWidth << std::endl;

    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] m_nROILeft      : " << m_nROILeft << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] m_nReqROILeft   :" << m_nReqROILeft << std::endl;

    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] m_nROITop       : " << m_nROITop << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] m_nReqROITop    : " << m_nReqROITop << std::endl;

    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] m_nROIWidth     : " << m_nROIWidth << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] m_nReqROIWidth  : " << m_nReqROIWidth << std::endl;

    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] m_nROIHeight    : " << m_nROIHeight << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] m_nReqROIHeight : " << m_nReqROIHeight << std::endl;

    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] getBitDepth()/8 : " << getBitDepth()/8 << std::endl;
    m_sLogFile.flush();
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
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] srcMemWidth     : " << srcMemWidth << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] nMemWidth       : " << nMemWidth << std::endl;
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] sizeReadFromCam : " << sizeReadFromCam << std::endl;
    m_sLogFile.flush();
#endif

    while(true) {
        ret = SVBGetVideoData(m_nCameraID, imgBuffer, sizeReadFromCam, 100);
        if(ret!=SVB_SUCCESS) {
            timeout++;
            if(timeout > MAX_DATA_TIMEOUT) {
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
                m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] SVBGetVideoData error : " << ret << std::endl;
                m_sLogFile.flush();
#endif
                stopCaputure();
                if(imgBuffer != frameBuffer)
                    free(imgBuffer);
                return ERR_RXTIMEOUT;
            }
            continue;
        }
        else
            break; // we have an image
    }

    // shift data
    if(m_nNbBitToShift) {
        buf = (uint16_t *)imgBuffer;
        for(int i=0; i<sizeReadFromCam/2; i++)
            buf[i] = buf[i]<<m_nNbBitToShift;
    }

    if(imgBuffer != frameBuffer) {
        copyWidth = srcMemWidth>nMemWidth?nMemWidth:srcMemWidth;
        copyHeight = m_nROIHeight>nHeight?nHeight:m_nROIHeight;
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] copying (" << m_nROILeft << ", " << m_nROITop << ", " << m_nROIWidth << ", " << m_nROIHeight << ") => (" <<  m_nReqROILeft << ", " << m_nReqROITop << ", " << m_nReqROIWidth << ", " << m_nReqROIHeight << ")" << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] srcMemWidth     : " << srcMemWidth << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] nMemWidth       : " << nMemWidth << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] copyHeight      : " << copyHeight << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] copyWidth       : " << copyWidth << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] sizeReadFromCam : " << sizeReadFromCam << std::endl;
        m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getFrame] size to TSX     : " <<  nHeight * nMemWidth << std::endl;
        m_sLogFile.flush();
#endif
        // copy every line from source buffer newly aligned into TSX buffer cutting at copyWidth
        for(i=0; i<copyHeight; i++) {
            memcpy(frameBuffer+(i*nMemWidth), imgBuffer+(i*srcMemWidth), copyWidth);
        }
        free(imgBuffer);
    }
    stopCaputure();
    return nErr;
}


#pragma mark - Camera relay


int CSVBony::RelayActivate(const int nXPlus, const int nXMinus, const int nYPlus, const int nYMinus, const bool bSynchronous, const bool bAbort)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret = SVB_SUCCESS;
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

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [buildGainList] Rebuilding gain list"<< std::endl;
    m_sLogFile.flush();
#endif

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

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [buildGainList] m_nNbGainValue : " << m_nNbGainValue << std::endl;
    m_sLogFile.flush();
#endif
}

int CSVBony::getNbGainInList()
{
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [getNbGainInList] m_nNbGainValue :  " << m_nNbGainValue << std::endl;
    m_sLogFile.flush();
#endif

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


#ifdef PLUGIN_DEBUG
void CSVBony::log(std::string sLogEntry)
{
    m_sLogFile << "["<<getTimeStamp()<<"]"<< " [log] " << sLogEntry << std::endl;
    m_sLogFile.flush();

}

const std::string CSVBony::getTimeStamp()
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    std::strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}
#endif
