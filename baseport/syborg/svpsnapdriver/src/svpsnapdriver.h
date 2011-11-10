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

#ifndef __SVPSNAPDRIVER_H
#define __SVPSNAPDRIVER_H

#include <kernel.h>    // for DLogicalChannel

#define SVPDBG
#ifdef SVPDBG
#define DP(format...) Kern::Printf(format)
#else
#define DP(format...)
#endif

#define RET_IF_ERROR(v, e) { if ((v = (e)) != KErrNone) return v; }

enum ESVPSnapReg {
	ESnapshot_Id = 0,
	ESnapshot_Address,
    ESnapshot_Length,
    ESnapshot_Trigger
  };

class DSVPSnapDriverFactory : public DLogicalDevice
{
 public:

  DSVPSnapDriverFactory();
  virtual TInt Install();
  virtual void GetCaps(TDes8& aDes) const;
  virtual TInt Create(DLogicalChannelBase*& aChannel);
};

class DSVPSnapChannel : public DLogicalChannel
{
 public:

  DSVPSnapChannel(DLogicalDevice* aLogicalDevice);
  ~DSVPSnapChannel();
  
  virtual TInt DoCreate(TInt aUnit, const TDesC* anInfo, const TVersion& aVer);	
  virtual void HandleMsg(TMessageBase* aMsg);
	
 protected:
  virtual void DoCancel(TInt aReqNo);
  virtual TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
  virtual TInt DoControl(TInt aFunction, TAny *a1, TAny *a2);

 private:
  TInt SaveVM(const TDesC8* aData);
  TInt LoadVM(const TDesC8* aData);
  
private:
  DThread* iClientThread;
  TDfcQue* iDFCQue;
  
  TBuf8<256> iSendDataBuffer;
};

#endif
