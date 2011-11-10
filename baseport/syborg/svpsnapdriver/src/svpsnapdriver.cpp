/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/

#include "svpsnapdriver.h"
#include "rsvpsnapdriver.h"
#include "system.h"

static inline TUint32 ReadReg(ESVPSnapReg aReg)
{
  DP("** (SVPSNAPDRIVER) ReadReg(%d)",aReg);

	return *(volatile TUint32 *)(KHwSVPSnapDevice + (aReg << 2));
}

static inline void WriteReg(ESVPSnapReg aReg, TUint32 aVal)
{
  DP("** (SVPSNAPDRIVER) WriteReg(%d,%d)",aReg,aVal);

  *(volatile TUint32*)(KHwSVPSnapDevice + (aReg << 2)) = aVal;
}

//
// DSVPSnapDriverFactory
//

DSVPSnapDriverFactory::DSVPSnapDriverFactory()
{
  DP("** (SVPSNAPDRIVER) DSVPSnapDriverFactory::DSVPSnapDriverFactory()");

  iVersion = TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);    
}

TInt DSVPSnapDriverFactory::Create(DLogicalChannelBase*& aChannel)
{
  DP("** (SVPSNAPDRIVER) DSVPSnapDriverFactory::Create()");
	
#if 0
  if (iOpenChannels != 0)
	return KErrInUse; // a channel is already open
#endif
	
	aChannel = new DSVPSnapChannel(this);
	
	return aChannel ? KErrNone : KErrNoMemory;
}

TInt DSVPSnapDriverFactory::Install()
{
  DP("** (SVPSNAPDRIVER) DSVPSnapDriverFactory::Install()");

  return(SetName(&KSVPSnapDriverName));
}

void DSVPSnapDriverFactory::GetCaps(TDes8& aDes) const
{
  DP("** (SVPSNAPDRIVER) DSVPSnapDriverFactory::GetCaps()");

  TCapsSVPSnapDriver b;
  b.iVersion = TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber);
  aDes.FillZ(aDes.MaxLength());
  aDes.Copy((TUint8 *)&b, Min(aDes.MaxLength(), sizeof(b)));
}

//
// DSVPSnapChannel
//

DSVPSnapChannel::DSVPSnapChannel(DLogicalDevice* aLogicalDevice)
{
  DP("** (SVPSNAPDRIVER) DSVPSnapChannel::DSVPSnapChannel()");

  iDevice = aLogicalDevice;
  
  iClientThread = &Kern::CurrentThread();
  iClientThread->Open();
}

DSVPSnapChannel::~DSVPSnapChannel()
{
  DP("** (SVPSNAPDRIVER) DSVPSnapChannel::~DSVPSnapChannel() ->");
  Kern::SafeClose((DObject*&)iClientThread, NULL);
  DP("** (SVPSNAPDRIVER) DSVPSnapChannel::~DSVPSnapChannel() <-");
}

TInt DSVPSnapChannel::DoCreate(TInt /*aUnit*/, const TDesC* anInfo, const TVersion& aVer)
{
  DP("** (SVPSNAPDRIVER) DSVPSnapChannel::DoCreate()");

  if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber), aVer))
	return KErrNotSupported; 
		       
  //Setup the driver for receiving client messages
  SetDfcQ(Kern::DfcQue0());
  iMsgQ.Receive();  	

  DP("** (SVPSNAPDRIVER) DSVPSnapChannel::DoCreate()- checking device");
  TUint id = ReadReg(ESnapshot_Id);
  DP("** (SVPSNAPDRIVER) DSVPSnapChannel::DoCreate()- checked device- 0x%08x", id);
  WriteReg(ESnapshot_Address, 0);
  return KErrNone;
}

void DSVPSnapChannel::DoCancel(TInt aReqNo)
{
  DP("** (SVPSNAPDRIVER) DSVPSnapChannel::DoCancel() %x(%d)", aReqNo, aReqNo);
}

TInt DSVPSnapChannel::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)
{
  DP("** (SVPSNAPDRIVER) DSVPSnapChannel::DoRequest() %x(%d)", aReqNo, aReqNo);

  TInt err = KErrGeneral;         

  switch(aReqNo)
	{
	case RSVPSnapDriver::ESaveVM:
	  	  DP("RSVPSnapDriver::ESaveVM");
	  	  err = SaveVM((const TDesC8*)a1);
	  break;
	case RSVPSnapDriver::ELoadVM:
	  	 DP("RSVPSnapDriver::ELoadVM");
	  	 err= LoadVM((const TDesC8*)a1);
	  break;
	default:
	  	  DP("default");
	  err = KErrGeneral;
	  break;
	}
	
  if (KErrNone != err)
	DP("** (SVPSNAPDRIVER) Error %d from DoRequest", err);
	
  return err;
}

TInt DSVPSnapChannel::DoControl(TInt aFunction, TAny* a1, TAny* a2)
{
  DP("** (SVPSNAPDRIVER) DSVPSnapChannel::DoControl(%d)", aFunction);

  TInt err = KErrGeneral;         
  /* There should be a good reason to use a control rather than a request. */

  if (KErrNone != err) {
	DP("** (SVPSNAPDRIVER) Error %d from control function", err);
  }
	
  return err;
}

void DSVPSnapChannel::HandleMsg(TMessageBase* aMsg)
{
	
  TThreadMessage& m = *(TThreadMessage*)aMsg;
  TInt id = m.iValue;
  
  DP("** (SVPSNAPDRIVER) DSVPSnapChannel::HandleMsg() %x(%d)", id, id);

  if (id == (TInt)ECloseMsg)
	{
	  m.Complete(KErrNone, EFalse); 
	  return;
	}
  if (id == KMaxTInt)
	{
	  // DoCancel
	  DoCancel(m.Int0());
	  m.Complete(KErrNone, ETrue); 
	  return;
	}
  if (id < 0)
	{
	  // DoRequest
	  TRequestStatus* pStatus = (TRequestStatus*)m.Ptr0();
	  TInt r = DoRequest(~id, pStatus, m.Ptr1(), m.Ptr2());
	  //	  if (r != KErrNone)
	  Kern::RequestComplete(iClientThread,pStatus,r);
	  m.Complete(KErrNone, ETrue);
	}
  else
	{
	  // DoControl
	  TInt r = DoControl(id, m.Ptr0(), m.Ptr1());
	  m.Complete(r, ETrue);
	}
}

TInt DSVPSnapChannel::SaveVM(const TDesC8* aData)
{
  DP("** DSVPSnapChannel::SaveVM()");
   
  TInt err = KErrNone; 
  RET_IF_ERROR(err, Kern::ThreadDesRead(iClientThread, aData, iSendDataBuffer,0));
  TUint32 * ptr = (TUint32*) iSendDataBuffer.Ptr();
  
  WriteReg(ESnapshot_Address, Epoc::LinearToPhysical((TUint32)ptr));
  WriteReg(ESnapshot_Length, (TUint32) (iSendDataBuffer.Length()));
  WriteReg(ESnapshot_Trigger, RSVPSnapDriver::ESaveVM);
  
  return err;
}

TInt DSVPSnapChannel::LoadVM(const TDesC8* aData)
{
  DP("** DSVPSnapChannel::LoadVM()");  

  TInt err = KErrNone; 
  RET_IF_ERROR(err, Kern::ThreadDesRead(iClientThread, aData, iSendDataBuffer,0));
  
  TUint32 * ptr = (TUint32*) iSendDataBuffer.Ptr();  
  WriteReg(ESnapshot_Address, Epoc::LinearToPhysical((TUint32)ptr));
  
  return ReadReg(ESnapshot_Address);
}

DECLARE_EXTENSION_LDD()
{
  // FIXME: Not needed?
  DP("** (SVPSNAPDRIVER) DSVPSnapDriverFactory created");
  return new DSVPSnapDriverFactory;
}

DECLARE_STANDARD_EXTENSION()
{
  DP("** (SVPSNAPDRIVER) SVPSnap extension entry point");
  TInt r;
  DSVPSnapDriverFactory* device = new DSVPSnapDriverFactory;
  if (device==NULL)
	r=KErrNoMemory;
  else
	r=Kern::InstallLogicalDevice(device);
  
  return r;
}
