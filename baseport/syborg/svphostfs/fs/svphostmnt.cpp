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

#define HOST_SVP_DRIVE_SIZE 1024*1024*1024

// Fixed low mass-memory warning at startup
//#define HOST_SVP_DRIVE_FREE_SIZE 10*1024*1024
#define HOST_SVP_DRIVE_FREE_SIZE 100*1024*1024


LOCAL_C TInt GetMediaSize(TInt /*aDriveNumber*/,TInt64& aSize,TInt64& aFree)
//
// Return the size and free space on a drive.
//
	{
        DP(_L("** (SVPHOSTMNT) GetMediaSize"));

	aSize = HOST_SVP_DRIVE_SIZE;
	aFree = HOST_SVP_DRIVE_FREE_SIZE;
	return(KErrNone);
	}

LOCAL_C TInt GetVolume(TInt /*aDriveNumber*/,TDes& aName,TUint& aUniqueID)
//
// Return the volume name and uniqueID.
//
	{
        DP(_L("** (SVPHOSTMNT) GetVolume"));

	aUniqueID=1234;
	aName=(_L("SVPHOSTDRV"));
	return(KErrNone);	
	}

void CanonicalizePathname(const TDesC& aName, TInt aDrive, TDes& n, THostFileName& aHostName)
        {
	DP(_L("** (SVPHOSTMNT) CanonicalizePathname (%S)"), &aName);
	n += TDriveUnit(aDrive).Name();
	n += aName;
	TParse parse;
	parse.Set(n,NULL,NULL);
	n=parse.FullName();
	aHostName.Copy(n);
	DP(_L("-> (%S)"), &aHostName);
	}

void CanonicalizePathname(const TDesC& aName, TInt aDrive, THostFileName& aHostName)
        {
	TUint16 buf[KMaxPath];
	TPtr n(buf, KMaxPath);
	CanonicalizePathname(aName, aDrive, n, aHostName);
	}



//////////////////////////////////////////////////////////////////////////
//	CSVPHostMountCB							//
//////////////////////////////////////////////////////////////////////////	


CSVPHostMountCB::CSVPHostMountCB()
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::CSVPHostMountCB()"));

	__DECLARE_NAME(_S("CSVPHostMountCB"));
	}

CSVPHostMountCB::~CSVPHostMountCB()
	{
	DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::~CSVPHostMountCB()"));
	iDevice.Close();
	}

void CSVPHostMountCB::MountL(TBool /*aForceMount*/)
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::MountL()"));

	TInt err = iDevice.Open();
	User::LeaveIfError(err);

	TFileName driveName;
	TInt d=Drive().DriveNumber();
	iSize=HOST_SVP_DRIVE_SIZE;
	User::LeaveIfError(GetVolume(d,driveName,iUniqueID));
	HBufC* pN=driveName.AllocL();
	DP(_L("** (SVPHOSTMNT) ->SetVolumeName()"));
	SetVolumeName(pN);
	DP(_L("** (SVPHOSTMNT) <-SetVolumeName()"));

	}

TInt CSVPHostMountCB::ReMount()
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::ReMount()"));

	TFileName n;
	TInt d=Drive().DriveNumber();
	TUint uniqueID;
	TInt r=GetVolume(d,n,uniqueID);
	if (r!=KErrNone)
		return(r);
	if (n==VolumeName() && uniqueID==iUniqueID)
		return(KErrNone);
	return(KErrGeneral);
	}

void CSVPHostMountCB::Dismounted()
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::Dismounted()"));
	}

void CSVPHostMountCB::VolumeL(TVolumeInfo& aVolume) const
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::VolumeL()"));
	TInt64 s,f(0);
	TFileName n;
	TInt d=Drive().DriveNumber();
	User::LeaveIfError(GetMediaSize(d,s,f));
	aVolume.iFree=f;
	}

void CSVPHostMountCB::SetVolumeL(TDes& /*aName*/)
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::SetVolumeL()"));
	User::Leave(KErrNotSupported);
	}

void CSVPHostMountCB::IsFileInRom(const TDesC& /*aName*/,TUint8*& /*aFileStart*/)
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::IsFileInRom()"));
	}

void CSVPHostMountCB::MkDirL(const TDesC& aName)
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::MkDirL()"));
	TBuf<KMaxPath> name;
	TUint driveNumber = Drive().DriveNumber();
	CanonicalizePathname(aName, driveNumber, name);
	TSVPHostFsMkDirInfo info(driveNumber, name, 0777);
	User::LeaveIfError(SVP_HOST_FS_DEVICE().MkDir(info));
	}

void CSVPHostMountCB::RmDirL(const TDesC& aName)
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::RmDirL()"));
	TBuf<KMaxPath> name;
	TUint driveNumber = Drive().DriveNumber();
	CanonicalizePathname(aName, driveNumber, name);
	TSVPHostFsRmDirInfo info(driveNumber, name);
	User::LeaveIfError(SVP_HOST_FS_DEVICE().RmDir(info));
	}

void CSVPHostMountCB::DeleteL(const TDesC& aName)
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::DeleteL()"));
	TBuf<KMaxPath> name;
	TUint driveNumber = Drive().DriveNumber();
	CanonicalizePathname(aName, driveNumber, name);
	TSVPHostFsDeleteInfo info(driveNumber, name);
	User::LeaveIfError(SVP_HOST_FS_DEVICE().Delete(info));
	}

void CSVPHostMountCB::RenameL(const TDesC& anOldName,const TDesC& aNewName)
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::RenameL()"));
	// TODO: do we allow renaming across drives?
	TBuf<KMaxPath> oldName, newName;
	TUint driveNumber = Drive().DriveNumber();
	CanonicalizePathname(anOldName, driveNumber, oldName);
	CanonicalizePathname(aNewName, driveNumber, newName);
	TSVPHostFsRenameInfo info(driveNumber, oldName, newName);
	User::LeaveIfError(SVP_HOST_FS_DEVICE().Rename(info));
	}

void CSVPHostMountCB::ReplaceL(const TDesC& anOldName,const TDesC& aNewName)
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::ReplaceL()"));

	if(FileNamesIdentical(anOldName,aNewName))
		{
		return;
		}
	TBuf<KMaxPath> oldName, newName;
	TUint driveNumber = Drive().DriveNumber();
	CanonicalizePathname(anOldName, driveNumber, oldName);
	CanonicalizePathname(aNewName, driveNumber, newName);
	TSVPHostFsReplaceInfo info(driveNumber, oldName, newName);
	User::LeaveIfError(SVP_HOST_FS_DEVICE().Replace(info));
	}

void CSVPHostMountCB::ReadUidL(const TDesC& aName,TEntry& anEntry) const
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::ReadUidL()"));
	TBuf<KMaxPath> name;
	TUint driveNumber = Drive().DriveNumber();
	CanonicalizePathname(aName, driveNumber, name);
	TSVPHostFsFileOpenInfo fileOpenInfo(driveNumber, name,EFileRead,EFileOpen);
	TInt err = SVP_HOST_FS_DEVICE().FileOpen(fileOpenInfo);

	User::LeaveIfError(err);

	TBuf8<sizeof(TCheckedUid)> uidBuf;
	uidBuf.SetLength(sizeof(TCheckedUid));

	TSVPHostFsFileReadInfo fileReadInfo(driveNumber, fileOpenInfo.iHandle,sizeof(TCheckedUid),0,(char*)uidBuf.Ptr());

	if (KErrNone != SVP_HOST_FS_DEVICE().FileRead(fileReadInfo))
		User::LeaveIfError(SVP_HOST_FS_DEVICE().FileClose(driveNumber, fileOpenInfo.iHandle));

	DP(_L("** (SVPHOSTMNT) CSVPHostFileCB::ReadUidL sizeof(TCheckedUid) %d fileOpenInfo.iLength %d "), sizeof(TCheckedUid), fileReadInfo.iLength);

	if (fileReadInfo.iLength!=sizeof(TCheckedUid))
	        User::LeaveIfError(SVP_HOST_FS_DEVICE().FileClose(driveNumber, fileOpenInfo.iHandle));

	TCheckedUid uid(uidBuf);
	anEntry.iType=uid.UidType();

	User::LeaveIfError(SVP_HOST_FS_DEVICE().FileClose(driveNumber, fileOpenInfo.iHandle));
	}

void CSVPHostMountCB::EntryL(const TDesC& aName,TEntry& anEntry) const
	{
	DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::EntryL(%S)"), &aName);
	TBuf<KMaxPath> name;
	TUint driveNumber = Drive().DriveNumber();
	CanonicalizePathname(aName, driveNumber, name);
	TSVPHostFsEntryInfo info(driveNumber, name);
	User::LeaveIfError(SVP_HOST_FS_DEVICE().Entry(info));
	anEntry.iAtt=info.iAtt&KEntryAttMaskSupported;
	anEntry.iSize=info.iSize;
	fileTimeToTime(info.iModified,anEntry.iModified, info.iTimeType);
	if (anEntry.iSize>=(TInt)sizeof(TCheckedUid))
		{
		ReadUidL(aName,anEntry);
		}
	}

void timeToFileTime(TUint32& t,const TTime& aTime, TFileTimeType aType);

void CSVPHostMountCB::SetEntryL(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask)
	{
	DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::SetEntryL()"));

	//User::Leave(KErrNotSupported);
	}

void CSVPHostMountCB::FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile)
	{
	DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::FileOpenL(%S)"), &aName);
	CSVPHostFileCB& file=(*((CSVPHostFileCB*)aFile));

	TBuf<KMaxPath> name;
	TUint driveNumber = Drive().DriveNumber();
	CanonicalizePathname(aName, driveNumber, name);
	TSVPHostFsFileOpenInfo info(driveNumber, name,aMode,anOpen);
	TInt err = SVP_HOST_FS_DEVICE().FileOpen(info);

	User::LeaveIfError(err);

	file.SetHandle(info.iHandle);
	file.SetSize(info.iSize);
	file.SetAtt(info.iAtt&KEntryAttMaskSupported);
	TTime tempTime=file.Modified();
	fileTimeToTime(info.iModified, tempTime, info.iTimeType);
	file.SetModified(tempTime);
	}

void CSVPHostMountCB::DirOpenL(const TDesC& aName ,CDirCB* aDir)
	{
	DP(_L("CFatMountCB::DirOpenL, drv:%d, %S"), DriveNumber(), &aName);
	CSVPHostDirCB& dir=(*((CSVPHostDirCB*)aDir));

	TBuf<KMaxPath> name;
	TUint driveNumber = Drive().DriveNumber();
	CanonicalizePathname(aName, driveNumber, name);
	TSVPHostFsDirOpenInfo info(driveNumber, name);
	TInt err = SVP_HOST_FS_DEVICE().DirOpen(info);

	User::LeaveIfError(err);

	dir.SetHandle(info.iHandle);
	TFileName n(TDriveUnit(Drive().DriveNumber()).Name());
	n.Append(aName);
	dir.SetFullName(n);
	}

void CSVPHostMountCB::RawReadL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aTrg*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/) const
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::RawReadL()"));
	User::Leave(KErrNotSupported);
	}

void CSVPHostMountCB::RawWriteL(TInt64 /*aPos*/,TInt /*aLength*/,const TAny* /*aSrc*/,TInt /*anOffset*/,const RMessagePtr2& /*aMessage*/)
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::RawWriteL()"));
	User::Leave(KErrNotSupported);
	}

void CSVPHostMountCB::GetShortNameL(const TDesC& aLongName,TDes& aShortName)
	{
	DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::GetShortNameL(%S)"), &aLongName);
	aShortName = aLongName;
	}

void CSVPHostMountCB::GetLongNameL(const TDesC& aShortName,TDes& aLongName)
	{
	DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::GetLongNameL(%S)"), &aShortName);
	aLongName = aShortName;
	}

void CSVPHostMountCB::ReadSectionL(const TDesC& /*aName*/,TInt /*aPos*/,TAny* /*aTrg*/,TInt /*aLength*/,const RMessagePtr2& /*aMessage*/)
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::RawSectionL()"));
	User::Leave(KErrNotSupported);
	}

TBool CSVPHostMountCB::IsRomDrive() const
	{
        DP(_L("** (SVPHOSTMNT) CSVPHostMountCB::IsRomDrive()"));
	return(EFalse);
	}
