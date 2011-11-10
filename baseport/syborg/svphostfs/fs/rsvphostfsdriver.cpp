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
* Sosco - bug fixes
*
* Description:
*
*/

#include <f32file.h>
#include <f32fsys.h>
#include <f32ver.h>
#include <e32uid.h>

#include <rsvphostfsdriver.h>

#include "svphostfsy.h"

#ifdef DP
#undef DP
#define DP(f...)
#endif

TInt RSVPHostFsDriver::Open()
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::Open()"));
	return DoCreate(KSVPHostFsDriverName, TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber), KNullUnit, NULL, NULL);
	}

TInt RSVPHostFsDriver::MkDir(TSVPHostFsMkDirInfo& aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::MkDir()"));
	return DoSVPRequest(EMkDir, &aInfo);
	}

TInt RSVPHostFsDriver::RmDir(TSVPHostFsRmDirInfo& aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::RmDir()"));
	return DoSVPRequest(ERmDir, &aInfo);
	}

TInt RSVPHostFsDriver::Delete(TSVPHostFsDeleteInfo& aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::Delete()"));
	return DoSVPRequest(EDelete, &aInfo);
	}

TInt RSVPHostFsDriver::Rename(TSVPHostFsRenameInfo& aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::Rename()"));
	return DoSVPRequest(ERename, &aInfo);
	}

TInt RSVPHostFsDriver::Replace(TSVPHostFsReplaceInfo& aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::Replace()"));
	return DoSVPRequest(EReplace, &aInfo);
	}

TInt RSVPHostFsDriver::ReadUid(const TDesC& /*aName*/,TEntry& /*anEntry*/)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::ReadUid()"));
	return KErrNone;
	}

TInt RSVPHostFsDriver::Entry(TSVPHostFsEntryInfo& aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::Entry()"));
	return DoSVPRequest(EEntry, &aInfo);
	}

TInt RSVPHostFsDriver::SetEntry(TSVPHostFsSetEntryInfo &aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::SetEntry()"));
	return  DoSVPRequest(ESetEntry, &aInfo);
	}

TInt RSVPHostFsDriver::FileOpen(TSVPHostFsFileOpenInfo& aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::FileOpen()"));
	return DoSVPRequest(EFileOpen, &aInfo);
	}

TInt RSVPHostFsDriver::FileClose(TUint32 aDrive, TUint32 aHandle)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::FileClose()"));

	// If attempt is made to access a file which does
	// not exist, it will not give a handle to close afterwards.
	// Attempting to call FileClose on this with an invalid handle
	// results in the error code KErrBadHandle (-8) being returned.
	// Some UI's cannot cope properly with this error code, resulting
	// in various servers repeatedly closing and reopening. To prevent
	// this,we trap it here and return KErrNone as though the call
	// completed properly without error.
	if (aHandle == 0)  
	  return KErrNone; // Bad handle, so exit immediately with KErrNone
	else

	return DoSVPRequest(EFileClose, (TAny *)aDrive, (TAny *)aHandle);
	}

TInt RSVPHostFsDriver::FileRead(TSVPHostFsFileReadInfo& aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::FileRead()"));
	return  DoSVPRequest(EFileRead, &aInfo);
	}

TInt RSVPHostFsDriver::FileWrite(TSVPHostFsFileWriteInfo& aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::FileWrite()"));
	return  DoSVPRequest(EFileWrite, &aInfo);
	}

TInt RSVPHostFsDriver::FileSetSize(TSVPHostFsFileSetSizeInfo &aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::FileSetSize()"));
	return  DoSVPRequest(EFileSetSize, &aInfo);
	}

TInt RSVPHostFsDriver::FileSetEntry(TSVPHostFsSetEntryInfo &aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::FileSetEntry()"));
	return  KErrNotSupported;
	}

TInt RSVPHostFsDriver::FlushData(TUint32 aDrive)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::FlushData()"));
	return DoSVPRequest(EFileFlushAll, (TAny *)aDrive);
	}

TInt RSVPHostFsDriver::FlushAll(TUint32 aDrive)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::FlushAll()"));
	return DoSVPRequest(EFileFlushAll, (TAny *)aDrive);
	}

TInt RSVPHostFsDriver::DirOpen(TSVPHostFsDirOpenInfo& aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::DirOpen()"));
	return DoSVPRequest(EDirOpen, &aInfo);
	}

TInt RSVPHostFsDriver::DirClose(TUint32 aDrive, TUint32 aHandle)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::DirClose()"));


	// If attempt is made to read a directory which does
	// not exist, it will not give a handle to close afterwards.
	// Attempting to call DirClose on this with an invalid handle
	// results in the error code KErrBadHandle (-8) being returned.
	// Some UI's cannot cope properly with this error code, resulting
	// in various servers repeatedly closing and reopening. To prevent
	// this,we trap it here and return KErrNone as though the call
	// completed properly without error.
	if (aHandle == 0)  
	  return KErrNone; // Bad handle, so exit immediately with KErrNone
	else

	return DoSVPRequest(EDirClose, (TAny *)aDrive, (TAny *)aHandle);
	}

TInt RSVPHostFsDriver::DirRead(TSVPHostFsDirReadInfo& aInfo)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::DirRead()"));
	return  DoSVPRequest(EDirRead, &aInfo);
	}

TUint32 RSVPHostFsDriver::GetDeviceID(TUint32 aDrive)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::GetDeviceID()"));

	TUint32 id;
	TInt r = DoSVPRequest(EGetDeviceID, (TAny *) aDrive, &id);
	return (r != KErrNone) ? r : id;
	}

TInt RSVPHostFsDriver::GetDriveMap(TUint32 * aMap)
	{
	DP(_L("** (rsvphostfsdriver.cpp) RSVPHostFsDriver::GetDeviceID()"));

	return DoSVPRequest(EGetDriveMap, (TAny *)aMap);
	}

