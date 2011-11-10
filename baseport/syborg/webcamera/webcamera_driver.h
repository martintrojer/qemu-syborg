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
* Description: USB driver for test
*
*/
#ifndef __deviceIF_H__
#define __deviceIF_H__

#include <e32cmn.h>
#include <e32ver.h>

#define BUFSIZE  (100*1024)

/**
User interface for 'WebcameraDevice'
Define of the RBusLogicalChannel that is used in the application
*/
class RWebcameraDevice : public RBusLogicalChannel
    {
public:
    /**
    Structure for holding driver capabilities information
    */
    class TCaps
        {
    public:
        TVersion iVersion;
        };
    /**
    Structure for holding driver configuration data
    */
    class TConfig
        {
    public:
        TInt iPddBufferSize;        /**< Size of the PDD's data buffer (not modifiable) */
       //RArray<TSize> iImageSizes /**< size the PDD support*/ //TODO:implement
        };
    typedef TPckgBuf<TConfig> TConfigBuf;

public:
    /**
      Opens a logical channel to the driver

      @return One of the system wide error codes.
    */
    inline TInt Open();
    /**
      Gets the current configuration settings.

      @param aConfig A structure which will be filled with the configuration settings.

      @return KErrNone
    */
    inline TInt GetConfig(TConfigBuf& aConfig);
    /**
      Sets the current configuration settings.

      @param aConfig The new configuration settings to be used.

      @return KErrInUse if there are outstanding data transfer requests.
              KErrArgument if any configuration values are invalid.
              KErrNone otherwise
    */
    inline TInt SetConfig(const TConfigBuf& aConfig);
    /**
      Request data from device.
      Only one send request may be pending at any time.

      @param aStatus The request to be signalled when the data has been sent.
                     The result value will be set to KErrNone on success;
                     or set to one of the system wide error codes when an error occurs.
      @param aData   A descriptor containing the data to send.
    */
    inline void StartViewFinder(TRequestStatus& aStatus,TDes8& aBuffer);
    /**
      Cancels a previous getdata request.
    */
    inline void StartViewFinderCancel();
    /**
      Cancels a previous getdata request and notice device not to send data       
    */
    inline void StopViewFinder();
    /**
      Request data(Capture data) from device.
      Only one send request may be pending at any time.

      @param aStatus The request to be signalled when the data has been sent.
                     The result value will be set to KErrNone on success;
                     or set to one of the system wide error codes when an error occurs.
      @param aData   A descriptor containing the data to send.
    */
    inline void Capture(TRequestStatus& aStatus,TDes8& aBuffer);
    /**
      Cancels a previous getCapturedata request.     
    */
	inline void CaptureCancel();
	/**
	  Returns the driver's name
	*/
    inline static const TDesC& Name();
    /**
      Returns the version number of the driver
    */
    inline static TVersion VersionRequired();
    
public:
    /**
    Enumeration of Control messages.
    */
    enum TControl
        {
        EGetConfig,
        ESetConfig
        };
    /**
    Enumeration of Request messages.
    */
    enum TRequest
        {
        EStart,
        ECapture,
        ENumRequests,
        EAllRequests = (1<<ENumRequests)-1
        };

    // Kernel side LDD channel is a friend
    friend class DDriver1Channel;
    };

// Inline functions
#include <webcamera_driver.inl>

#endif
