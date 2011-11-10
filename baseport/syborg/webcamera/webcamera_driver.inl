/*
* Copyright (c) 2010 ISB.
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* ISB - initial contribution.
*
* Contributors:
*
* Description:
*
*/

#ifndef __deviceIFI_H
#define __deviceIFI_H

/**
  Returns the driver's name
*/
inline const TDesC& RWebcameraDevice::Name()
    {
    _LIT(KDriverName,"WebcameraDevice");
    return KDriverName;
    }

/**
  Returns the version number of the driver
*/
inline TVersion RWebcameraDevice::VersionRequired()
    {
    const TInt KMajorVersionNumber=1;
    const TInt KMinorVersionNumber=1;
    const TInt KBuildVersionNumber=0;
    return TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);
    }

/*
  NOTE: The following member functions would normally be exported from a seperate client DLL
  but are included inline in this header file for convenience.
*/

#ifndef __KERNEL_MODE__

/**
  Opens a logical channel to the driver

  @return One of the system wide error codes.
*/
inline TInt RWebcameraDevice::Open()
    {
    return DoCreate(Name(),VersionRequired(),KNullUnit,NULL,NULL,EOwnerThread);
    }


/**
  Gets the current configuration settings.

  @param aConfig A structure which will be filled with the configuration settings.

  @return KErrNone
*/
inline TInt RWebcameraDevice::GetConfig(TConfigBuf& aConfig)
    {
    return DoControl(EGetConfig,(TAny*)&aConfig);
    }


/**
  Sets the current configuration settings.

  @param aConfig The new configuration settings to be used.

  @return KErrInUse if there are outstanding data transfer requests.
          KErrArgument if any configuration values are invalid.
          KErrNone otherwise
*/
inline TInt RWebcameraDevice::SetConfig(const TConfigBuf& aConfig)
    {
    return DoControl(ESetConfig,(TAny*)&aConfig);
    }

/**
  Receives image from the device.
  Only one receive request may be pending at any time.

  @param aStatus The request to be signalled when the data has been received.
                 The result value will be set to KErrNone on success;
                 or set to one of the system wide error codes when an error occurs.
  @param aData   A descriptor to which the received data will be written.
*/
inline void RWebcameraDevice::StartViewFinder(TRequestStatus& aStatus,TDes8& aBuffer)
    {
	TInt length=BUFSIZE;
    DoRequest(EStart,aStatus,(TAny*)&aBuffer,(TAny*)&length);
    }


/**
  Cancels a previous StartViewFinder request.
*/
inline void RWebcameraDevice::StartViewFinderCancel()
    {
    DoCancel(1<<EStart);
    }

inline void RWebcameraDevice::StopViewFinder()
	{
    DoCancel(1<<EStart);
	}

/**
  Capture data from the device.
  Only one Capture request may be pending at any time.

  @param aStatus The request to be signalled when the data has been captureed.
                 The result value will be set to KErrNone on success;
                 or set to one of the system wide error codes when an error occurs.
  @param aData   A descriptor to which the captured data will be written.
*/
inline void RWebcameraDevice::Capture(TRequestStatus& aStatus,TDes8& aBuffer)
    {
	TInt length=BUFSIZE;
    DoRequest(ECapture,aStatus,(TAny*)&aBuffer,(TAny*)&length);
    }

/**
  Cancels a previous capture request.
*/
inline void RWebcameraDevice::CaptureCancel()
    {
	DoCancel(1<<ECapture);
    }
#endif   // !__KERNEL_MODE__

#endif
