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

#include <e32base.h>   // CTrapCleanup

#include "rsvpsnapdriver.h"

#define SVPDBG
#ifdef SVPDBG
#include <e32debug.h>
#define DP(format...) RDebug::Printf(format)
#else
#define DP(format...)
#endif

_LIT8(KTestSendData,"kalle");

GLDEF_C TInt E32Main()
{
  DP("** (SNAPAPP) E32Main()");

  //  CTrapCleanup* cleanup;
  //  cleanup=CTrapCleanup::New();
  //  __UHEAP_MARK;

#if 0
  TInt err = User::LoadLogicalDevice(KSVPSnapDriverLDD);
  if (err==KErrAlreadyExists)
	DP("KErrAlreadyExists");
#endif
  
  RSVPSnapDriver drv;
  TInt err = drv.Open();
  
#if 1
  if (err==KErrNone)
	  {
	  
	drv.SaveVM(KTestSendData);
	  }
  else
	DP("Error");
#else
  drv.LoadVM(KTestSendData);   // Will never return
#endif
  
  //  __UHEAP_MARKEND;
  //  delete cleanup;
  return(KErrNone);
}
