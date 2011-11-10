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

#include <e32std.h>    // for User::
#include "rsvpsnapdriver.h"

//#define SVPDBG
#ifdef SVPDBG
#include <e32debug.h>
#define DP(format...) RDebug::Printf(format)
#else
#define DP(format...)
#endif

TInt RSVPSnapDriver::Open(void)
{
  DP("** RSVPSnapDriver::Open()");

  const TVersion ver = TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);

  return DoCreate(KSVPSnapDriverName,
				  ver,
				  KNullUnit,
				  NULL,
				  NULL);
}

TInt RSVPSnapDriver::SaveVM(const TDesC8& aData)
{
  DP("** RSVPSnapDriver::SaveVM");
  
  //DP("** RSVPSnapDriver::SaveVM: - length of data=%d", length);
  //DP("** RSVPSnapDriver::SaveVM: - aName= %s", name.Ptr());
	
  return DoSVPRequest(ESaveVM,(TAny*)&aData);
 }

TInt RSVPSnapDriver::LoadVM(const TDesC8& aData)
{
  DP("** RSVPSnapDriver::LoadVM");
  return DoSVPRequest(ELoadVM,(TAny*)&aData);
}
