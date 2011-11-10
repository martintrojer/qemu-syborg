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
#include <f32fsys.h>
#include <f32ver.h>
#include <e32uid.h>

#include "svphostfsy.h"

//////////////////////////////////////////////////////////////////////////
// CSVPHostFileSystem	                                                //
//////////////////////////////////////////////////////////////////////////	

CSVPHostFileSystem::CSVPHostFileSystem()
//
// Constructor
//
	{
	DP(_L("** (SVPHOSTFSY) CSVPHostFileSystem::CSVPHostFileSystem()"));
	__DECLARE_NAME(_S("CSVPHostFileSystem"));
	}

CSVPHostFileSystem::~CSVPHostFileSystem()
//
// Destructor
//
	{
	DP(_L("** (SVPHOSTFSY) CSVPHostFileSystem::~CSVPHostFileSystem"));
	iDevice.Close();
	}



TInt CSVPHostFileSystem::Install()
//
// Install the file system.
//
	{
	DP(_L("** (SVPHOSTFSY) CSVPHostFileSystem::Install()"));

	// Open the device
	RSVPHostFsDriver device;
	TInt err = device.Open();

	if (KErrNone != err)
		return err;

	iVersion=TVersion(KMajorVersionNumber,KMinorVersionNumber,KF32BuildVersionNumber);
	TPtrC name=_L("SVPHOSTFSY");
	
	return(SetName(&name));
	}

CMountCB* CSVPHostFileSystem::NewMountL(/*CSessionFs* aSession*/) const
//
// Create a new mount control block.
//
	{
	DP(_L("** (SVPHOSTFSY) CSVPHostFileSystem::NewMountL()"));
	return(new(ELeave) CSVPHostMountCB);
	}

CFileCB* CSVPHostFileSystem::NewFileL(/*CSessionFs* aSession*/) const
//
// Create a new file.
//
	{
	DP(_L("** (SVPHOSTFSY) CSVPHostFileSystem::NewFileL()"));
	return(new(ELeave) CSVPHostFileCB);
	}

CDirCB* CSVPHostFileSystem::NewDirL(/*CSessionFs* aSession*/) const
//
// Create a new directory lister.
//
	{
	DP(_L("** (SVPHOSTFSY) CSVPHostFileSystem::NewDirL()"));
	return(new(ELeave) CSVPHostDirCB(/*aSession*/));
	}

CFormatCB* CSVPHostFileSystem::NewFormatL(/*CSessionFs* aSession*/) const
//
// Create a new media formatter.
//
	{
	DP(_L("** (SVPHOSTFSY) CSVPHostFileSystem::NewFormatL()"));
	return(new(ELeave) CSVPHostFormatCB(/*aSession*/));
	}

TInt CSVPHostFileSystem::DefaultPath(TDes& aPath) const
//
// Return the initial default path.
//
	{
	DP(_L("** (SVPHOSTFSY) CSVPHostFileSystem::DefaultPath()"));
	aPath=_L("\\");
	return(KErrNone);
	}


CFileSystem* CSVPHostFileSystem::NewL()
//
//	Factory for CSVPHostFileSystem
//
	{
	DP(_L("** (SVPHOSTFSY) CSVPHostFileSystem::NewL()"));
	CSVPHostFileSystem* svpHostFsy=new(ELeave) CSVPHostFileSystem();
	return svpHostFsy;
	}


void CSVPHostFileSystem::DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const
//
// Return the drive info iDriveAtt  && iBatteryState already set
//
	{
	DP(_L("** (SVPHOSTFSY) CSVPHostFileSystem::DriveInfo()"));

	anInfo.iMediaAtt=KMediaAttVariableSize;
//  SF BUG 1313 - T_FSYS fails - start
// Media type changed from RAM drive to Hard Disk - HOSTFS cannot use FAT file system so various
// tests fail, but T_FSYS fails on any drive as all available drives are exercised by the test.
// EMediaHardDisk is more appropriate.
	anInfo.iType=EMediaHardDisk;  // was EMediaRam;
// SF BUG 1312 - T_FSYS fails - end	

#if 0
	// !@! can't execute from remote drives
	anInfo.iDriveAtt=KDriveAttRemote;
#endif
	anInfo.iDriveAtt=KDriveAttLocal|KDriveAttInternal;
	}


extern "C" {

EXPORT_C CFileSystem* CreateFileSystem()
//
// Create a new file system
//
	{
	DP(_L("** (SVPHOSTFSY) CreateFileSystem()"));

	return(CSVPHostFileSystem::NewL());
	}
}


