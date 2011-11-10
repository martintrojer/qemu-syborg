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

#ifndef __RSVPSNAPDRIVER_H
#define __RSVPSNAPDRIVER_H

#include <e32cmn.h>    // for RBusLogicalChannel

class TCapsSVPSnapDriver
{
public:
	TVersion	iVersion;
};

_LIT(KSVPSnapDriverName,"SVP Snapshot Driver");
_LIT(KSVPSnapDriverLDD, "svpsnapdriver.ldd");

// Version information
const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=1;

class RSVPSnapDriver : public RBusLogicalChannel
{
 public:
  enum TControl
	{
	  EDummy = 0,
	  ESaveVM,
	  ELoadVM
	};

 public:
#ifndef __KERNEL_MODE__
  TInt Open(void);
  TInt SaveVM(const TDesC8& aData);
  TInt LoadVM(const TDesC8& aData);

private:
  inline TInt DoSVPRequest(TInt aReqNo, TAny * a1) 
  {
	TRequestStatus status;
	DoRequest(aReqNo, status, a1);
	User::WaitForRequest(status);
	return status.Int();
  }
#endif
};

#endif
