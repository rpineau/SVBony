#!/bin/bash

SDK_VERSION=$1

cp -fv ~/src/Astro/SVBony-SDK-${SDK_VERSION}/SVBCameraSDK_macOS/lib/all/libSVBCameraSDK.a static_libs/macOS
cp -fv ~/src/Astro/SVBony-SDK-${SDK_VERSION}/SVBCameraSDK_Linux/lib/x64/libSVBCameraSDK.a static_libs/x86_64
cp -fv ~/src/Astro/SVBony-SDK-${SDK_VERSION}/SVBCameraSDK_Linux/lib/armv8/libSVBCameraSDK.a static_libs/aarch64/libSVBCameraSDK.a
cp -fv ~/src/Astro/SVBony-SDK-${SDK_VERSION}/SVBCameraSDK_Linux/lib/armv7/libSVBCameraSDK.a static_libs/armv7l/libSVBCameraSDK.a
cp -fv ~/src/Astro/SVBony-SDK-${SDK_VERSION}/SVBCameraSDK_Windows/lib/x86/SVBCameraSDK.* static_libs/Windows/Win32
cp -fv ~/src/Astro/SVBony-SDK-${SDK_VERSION}/SVBCameraSDK_Windows/lib/x64/SVBCameraSDK.* static_libs/Windows/x64
cp -fv ~/src/Astro/SVBony-SDK-${SDK_VERSION}/SVBCameraSDK_macOS/include/SVBCameraSDK.h .


