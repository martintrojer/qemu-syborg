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

#ifndef __SVPHOSTFSDRIVER_H__
#define __SVPHOSTFSDRIVER_H__


#include "rsvphostfsdriver.h"

// Debug messages
//#define SVPDBG
#ifdef SVPDBG
#define LOG_MSG(x) Kern::Printf(x)
#define LOG_MSG2(x, y) Kern::Printf(x, y)
#else
#define LOG_MSG(x)
#define LOG_MSG2(x, y)
#endif


//
// class DSVPHostFsDriverFactory
//
class DSVPHostFsDriverFactory : public DLogicalDevice
{
public:

	DSVPHostFsDriverFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
};

//
// DSVPHostFsChannel
//
class DSVPHostFsChannel : public DLogicalChannel
{
public:

	DSVPHostFsChannel(DLogicalDevice* aLogicalDevice);
	~DSVPHostFsChannel();

	virtual TInt DoCreate(TInt aUnit, const TDesC* anInfo, const TVersion& aVer);	
	virtual void HandleMsg(TMessageBase* aMsg);
	
protected:
	virtual void DoCancel(TInt aReqNo);
	virtual TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	virtual TInt DoControl(TInt aFunction, TAny *a1, TAny *a2);
	
private:
	TInt DirOpen(TSVPHostFsDirOpenInfo* aInfo);
	TInt DirClose(TUint32 aHandle);
	TInt DirRead(TSVPHostFsDirReadInfo* aInfo);

private:
	DThread* iClientThread;
	
	
	TDfcQue* iDFCQue;

};

#endif //__SVPHOSTFSDRIVER_H__
