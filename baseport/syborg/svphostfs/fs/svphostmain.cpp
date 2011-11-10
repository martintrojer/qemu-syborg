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

#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <f32dbg.h>

#include "svphostfsy.h"

#ifdef DP
#undef DP
#define DP(f...)
#endif

_LIT(KSVPHOSTFSY, "SVPHOSTFSY");

#if 1
GLDEF_C TInt E32Main()
    {
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();

	RSVPHostFsDriver driver;

	TInt r = driver.Open();

	if (r != KErrNone)
	  {
	    delete cleanup;
	    return(r);
	  }

	TUint32 driveMap[DRIVE_MAP_SIZE];

	r = driver.GetDriveMap(driveMap);

	driver.Close();
	 
	if (r != KErrNone)
	  {
	    delete cleanup;
	    return(r);
	  }

	RFs aFs;
	r=aFs.Connect();

	if (r != KErrNone) goto fail;

	r=aFs.AddFileSystem(KSVPHOSTFSY);

	if (r != KErrNone) goto close;

	for (TInt i = 0; i < DRIVE_MAP_SIZE ; i++ )
	        {
	        if (driveMap[i]) 
		        {
			char driveLetter = LOWEST_DRIVE + i;
			TInt driveNumber = driveLetter - 'A';
			DP(_L(" Drive %c to be mounted\n"), driveLetter);
			r = aFs.MountFileSystem(KSVPHOSTFSY,driveNumber);
			DP(_L("aFs.MountFileSystem(KSVPHOSTFSY, %c) -> %d"), driveNumber, r);
			if (r != KErrNone) goto close;
			}
		}

close:
	aFs.Close();

fail:
	delete cleanup;
	return(r);
    }

#else

GLDEF_C TInt E32Main()
    {
	CTrapCleanup* cleanup;
	cleanup=CTrapCleanup::New();

	RSVPHostFsDriver driver;

	TInt r = driver.Open();

	if (r != KErrNone){
	  delete cleanup;
	  return(r);
	}

	RFs aFs;
	r=aFs.Connect();

	r=aFs.AddFileSystem(KSVPHOSTFSY);
	DP(_L("r=aFs.AddFileSystem(KSVPHOSTFSY) -> %d\n"), r);

	// !@!
	// Until we have 'platform' device driver that we can use hardcode single virtual host drive as 'S'
	// !@!
	char driveLetter = 'S';
	TInt driveNumber = driveLetter - 'A';
	DP(_L(" Drive %c to be mounted\n"), driveLetter);
	r = aFs.MountFileSystem(KSVPHOSTFSY,driveNumber);
	DP(_L("aFs.MountFileSystem(KSVPHOSTFSY, %d) -> %d"), driveNumber, r);

	aFs.Close();

	delete cleanup;
	return(KErrNone);
    }

#endif
