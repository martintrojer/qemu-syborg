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

#ifndef __deviceBASE_H
#define __deviceBASE_H

#include <comm.h>
#include <e32hal.h>
#include <e32ver.h>


class DWebcameraLogicalChannelBase : public DLogicalChannel
{
public:
	/**
	  Called by PDD from ISR to indicate that a get oneflame operation has completed.
	*/
    virtual void GetOneFlameComplete(TInt aResult)=0;
	/** 
	call to the function if one Capture image is received.
	*/
    virtual void CaptureComplete(TInt aResult)=0;
	/** 
	call to the function if one flame is received.
	*/
	/** 
	call to the function if one Capture image is received.
	*/
	virtual void DoCaptureComplete()=0;
	
public:
  /**
   * pointer to client.
   */	
	DThread* iClient;	
};

class DWebcameraDriverBase : public DBase
{
public:
  /**
  Enumeration of stop modes.
  */
  enum TUSBStopMode
	{
	USB_ViewerFinder =0,
	USB_capture      =1,
	USB_cancel      =2
	};
  /**
  request.
  */
  virtual TInt StartViewerFinder(TUint aBuffer,TInt aSize)=0;
  /**
  Enumeration of stop modes.
  */
  virtual TInt StartCapture(TUint aBuffer,TInt aSize)=0;
  /**
  Enumeration of stop modes.
  */
  virtual void Stop(TUSBStopMode aMode)=0;
//virtual void Caps(TDes8 &aCaps) const;
 
public:
  /**
   * pointer to logic channel.
   */		
  DWebcameraLogicalChannelBase* iLdd;
   /**
   * Linear Addresses of Peripherals.
   */		
  TLinAddr iPortAddr;
  /**
   * interrupt number.
   */		
  TInt iIrq;
  
};

#endif
