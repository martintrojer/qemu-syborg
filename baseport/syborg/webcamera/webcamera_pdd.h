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

#ifndef __devicePDD_H__
#define __devicePDD_H__

#include <comm.h>
#include <e32hal.h>
#include "system.h"
#include "webcamera_device.h"

const TInt KMinimumLddMajorVersion=1;
const TInt KMinimumLddMinorVersion=1;
const TInt KMinimumLddBuild=0;
const TInt EIrqWebamera=0xb;

class DWebcameraPddFactory : public DPhysicalDevice
{
public:
	/**
	  Constructor
	*/
	DWebcameraPddFactory();
	
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
};


class DWebcameraDriver : public DWebcameraDriverBase
{
public:
	/**
	  Constructor
	*/
	DWebcameraDriver();
	/**
	  Destructor
	*/
	~DWebcameraDriver();
    /**
      Second stage constructor called by the kernel's device driver framework.
      This is called in the context of the user thread (client) which requested the creation of a Logical Channel
      The thread is in a critical section.

      @param aUnit The unit argument supplied by the client
      @param aInfo The info argument supplied by the client

      @return KErrNone if successful, otherwise one of the other system wide error codes.
    */
	TInt DoCreate(TInt aUnit, const TDesC8* anInfo);
    /**
      Request data from device.
      Only one send request may be pending at any time.

      @param  aBuffer physical adress
      @param aData   size of buffer.
    */
	virtual TInt StartViewerFinder(TUint aBuffer,TInt aSize);
    /**
      Request data from device.
      Only one send request may be pending at any time.

      @param  aBuffer physical adress
      @param aData   size of buffer.
    */
	virtual TInt StartCapture(TUint aBuffer,TInt aSize);
    /**
      Request device not to send data.
      Only one send request may be pending at any time.

      @param  aMode stop mode
    */
	virtual void Stop(TUSBStopMode aMode);
//	virtual void Caps(TDes8 &aCaps) const;
	/**
	  Called by ISR to indicate that interrupt occurs.
	*/
	static void Isr(TAny* aPtr);
    /**
      Called from a Isr after the Peripheral has signalled that getting oneflame completed.
    */
	void receivedatacallback();

public:
	  /**
	  Enumeration of register types.
	  */
  enum {
		WEBCAMERA_REG_ID           = 0,
		WEBCAMERA_REG_INT_ENABLE   = 1,
		WEBCAMERA_REG_DATA_TYPE    = 2,
		WEBCAMERA_REG_DMA_ADDR     = 3,
		WEBCAMERA_REG_DMA_SIZE     = 4
	    };

private:
    /**
    operation types.
    */
	TInt iType;
};

#endif
