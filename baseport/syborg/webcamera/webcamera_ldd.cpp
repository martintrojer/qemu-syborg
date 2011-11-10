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

#include <kern_priv.h>
#include "webcamera_ldd.h"
#include <webcamera_driver.h>

#define DP(format...) Kern::Printf(format)

_LIT(KDriver1PanicCategory,"WebcameraDevice");

/**
 *Create Logic device.
 *
 */
DECLARE_STANDARD_LDD()
	{
    DP("DECLARE_STANDARD_LDD()");
    return new DWebcameraLogicalDevice;
	}

/**
  Constructor
*/
DWebcameraLogicalDevice::DWebcameraLogicalDevice()
	{
	DP("DWebcameraLogicalDevice()");
    // Set version number for this device
    iVersion=RWebcameraDevice::VersionRequired();
    // Indicate that we work with a PDD
    iParseMask=KDeviceAllowPhysicalDevice;
	}

/**
  Destructor
*/
DWebcameraLogicalDevice::~DWebcameraLogicalDevice()
	{
	}

/**
  Second stage constructor for DDriver1Factory.
  This must at least set a name for the driver object.

  @return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DWebcameraLogicalDevice::Install()
	{
	DP("DWebcameraLogicalDevice::Install()");

	return SetName(&RWebcameraDevice::Name());
	}

/**
  Return the drivers capabilities.
  Called in the response to an RDevice::GetCaps() request.

  @param aDes User-side descriptor to write capabilities information into
*/
void DWebcameraLogicalDevice::GetCaps(TDes8& aDes) const
	{
    // Create a capabilities object
	RWebcameraDevice::TCaps caps;
    caps.iVersion = iVersion;
    // Write it back to user memory
    Kern::InfoCopy(aDes,(TUint8*)&caps,sizeof(caps));
	}

/**
  Called by the kernel's device driver framework to create a Logical Channel.
  This is called in the context of the user thread (client) which requested the creation of a Logical Channel
  (E.g. through a call to RBusLogicalChannel::DoCreate)
  The thread is in a critical section.

  @param aChannel Set to point to the created Logical Channel

  @return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DWebcameraLogicalDevice::Create(DLogicalChannelBase*& aChannel)
	{
	DP("DWebcameraLogicalDevice::Create() start");
	aChannel = new DWebcameraLogicalChannel;
	if(!aChannel)
		return KErrNoMemory;
	return KErrNone;
	DP("DWebcameraLogicalDevice::Create() end");
	}

/**
  Constructor
*/
DWebcameraLogicalChannel::DWebcameraLogicalChannel()
	: iReceiveDataDfc(GetOneFlameDfc, this, 1)
	  ,iCaptureDfc(CaptureDfc,this,1)
	{
	DP("DWebcameraLogicalChannel::DWebcameraLogicalChannel() start");

    // Get pointer to client threads DThread object
	iClient=&Kern::CurrentThread();
    // Open a reference on client thread so it's control block can't dissapear until
    // this driver has finished with it.
	((DObject*)iClient)->Open();
	
	DP("DWebcameraLogicalChannel::DWebcameraLogicalChannel() end");
	}

/**
  Destructor
*/
DWebcameraLogicalChannel::~DWebcameraLogicalChannel()
	{
	DP("DWebcameraLogicalChannel::~DWebcameraLogicalChannel() start");
    // Cancel all processing that we may be doing
	DoCancel(RWebcameraDevice::EAllRequests);	
	if (iComm)
		{
		delete iComm;
		}
    if (iChunk)
        {
        Epoc::FreePhysicalRam(iPhysAddr, iSize);
        }
    // Close our reference on the client thread
	Kern::SafeClose((DObject*&)iClient,NULL);
	DP("DWebcameraLogicalChannel::~DWebcameraLogicalChannel() end");
	}

/**
  Called when a user thread requests a handle to this channel.
*/
TInt DWebcameraLogicalChannel::RequestUserHandle(DThread* aThread, TOwnerType aType)
    {
    // Make sure that only our client can get a handle
    if (aType!=EOwnerThread || aThread!=iClient)
        return KErrAccessDenied;
    return KErrNone;
    }

/**
  Second stage constructor called by the kernel's device driver framework.
  This is called in the context of the user thread (client) which requested the creation of a Logical Channel
  (E.g. through a call to RBusLogicalChannel::DoCreate)
  The thread is in a critical section.

  @param aUnit The unit argument supplied by the client
  @param aInfo The info argument supplied by the client
  @param aVer The version argument supplied by the client

  @return KErrNone if successful, otherwise one of the other system wide error codes.
*/
TInt DWebcameraLogicalChannel::DoCreate(TInt /*aUnit*/, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	DP("DWebcameraLogicalChannel::DoCreate() start");
    if(!Kern::CurrentThreadHasCapability(ECapability_None,__PLATSEC_DIAGNOSTIC_STRING("Checked by Webcamera")))
    	{
        return KErrPermissionDenied;
    	}
    // Check version
	if (!Kern::QueryVersionSupported(RWebcameraDevice::VersionRequired(),aVer))
		{
		return KErrNotSupported;
		}
    // Setup LDD for receiving client messages
	SetDfcQ(Kern::DfcQue0());
	iMsgQ.Receive();
    // Associate DFCs with the same queue we set above to receive client messages on
	iReceiveDataDfc.SetDfcQ(iDfcQ);
	iCaptureDfc.SetDfcQ(iDfcQ);
    // Give PDD a pointer to this channel
	Pdd()->iLdd=this;

	//allocate Memory
	iSize=Kern::RoundToPageSize(BUFSIZE);
	TInt rtn=Epoc::AllocPhysicalRam(iSize, iPhysAddr);
	if (rtn != KErrNone)
		{
		return rtn;
		}
	rtn=DPlatChunkHw::New(iChunk, iPhysAddr, iSize,EMapAttrUserRw|EMapAttrBufferedC);
	if (rtn != KErrNone)
		{
		if (iPhysAddr)
			{
			Epoc::FreePhysicalRam(iPhysAddr, iSize);
			}
		return rtn;
		}
	iLAdr = reinterpret_cast<TUint8*>(iChunk->LinearAddress());
	
	iComm=HBuf8::New(BUFSIZE);
	if (!iComm)
		{
		return KErrNotSupported;
		}
	iReceiveDataBuffer=iComm;
	iCaptureBuffer=iComm;

	DP("DWebcameraLogicalChannel::DoCreate() end");
	return KErrNone;
	}

/**
  Process a message for this logical channel.
  This function is called in the context of a DFC thread.

  @param aMessage The message to process.
                  The iValue member of this distinguishes the message type:
                  iValue==ECloseMsg, channel close message
                  iValue==KMaxTInt, a 'DoCancel' message
                  iValue>=0, a 'DoControl' message with function number equal to iValue
                  iValue<0, a 'DoRequest' message with function number equal to ~iValue
*/
void DWebcameraLogicalChannel::HandleMsg(TMessageBase* aMsg)
	{
	DP("DWebcameraLogicalChannel::HandleMsg() start");
	TThreadMessage& m=*(TThreadMessage*)aMsg;

    // Get message type
	TInt id=m.iValue;
    DP("id=%d",id);
    
    // Decode the message type and dispatch it to the relevent handler function...
	if (id==(TInt)ECloseMsg)
		{
		DoCancel(RWebcameraDevice::EAllRequests);
		m.Complete(KErrNone, EFalse);
		return;
		}

	if(m.Client()!=iClient)
		{
		Kern::ThreadKill(m.Client(),
						 EExitPanic,
						 ERequestFromWrongThread,
						 KDriver1PanicCategory);
		m.Complete(KErrNone,ETrue);
		return;
		}

	if (id==KMaxTInt)
		{
		DoCancel(m.Int0());
		m.Complete(KErrNone,ETrue);
		return;
		}

	if (id<0)
		{
		// DoRequest
		TRequestStatus* pS=(TRequestStatus*)m.Ptr0();
		TInt rtn =DoRequest(~id,pS,m.Ptr1(),aMsg);

		if (rtn != KErrNone)
			Kern::RequestComplete(iClient,pS,rtn);
        m.Complete(KErrNone,ETrue);
		}
	else
		{
		// DoControl
		TInt rtn = DoControl(id,m.Ptr0(),aMsg);
		m.Complete(rtn,ETrue);
		}
	DP("DWebcameraLogicalChannel::HandleMsg() end");
	}

/**
  Process synchronous 'control' requests
*/
TInt DWebcameraLogicalChannel::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	DP("DWebcameraLogicalChannel::DoControl() start");
	TInt rtn;
	TThreadMessage& m=*(TThreadMessage*)a2;
	TRequestStatus* pS=(TRequestStatus*)m.Ptr0();

	switch (aFunction)
		{
		case RWebcameraDevice::EGetConfig:
//			rtn = GetConfig((TDes8*)a1);
			rtn = KErrNone;
			if ( rtn != KErrNone )
				{
				Kern::RequestComplete(iClient,pS,rtn);
				}
			break;
        case RWebcameraDevice::ESetConfig:
 //       	rtn = SetConfig((const TDesC8*)a1);
            break;
            
		default:
			rtn = KErrNotSupported;
			Kern::RequestComplete(iClient,pS,rtn);
			break;
		}
	DP("DWebcameraLogicalChannel::DoControl() end");
	return rtn;

	}

/**
  Process asynchronous requests.
*/
TInt DWebcameraLogicalChannel::DoRequest(TInt aReqNo,
											TRequestStatus* aStatus,
											TAny* a1,
											TAny* a2)
	{
	DP("DWebcameraLogicalChannel::DoRequest() start");
	TInt rtn;
	TThreadMessage& m=*(TThreadMessage*)a2;

	iRequesting =ETrue;
	rtn = KErrNone;
    DP("aReqNo=%d",aReqNo);
	switch(aReqNo)
		{
		case RWebcameraDevice::EStart:
   			DP("EStart=%d",RWebcameraDevice::EStart);
			iReceiveDataStatus = aStatus;
			iReceiving = ETrue ;
			iReceiveDataBuffer->FillZ(iCaptureBuffer->MaxLength());
			iReceiveDataBuffer->Zero();
			DP("iReceiveDataBuffer Len=%d",iReceiveDataBuffer->MaxLength());
			DP("iReceiveDataBuffer Len=%d",iReceiveDataBuffer->Length());
			rtn = Pdd()->StartViewerFinder(iPhysAddr,iSize);
			if ( rtn != KErrNone ) 
				{
	   			DP("rtn=%d",rtn);
				iReceiving = EFalse ;
				Kern::RequestComplete(iClient,aStatus,rtn);
				}
			else
				{
	   			DP("rtn=%d",rtn);
				// Example Platform Security capability check which tests the
				// client for ECapability_None (which always passes)...
				if ( iRequesting == EFalse )
					{
		   			DP("iRequesting=EFalse");
					iReceiving = EFalse ;
					Kern::RequestComplete(iClient,
										  iReceiveDataStatus,
										  iReceiveDataResult);
					}
				else
					{
					DP("iRequesting=ETrue");
					iReceiveDataDescriptor=(TDes8*)a1;
					}
				}
			break;
		case RWebcameraDevice::ECapture:
			iCaptureing = ETrue ;
			iCaptureStatus = aStatus;
			iCaptureBuffer->FillZ(iCaptureBuffer->MaxLength());
			iCaptureBuffer->Zero();
		    DP("iCaptureBuffer Len=%d",iCaptureBuffer->MaxLength());
		    DP("iCaptureBuffer Len=%d",iCaptureBuffer->Length());
			rtn = Pdd()->StartCapture(iPhysAddr,iSize);
			DP("rtn=%d",rtn);
			if ( rtn != KErrNone ) 
				{
				iCaptureing = EFalse ;
				Kern::RequestComplete(iClient,aStatus,rtn);
				}
			else
				{
				if ( iRequesting == EFalse )
					{
				    DP("iRequesting=EFalse");
					iReceiving = EFalse ;
					Kern::RequestComplete(iClient,iCaptureStatus,iCaptureResult);
					}
				else
					{
			        DP("Capture iRequesting=ETrue");
				    iCaptureDescriptor=(TDes8*)a1;
					}
				}
			break;
		default:
			rtn=KErrNotSupported;
			Kern::RequestComplete(iClient,aStatus,rtn);
			break;
		}
	iRequesting = EFalse;

	DP("DWebcameraLogicalChannel::DoRequest() end");
	return rtn;

	}

/**
  Process cancelling of asynchronous requests.
*/
void DWebcameraLogicalChannel::DoCancel(TUint aMask)
	{
	DP("DWebcameraLogicalChannel::DoCancel() start");
	TInt rtn;
	DP("aMask=%d",aMask);
    if (aMask&(1<<RWebcameraDevice::EStart))
    	{
		DP("RWebcameraDevice::EStart=%d",RWebcameraDevice::EStart);
		if (iReceiveDataStatus)
			{
			DP("iReceiveDataStatus=%d",iReceiveDataStatus);
			Pdd()->Stop(DWebcameraDriverBase::USB_cancel);
			iReceiving = EFalse ;
			iReceiveDataDfc.Cancel();
			Kern::RequestComplete(iClient,iReceiveDataStatus,KErrCancel);
			}
    	}
    if (aMask&(1<<RWebcameraDevice::ECapture))
    	{
		DP("RWebcameraDevice::ECapture=%d",RWebcameraDevice::ECapture);
		if (iCaptureStatus)
			{
			Pdd()->Stop(DWebcameraDriverBase::USB_cancel);
			iReceiving = EFalse ;
			iCaptureDfc.Cancel();
			Kern::RequestComplete(iClient,iCaptureStatus,KErrCancel);
			}
    	}
	DP("DWebcameraLogicalChannel::DoCancel() end");
	}

/**
  Called by PDD from ISR to indicate that a ReceiveData operation has completed.
*/
void DWebcameraLogicalChannel::GetOneFlameComplete(TInt aDataSize)
    {
	DP("DWebcameraLogicalChannel::GetOneFlameComplete() start");
	DP("datasize=%d",aDataSize);
	iSaveSize=iSize - aDataSize;
    // Queue DFC
    iReceiveDataDfc.Add();
    //set size of received data
    if (iSaveSize > 0)
    	{
	    iReceiveDataResult = KErrNone;
    	}
    else
    	{
		iReceiveDataResult = KErrUnknown;//TODO:define of error
    	}
	DP("DWebcameraLogicalChannel::GetOneFlameComplete() end");
    }

/**
  Called by PDD from ISR to indicate that a get capture image operation has completed.
*/
void DWebcameraLogicalChannel::CaptureComplete(TInt aDataSize)
    {
	DP("DWebcameraLogicalChannel::CaptureComplete() start");
	DP("datasize=%d",aDataSize);
	iSaveSize=iSize - aDataSize;
    // Queue DFC
	iCaptureDfc.Add();
    //set size of received data
    if (iSaveSize > 0)
    	{
		iCaptureResult = KErrNone;
    	}
    else
    	{
        iCaptureResult = KErrUnknown;//TODO:define of error
    	}
	DP("DWebcameraLogicalChannel::CaptureComplete() end");
    }

/**
  DFC Callback which gets triggered after the PDD has signalled that getting oneflame completed.
  This just casts aPtr and calls DoGetOneFlameComplete().
*/
void DWebcameraLogicalChannel::GetOneFlameDfc(TAny* aPtr)
    {
	DP("DWebcameraLogicalChannel::GetOneFlameDfc() start");
    ((DWebcameraLogicalChannel*)aPtr)->DoGetOneFlameComplete();
	DP("DWebcameraLogicalChannel::GetOneFlameDfc() end");
    }

/**
  DFC Callback which gets triggered after the PDD has signalled that getting Capture image completed.
  This just casts aPtr and calls DoCaptureComplete().
*/
void DWebcameraLogicalChannel::CaptureDfc(TAny* aPtr)
    {
	DP("DWebcameraLogicalChannel::CaptureDfc() start");
    ((DWebcameraLogicalChannel*)aPtr)->DoCaptureComplete();
	DP("DWebcameraLogicalChannel::CaptureDfc() end");
    }

/**
  Called from a DFC after the PDD has signalled that getting oneflame completed.
*/
void DWebcameraLogicalChannel::DoGetOneFlameComplete()
    {
	DP("DWebcameraLogicalChannel::DoGetOneFlameComplete() start");
	iReceiveDataBuffer->Copy(iLAdr,iSaveSize);
    DP("iReceiveDataBuffer Len=%d",iReceiveDataBuffer->Length());
	// Write data to client from our buffer
    TInt result=Kern::ThreadDesWrite(iClient,iReceiveDataDescriptor,*iReceiveDataBuffer,0);
    // Finished with client descriptor, so NULL it to help detect coding errors
    iReceiveDataDescriptor = NULL;

    // Use result code from PDD if it was an error
    if(iReceiveDataResult!=KErrNone)
        result = iReceiveDataResult;
    
    // Complete clients request
    Kern::RequestComplete(iClient,iReceiveDataStatus,result);
	DP("DWebcameraLogicalChannel::DoGetOneFlameComplete() end");
    }

/**
  Called from a DFC after the PDD has signalled that getting Capture image completed.
*/
void DWebcameraLogicalChannel::DoCaptureComplete()
    {
	DP("DWebcameraLogicalChannel::DoCaptureComplete() start");
	iCaptureBuffer->Copy(iLAdr,iSaveSize);
    DP("iCaptureBuffer Len=%d",iCaptureBuffer->Length());
	// Write data to client from our buffer
   TInt result=Kern::ThreadDesWrite(iClient,iCaptureDescriptor,*iCaptureBuffer,0);  	
    // Finished with client descriptor, so NULL it to help detect coding errors
    iCaptureDescriptor = NULL;

    // Use result code from PDD if it was an error
    if(iCaptureResult!=KErrNone)
        result = iCaptureResult;

    // Complete clients request
    Kern::RequestComplete(iClient,iCaptureStatus,result);
	DP("DWebcameraLogicalChannel::DoCaptureComplete() end");
    }

/**
  Process a GetConfig control message. This writes the current driver configuration to a
  RWebcameraDevice::TConfigBuf supplied by the client.
*/
TInt DWebcameraLogicalChannel::GetConfig(TDes8* aConfigBuf)
    {
	//unsupported
    }

/**
  Process a SetConfig control message. This sets the driver configuration using a
  RWebcameraDevice::TConfigBuf supplied by the client.
*/
TInt DWebcameraLogicalChannel::SetConfig(const TDesC8* aConfigBuf)
    {
	//unsupported
    }

/**
  Fill a TConfig with the drivers current configuration.
*/
/*void DWebcameraLogicalChannel::CurrentConfig(RWebcameraDevice::TConfig& aConfig)
    {
	//unsupported
    }
*/

/**
 *Get the point to Physical channel.
 */
DWebcameraDriverBase* DWebcameraLogicalChannel::Pdd()
	{
	DP("DWebcameraLogicalChannel::Pdd() start");
	return (DWebcameraDriverBase*)iPdd;
	}

