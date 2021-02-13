//
//  SVBony.cpp
//  IIDC
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
    m_nDefaultGain = 100;
    m_pSleeper = nullptr;
    
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
    
    if(nCameraID)
        m_nCameraID = nCameraID;
    else
        return ERR_DEVICENOTSUPPORTED;

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


    ret = SVBSetControlValue(m_nCameraID, SVB_EXPOSURE , (double)(1 * 1000000), SVB_FALSE);
    // set default valurs
    /*
    ret = SVBSetControlValue(m_nCameraID, SVB_GAIN , m_nDefaultGain, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_CONTRAST , 50, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_SHARPNESS , 0, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_SATURATION , 150, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_WB_R , 100, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_WB_G , 100, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_WB_B , 100, SVB_FALSE);
    ret = SVBSetControlValue(m_nCameraID, SVB_GAMMA , 100, SVB_FALSE);
*/
     ret = SVBSetControlValue(m_nCameraID, SVB_FRAME_SPEED_MODE , 0, SVB_FALSE); // low speed
    
    ret = SVBSetOutputImageType(m_nCameraID, m_nVideoMode);
    ret = SVBSetCameraMode(m_nCameraID, SVB_MODE_TRIG_SOFT);
    
    ret = SVBStartVideoCapture(m_nCameraID);
    if(ret!=SVB_SUCCESS)
        nErr =ERR_CMDFAILED;
    m_bCapturerunning = true;


    /*
    // set video mode by chosng the biggest one for now.
    nFrameSize = 0;
    nErr = getResolutions(m_vResList);
    if(nErr)
        return ERR_COMMANDNOTSUPPORTED;

    for (i=0; i< m_vResList.size(); i++) {
        if(nFrameSize < (m_vResList[i].nWidth * m_vResList[i].nHeight) && !bIsVideoFormat7(m_vResList[i].vidMode) ) { // let's avoid format 7 for now.
            nFrameSize = m_vResList[i].nWidth * m_vResList[i].nHeight;
            m_tCurrentResolution = m_vResList[i];
            printf("Selecting nWidth %d\n", m_tCurrentResolution.nWidth);
            printf("Selecting nHeight %d\n", m_tCurrentResolution.nHeight);
            printf("Selecting vidMode %d\n", m_tCurrentResolution.vidMode);
            printf("Selecting bMode7 %d\n", m_tCurrentResolution.bMode7);
            printf("Selecting nPacketSize %d\n", m_tCurrentResolution.nPacketSize);
            printf("Selecting bModeIs16bits %d\n", m_tCurrentResolution.bModeIs16bits);
            printf("Selecting nBitsPerPixel %d\n", m_tCurrentResolution.nBitsPerPixel);
            printf("Selecting bNeed8bitTo16BitExpand %d\n", m_tCurrentResolution.bNeed8bitTo16BitExpand);
        }
 printf("bit depth = %d\n", m_tCurrentResolution.nBitsPerPixel);
 */

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
    
    m_bAbort = false;
    
    ret = SVBSetCameraMode(m_nCameraID, SVB_MODE_TRIG_SOFT);
    if(ret!=SVB_SUCCESS)
        nErr =ERR_CMDFAILED;

    if(!m_bCapturerunning) {
        ret = SVBStartVideoCapture(m_nCameraID);
        if(ret!=SVB_SUCCESS)
            nErr =ERR_CMDFAILED;
        m_bCapturerunning = true;
    }
    ret = SVBSetOutputImageType(m_nCameraID, m_nVideoMode);
    if(ret!=SVB_SUCCESS)
        nErr =ERR_CMDFAILED;

    // set exposure time (s -> us)
    ret = SVBSetControlValue(m_nCameraID, SVB_EXPOSURE , (double)(dTime * 1000000), SVB_FALSE);
    if(ret!=SVB_SUCCESS)
        nErr =ERR_CMDFAILED;

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
                sBayerPattern.assign("GBGR");
                break;
        }
    }
}

int CSVBony::setROI(int nLeft, int nTop, int nWidth, int nHeight)
{
    int nErr = PLUGIN_OK;
    SVB_ERROR_CODE ret;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::setROI] x, y, w, h : %d, %d, %d, %d\n", timestamp, nLeft, nTop, nWidth, nHeight);
        fflush(Logfile);
#endif

    ret = SVBSetROIFormat(m_nCameraID, nLeft, nTop, nWidth, nHeight, m_nCurrentBin);
    if(ret!=SVB_SUCCESS)
        nErr =ERR_CMDFAILED;
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
    int sizeToCopy;

    uint16_t *buf;
    
    if(!frameBuffer)
        return ERR_POINTER;

    sizeToCopy = nHeight * nMemWidth;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::getFrame] nHeight, nMemWidth, sizeToCopy           : %d, %d, %d\n", timestamp, nHeight, nMemWidth, sizeToCopy);
        fflush(Logfile);
#endif
    ret = SVBGetVideoData(m_nCameraID, frameBuffer, sizeToCopy, 100);
    if(ret!=SVB_SUCCESS) {
        // wait and retry
        m_pSleeper->sleep(100);
        ret = SVBGetVideoData(m_nCameraID, frameBuffer, sizeToCopy, 100);
        if(ret!=SVB_SUCCESS) {
            return ERR_CMDFAILED;
        }
    }
#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::getFrame] buffer[0] content                        : %02x%02x\n", timestamp, *(frameBuffer+1), *frameBuffer);
        fprintf(Logfile, "[%s] [CSVBony::getFrame] end of buffer content                    : %02x%02x\n", timestamp, *(frameBuffer+sizeToCopy), *(frameBuffer+sizeToCopy-1));
        fflush(Logfile);
#endif

    
    // shift data
    buf = (uint16_t *)frameBuffer;
    for(int i=0; i<sizeToCopy/2; i++)
        buf[i] = buf[i]<<m_nNbBitToShift;

#if defined PLUGIN_DEBUG && PLUGIN_DEBUG >= 2
        ltime = time(NULL);
        timestamp = asctime(localtime(&ltime));
        timestamp[strlen(timestamp) - 1] = 0;
        fprintf(Logfile, "[%s] [CSVBony::getFrame] after bit shift buffer[0] content        : %02x%02x\n", timestamp, *(frameBuffer+1), *frameBuffer);
        fprintf(Logfile, "[%s] [CSVBony::getFrame] after bit shift end of buffer content    : %02x%02x\n", timestamp, *(frameBuffer+sizeToCopy), *(frameBuffer+sizeToCopy-1));
        fflush(Logfile);
#endif

    return nErr;
}

int CSVBony::getResolutions(std::vector<camResolution_t> &vResList)
{
    int nErr = PLUGIN_OK;
    int i = 0;
    uint32_t nWidth;
    uint32_t nHeight;
    camResolution_t tTmpRes;


    return nErr;
}

/*
int CSVBony::setFeature(dc1394feature_t tFeature, uint32_t nValue, dc1394feature_mode_t tMode)
{
    int nErr;


    return nErr;
}

int CSVBony::getFeature(dc1394feature_t tFeature, uint32_t &nValue, uint32_t nMin, uint32_t nMax,  dc1394feature_mode_t &tMode)
{
    int nErr;

    return nErr;
}
*/

#pragma mark protected methods


void CSVBony::setCameraFeatures()
{
    int nErr;
    uint32_t nTemp;

}
