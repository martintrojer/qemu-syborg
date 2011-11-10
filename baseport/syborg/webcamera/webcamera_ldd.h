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
#ifndef __deviceLDD_H__
#define __deviceLDD_H__

#include <kernel.h>
#include "webcamera_device.h"

#define BUFSIZE  (100*1024)
/**
 *Logical channel class
 *
 */
class DWebcameraLogicalDevice : public DLogicalDevice
	{
public:
	DWebcameraLogicalDevice();
	~DWebcameraLogicalDevice();
    /**
      Second stage constructor for DDriver1Factory.
      This must at least set a name for the driver object.

      @return KErrNone if successful, otherwise one of the other system wide error codes.
    */
    virtual TInt Install();
    /**
      Return the drivers capabilities.
      Called in the response to an RDevice::GetCaps() request.

      @param aDes User-side descriptor to write capabilities information into
    */
    virtual void GetCaps(TDes8& aDes) const;
    /**
      Called by the kernel's device driver framework to create a Logical Channel.
      This is called in the context of the user thread (client) which requested the creation of a Logical Channel
      (E.g. through a call to RBusLogicalChannel::DoCreate)
      The thread is in a critical section.

      @param aChannel Set to point to the created Logical Channel

      @return KErrNone if successful, otherwise one of the other system wide error codes.
    */
    virtual TInt Create(DLogicalChannelBase*& aChannel);
	};

/**
 *
 * 論理チャネルベースクラス
 *
 * 本クラスは、論理チャネル機能を提供する。
 *
 * @
 * @
 *
 */
class DWebcameraLogicalChannel : public DWebcameraLogicalChannelBase
	{
public:
	/**
	  Constructor
	*/
	DWebcameraLogicalChannel();
	/**
	  Destructor
	*/
	~DWebcameraLogicalChannel();
	/**
	  Called when a user thread requests a handle to this channel.
	*/
    virtual TInt RequestUserHandle(DThread* aThread, TOwnerType aType);
    /**
      Second stage constructor called by the kernel's device driver framework.
      This is called in the context of the user thread (client) which requested the creation of a Logical Channel
      The thread is in a critical section.

      @param aUnit The unit argument supplied by the client
      @param aInfo The info argument supplied by the client
      @param aVer The version argument supplied by the client

      @return KErrNone if successful, otherwise one of the other system wide error codes.
    */
	virtual TInt DoCreate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer);
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
	virtual void HandleMsg(TMessageBase* aMsg);
	/**
	  Process synchronous 'control' requests
	*/
	virtual TInt DoControl(TInt aFunction, TAny* a1, TAny* a2);
	/**
	  Process asynchronous requests.
	*/
	virtual TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	/**
	  Process cancelling of asynchronous requests.
	*/
	virtual void DoCancel(TUint aMask);
	/**
	  Called by PDD from ISR to indicate that a get oneflame operation has completed.
	*/
    virtual void GetOneFlameComplete(TInt aResult);
    /**
      Called by PDD from ISR to indicate that a get capture image operation has completed.
    */
    virtual void CaptureComplete(TInt aResult);
    /**
      DFC Callback which gets triggered after the PDD has signalled that get oneflame completed.
      This just casts aPtr and calls DoGetOneFlameComplete().
    */
    static  void GetOneFlameDfc(TAny* aPtr);
	/**
	  DFC Callback which gets triggered after the PDD has signalled that getting Capture image completed.
	  This just casts aPtr and calls DoCaptureComplete().
	*/
    static  void CaptureDfc(TAny* aPtr);
    /**
      Called from a DFC after the PDD has signalled that getting oneflame completed.
    */
	virtual void DoGetOneFlameComplete();
	/**
	  Called from a DFC after the PDD has signalled that getting Capture image completed.
	*/
	virtual void DoCaptureComplete();
    
	/**
	  Process a GetConfig control message. This writes the current driver configuration to a
	  RWebcameraDevice::TConfigBuf supplied by the client.
	*/
    TInt GetConfig(TDes8* aConfigBuf);
    /**
      Process a SetConfig control message. This sets the driver configuration using a
      RWebcameraDevice::TConfigBuf supplied by the client.
    */
    TInt SetConfig(const TDesC8* aConfigBuf);
//  void CurrentConfig(RWebcameraDevice::TConfig& aConfig);
    /**
     *Get the point to Physical channel.
     */
    DWebcameraDriverBase* Pdd();
    
private:
    /**
     *point to description sent by user-side.
     */
    TDes8* iReceiveDataDescriptor;
    /**
     *buffer for one flame.
     */
    HBuf8* iReceiveDataBuffer;
    /**
     *the status getting one flame.
     */
	TRequestStatus* iReceiveDataStatus;
    /**
     *DFC for getting one flame.
     */
	TDfc iReceiveDataDfc;
    /**
     *the result of the get oneflame operation.
     */
	TInt iReceiveDataResult;
    /**
     */
	TBool iReceiving;
    /**
     *point to description sent by user-side.
     */
    TDes8* iCaptureDescriptor;
    /**
     *buffer for capture image.
     */
	HBuf8* iCaptureBuffer;
    /**
     *the status getting capture image.
     */
	TRequestStatus* iCaptureStatus;
    /**
     *DFC of capture.
     */
	TDfc iCaptureDfc;
    /**
     *the result of the capture operation.
     */
	TInt iCaptureResult;
    /**
     *the status getting capture image.
     */
	TBool iCaptureing;
    /**
     *the status of request.
     */	
	TBool iRequesting;
    /**
     *point to memory used to save one frame or capture image.
     */
    HBuf8* iComm;
    /**
     *Physical adress of contiguous memory.
     */
	TUint32 iPhysAddr;
    /**
     *the size of buffer used to save one frame or capture image.
     */
	TInt iSize;
    /**
     *chunck.
     */
	DPlatChunkHw* iChunk;
    /**
     *Linear adress of chunck.
     */
	TUint8* iLAdr;
    /**
     *the size of received data.
     */
	TInt iSaveSize;
	};

#endif
