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
    m_bDeviceIsUSB = false;
    m_nCameraID = 0;
    m_bAbort = false;
    m_bNeedDepthFix = false;

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
    if (ret != SVB_SUCCESS)
    {
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

    m_bAbort = false;
    m_bFrameAvailable = false;
    
    return nErr;
}

int CSVBony::stopCaputure()
{
    int nErr = PLUGIN_OK;
    return nErr;
}

void CSVBony::abortCapture(void)
{
    m_bAbort = true;
    stopCaputure();
}

int CSVBony::getTemperture(double &dTemp)
{
    // (Value)/10 -273.15 = degree C.
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
    
    return bFrameAvailable;
}

uint32_t CSVBony::getBitDepth()
{
    return 16; //  m_tCurrentResolution.nBitsPerPixel;
}

int CSVBony::getFrame(int nHeight, int nMemWidth, unsigned char* frameBuffer)
{
    int nErr = PLUGIN_OK;
    int sizeToCopy;

    if(!m_bFrameAvailable)
        return ERR_OBJECTNOTFOUND;

    if(!frameBuffer)
        return ERR_POINTER;

    //  copy internal m_pframeBuffer to frameBuffer
    sizeToCopy = nHeight * nMemWidth;
    printf("copying %d byte from internal buffer\n", sizeToCopy);
    memcpy(frameBuffer, m_pframeBuffer, sizeToCopy);

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

int CSVBony::setCameraExposure(double dTime)
{
    int nErr = PLUGIN_OK;
    uint32_t nValue;
    float fMin;
    float fMax;
    bool bAvailable;

    return nErr;
}

