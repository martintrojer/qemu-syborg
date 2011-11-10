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

#ifndef __RSVPHOSTFSDRIVER_H__
#define __RSVPHOSTFSDRIVER_H__

#include <e32des16.h>

typedef TDes16 THostFileName;

typedef enum 
	{
	EUnknown=0,
	EWindows,
	EUnix,
	EMac,
	} TFileTimeType;

class TSVPHostFsMkDirInfo
	{
public:
	inline TSVPHostFsMkDirInfo() :
		iDrive(0),
		iName(0),
		iLength(0),
		iFlags(0)
		{};
	inline TSVPHostFsMkDirInfo(TUint aDrive, const THostFileName & aName, TUint32 aFlags) :
		iDrive(aDrive),
		iName(aName.Ptr()),
		iLength(aName.Length()),
		iFlags(aFlags)
		{};
public:
	TUint iDrive; 
	const TUint16 * iName;
	TUint32 iLength;
	TUint32 iFlags;
	};

class TSVPHostFsRmDirInfo
	{
public:
	inline TSVPHostFsRmDirInfo() :
		iDrive(0),
		iName(0),
		iLength(0)
		{};
	inline TSVPHostFsRmDirInfo(TUint aDrive, const THostFileName & aName) :
		iDrive(aDrive),
		iName(aName.Ptr()),
		iLength(aName.Length())
		{};
public:
	TUint iDrive; 
	const TUint16 * iName;
	TUint32 iLength;
	};

class TSVPHostFsDeleteInfo
	{
public:
	inline TSVPHostFsDeleteInfo() :
		iDrive(0),
		iName(0),
		iLength(0)
		{};
	inline TSVPHostFsDeleteInfo(TUint aDrive, const THostFileName & aName) :
		iDrive(aDrive),
		iName(aName.Ptr()),
		iLength(aName.Length())
		{};
public:
	TUint iDrive; 
	const TUint16 * iName;
	TUint32 iLength;
	};

class TSVPHostFsRenameInfo
	{
public:
	inline TSVPHostFsRenameInfo() :
		iDrive(0),
		iOldName(0),
		iOldLength(0),
		iNewName(0),
		iNewLength(0)
		{};
	inline TSVPHostFsRenameInfo(TUint aDrive, const THostFileName & aOldName, const THostFileName & aNewName) :
		iDrive(aDrive),
		iOldName(aOldName.Ptr()),
		iOldLength(aOldName.Length()),
		iNewName(aNewName.Ptr()),
		iNewLength(aNewName.Length())
		{};
public:
	TUint iDrive; 
	const TUint16 * iOldName;
	TUint32 iOldLength;
	const TUint16 * iNewName;
	TUint32 iNewLength;
	};

class TSVPHostFsReplaceInfo
	{
public:
	inline TSVPHostFsReplaceInfo() :
		iDrive(0),
		iOldName(0),
		iOldLength(0),
		iNewName(0),
		iNewLength(0)
		{};
	inline TSVPHostFsReplaceInfo(TUint aDrive, const THostFileName & aOldName, const THostFileName & aNewName) :
		iDrive(aDrive),
		iOldName(aOldName.Ptr()),
		iOldLength(aOldName.Length()),
		iNewName(aNewName.Ptr()),
		iNewLength(aNewName.Length())
		{};
public:
	TUint iDrive; 
	const TUint16 * iOldName;
	TUint32 iOldLength;
	const TUint16 * iNewName;
	TUint32 iNewLength;
	};

class TSVPHostFsEntryInfo
	{
public:
	inline TSVPHostFsEntryInfo() :
		iDrive(0),
		iName(0),
		iLength(0),
		iAtt(0),
		iModified(0),
		iSize(0),
		iTimeType(EUnknown)
		{};
	inline TSVPHostFsEntryInfo(TUint aDrive, const THostFileName & aName) :
		iDrive(aDrive),
		iName(aName.Ptr()),
		iLength(aName.Length()),
		iAtt(0),
		iModified(0),
		iSize(0),
		iTimeType(EUnknown)
		{};
public:
	TUint iDrive; 
	const TUint16 * iName;
	TUint32 iLength;
	TUint iAtt;
	TUint32 iModified; // time in seconds since the epoc
	TInt iSize;
	TFileTimeType iTimeType;
	char iHostName[KMaxFileName];
};

class TSVPHostFsSetEntryInfo
	{
public:
	inline TSVPHostFsSetEntryInfo() :
		iDrive(0),
		iName(0),
		iModified(0),
		iNewAtt(0),
		iTimeType(EUnknown)
		{};
	inline TSVPHostFsSetEntryInfo(TUint aDrive, const THostFileName & aName, TUint32 aModified, TUint aNewAtt) :
		iDrive(aDrive),
		iName(aName.Ptr()),
		iModified(aModified),
		iNewAtt(aNewAtt),
		iTimeType(EUnknown)
		{};
public:
	TUint iDrive; 
	const TUint16 *iName;
	TUint32 iModified; // time in seconds since the epoc
	TUint iNewAtt;
	TFileTimeType iTimeType;
	};

class TSVPHostFsDirOpenInfo
	{
public:
	inline TSVPHostFsDirOpenInfo() :
		iDrive(0),
		iName(0),
		iLength(0),
		iHandle(0)
		{};
inline TSVPHostFsDirOpenInfo(TUint aDrive, const THostFileName & aName) :
		iDrive(aDrive),
		iName(aName.Ptr()),
		iLength(aName.Length()),
		iHandle(0)
		{};
public:
	TUint iDrive; 
	const TUint16 * iName;
	TUint32 iLength;
	TUint32 iHandle;
	};

#define SVP_HOST_FS_INVALID_FILE_HANDLE -1
class TSVPHostFsFileOpenInfo
	{
public:
	inline TSVPHostFsFileOpenInfo() :
		iDrive(0),
		iName(0),
		iLength(0), // of file name
		iMode(0),
		iOpen(0),
		iAtt(0),
		iModified(0), // time in seconds since the epoc
		iSize(0),     // of file
		iTimeType(EUnknown),
		iHandle(SVP_HOST_FS_INVALID_FILE_HANDLE)
		{};
	inline TSVPHostFsFileOpenInfo(TUint aDrive, const THostFileName & aName, TUint32 aMode, TUint32 anOpen) :
		iDrive(aDrive),
		iName(aName.Ptr()),
		iLength(aName.Length()),
		iMode(aMode),
		iOpen(anOpen),
		iAtt(0),
		iModified(0), // time in seconds since the epoc
		iSize(0),     // of file
		iTimeType(EUnknown),
		iHandle(SVP_HOST_FS_INVALID_FILE_HANDLE)
		{};
public:
	TUint iDrive; 
	const TUint16 * iName;
	TUint32 iLength;
	TUint32 iMode;
	TUint32 iOpen;
	TUint iAtt;
	TUint32 iModified; // time in seconds since the epoc
	TInt iSize;
	TFileTimeType iTimeType;
	TInt iHandle;
	};

class TSVPHostFsFileReadInfo
	{
public:
	inline TSVPHostFsFileReadInfo() :
		iDrive(0),
		iHandle(0),
		iLength(0),
		iPos(0),
		iBuf(0)
		{};
	inline TSVPHostFsFileReadInfo(TUint aDrive, TUint aHandle,TInt aLength,TInt aPos, char * aBuf):
		iDrive(aDrive),
		iHandle(aHandle),
		iLength(aLength),
		iPos(aPos),
		iBuf(aBuf)
		{};
public:
	TUint iDrive; 
	TUint iHandle;
	TInt iLength;
	TInt iPos;
	char * iBuf;
	};	

class TSVPHostFsFileWriteInfo
	{
public:
	inline TSVPHostFsFileWriteInfo() :
		iDrive(0),
		iHandle(0),
		iLength(0),
		iPos(0),
		iBuf(0)
		{};
	inline TSVPHostFsFileWriteInfo(TUint aDrive, TUint aHandle,TInt aLength,TInt aPos, char * aBuf):
		iDrive(aDrive),
		iHandle(aHandle),
		iLength(aLength),
		iPos(aPos),
		iBuf(aBuf)
		{};
public:
	TUint iDrive; 
	TUint iHandle;
	TInt iLength;
	TInt iPos;
	char * iBuf;
	};

class TSVPHostFsFileSetSizeInfo
	{
public:
	inline TSVPHostFsFileSetSizeInfo() :
		iDrive(0),
		iHandle(0),
		iLength(0)
		{};
	inline TSVPHostFsFileSetSizeInfo(TUint aDrive, TUint aHandle, TInt aLength) :
		iDrive(aDrive),
		iHandle(aHandle),
		iLength(aLength)
		{};
public:
	TUint iDrive; 
	TUint iHandle;
	TInt iLength;
	};

class TSVPHostFsDirReadInfo
{
public:
	inline TSVPHostFsDirReadInfo() :
		iDrive(0),
		iHandle(0),
		iLength(0),
		iAtt(0),
		iModified(0),
		iSize(0),
		iTimeType(EUnknown)
		{};
	inline TSVPHostFsDirReadInfo(TUint aDrive, TUint32 aHandle) :
		iDrive(aDrive),
		iHandle(aHandle),
		iLength(-1),
		iAtt(0),
		iModified(666),
		iSize(-1),
		iTimeType(EUnknown)
		{};
public:
	TUint iDrive; 
	TUint32 iHandle;
	TInt iLength;
	TUint16 iName[KMaxPath];
	TUint iAtt;
	TUint32 iModified; // time in seconds since the epoc
	TInt iSize;
	TFileTimeType iTimeType;
	};

class TCapsSVPHostFsDriver
	{
public:
	TVersion	iVersion;
	};

_LIT(KSVPHostFsDriverName,"SVP Host Filesystem Driver");
_LIT(KSVPHostFsDriverLDD, "svphostfsdriver.ldd");

// Version information
const TInt KMajorVersionNumber=0;
const TInt KMinorVersionNumber=0;
const TInt KBuildVersionNumber=1;

// keep in sync with the definitions in QEMU/hw/svphostfs.c 
#define LOWEST_DRIVE 'A'
#define HIGHEST_DRIVE 'Z'
#define DRIVE_MAP_SIZE (HIGHEST_DRIVE - LOWEST_DRIVE + 1)

class RSVPHostFsDriver : public RBusLogicalChannel
	{
public:

	enum TControl
		{
		EDummu = 0,

		// Codes for CMountCB operations
		EMkDir,
		ERmDir,
		EDelete,
		ERename,
		EReplace,
		EReadUid,
		EEntry,
		ESetEntry,
		EFileOpen,
		EDirOpen,
		
		// Code for CFileCB operations
		EFileClose, 
		EFileRead,
		EFileWrite,
		EFileSetSize,
		EFileFlushAll,

		// Code for CDirCB operations
		EDirClose, 
		EDirRead,

		// Device ops
		EGetDeviceID,
		EGetDriveMap,

		};
	
		
public:
#ifndef __KERNEL_MODE__  // don't need to see these in the driver
	TInt Open();
	TInt MkDir(TSVPHostFsMkDirInfo& aInfo);
	TInt RmDir(TSVPHostFsRmDirInfo& aInfo);
	TInt Delete(TSVPHostFsDeleteInfo& aInfo);
	TInt Rename(TSVPHostFsRenameInfo& aInfo);
	TInt Replace(TSVPHostFsReplaceInfo& aInfo);
	TInt ReadUid(const TDesC& aName,TEntry& anEntry);
	TInt Entry(TSVPHostFsEntryInfo& aInfo);
	TInt SetEntry(TSVPHostFsSetEntryInfo &aInfo);
	TInt FileOpen(TSVPHostFsFileOpenInfo &aInfo);
	TInt DirOpen(TSVPHostFsDirOpenInfo& aInfo);
	TInt FileClose(TUint32 aDrive, TUint32 aHandle);
	TInt FileRead(TSVPHostFsFileReadInfo& aInfo);		
	TInt FileWrite(TSVPHostFsFileWriteInfo& aInfo);
	TInt FileSetSize(TSVPHostFsFileSetSizeInfo &aInfo);
	TInt FileSetEntry(TSVPHostFsSetEntryInfo &aInfo);
	TInt FlushData(TUint32 aDrive);
	TInt FlushAll(TUint32 aDrive);

	TInt DirClose(TUint32 aDrive, TUint32 aHandle);
	TInt DirRead(TSVPHostFsDirReadInfo& aInfo);

	TUint32 GetDeviceID(TUint32 aDrive);

	TInt GetDriveMap(TUint32 * aMap);

private:
	inline TInt DoSVPRequest(TInt aReqNo, TAny * a1) 
		{
		TRequestStatus status;
		DoRequest(aReqNo, status, a1);
		User::WaitForRequest(status);
		return status.Int();
		}

	inline TInt DoSVPRequest(TInt aReqNo, TAny * a1, TAny * a2) 
		{
		TRequestStatus status;
		DoRequest(aReqNo, status, a1, a2);
		User::WaitForRequest(status);
		return status.Int();
		}
#endif
	};

#endif // __rsvphostfsdriver_H__
