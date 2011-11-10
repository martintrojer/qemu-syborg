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

/*
 *
 * This file starts the HOSTFS local drive mapping
 *
 */

#include <e32std.h>
#include "estart.h"

#define __ENABLE_HOSTFS

/*!
 * Customised estart class
 */
class TSyborgFSStartup : public TFSStartup
	{
public:
#ifdef __ENABLE_HOSTFS
	virtual void InitHostFS();
#endif
	};
	

#ifdef __ENABLE_HOSTFS

/*
 *
 * Run the Host FS starter program
 *
 */ 
void TSyborgFSStartup::InitHostFS()
	{ 
_LIT(KHostFSMounter, "z:\\sys\\bin\\SVPHOSTFS.EXE");

	RProcess ws;
	TInt r=ws.Create(KHostFSMounter, KNullDesC);
	if (r == KErrNone)
        {
		TRequestStatus stat;
		ws.Rendezvous(stat);
		ws.Resume();
		User::WaitForRequest(stat);		// wait for start or death
		ws.Close();
		}
	}	
#endif


/*!
 * Estart entry point
 */
GLDEF_C TInt E32Main()
	{
	
	TSyborgFSStartup fsStart;
	fsStart.Init();
	
	fsStart.Run();
#ifdef __ENABLE_HOSTFS
	fsStart.InitHostFS();
#endif
	fsStart.StartSystem();
			
	fsStart.Close();
	return(0);
	}
