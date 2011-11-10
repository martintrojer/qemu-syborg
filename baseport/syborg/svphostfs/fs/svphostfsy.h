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
* Description: Based on the test code under f32test\fsstress
*
*/

#if !defined(__SVPHOSTFSY_H__)
#define __SVPHOSTFSY_H__

#include <f32fsys.h>
#include <f32file.h>
#include <e32test.h>
#include <e32hal.h>
#include <e32math.h>
#include <f32dbg.h>

#include <rsvphostfsdriver.h>

//
// Common constants used by both EFSRV and the filesystems
//

const TUint KEntryAttIllegal=(KEntryAttVolume|KEntryAttDir);
const TUint KEntryAttModified=0x20000000;
const TUint KEntryAttMustBeFile=0x80000000;
const TInt KCurrentPosition=KMinTInt;


GLDEF_D const TInt KMaxParses=7;
GLDEF_D const TInt KHeapSize=0x2000;
GLREF_C void TurnAllocFailureOff();
GLREF_C void TurnAllocFailureOn();
GLREF_C void ReportCheckDiskFailure(TInt aRet);
GLREF_D RTest test;
GLREF_D TFileName gSessionPath;
GLREF_D TInt gAllocFailOff;
GLREF_D TInt gAllocFailOn;

#if defined(_DEBUG)
#define SetAllocFailure(a) SetAllocFailure(a)
#else
#define SetAllocFailure(a) IsRomAddress(NULL)
#define KAllocFailureOn 0
#define KAllocFailureOff 0
#endif

// Debug virtual host file service - uncomment define below
//#define SVPDBG
#ifdef SVPDBG
IMPORT_C TUint32 DebugRegister();
#define DP(format...) { if (DebugRegister()&KFSYS) RDebug::Print(format);}
#else
#define DP(format...)
#endif

#define SVP_HOST_FS_DEVICE() (((CSVPHostMountCB&)Mount()).Device())
#define SVP_HOST_FS_DEVICE_ID 0xc51d0008

enum TPanic
	{
	EFileTimeToSystemTime,
	EFileClose,
	EFileCloseSetAttributes,
	EDirClose,
	EMapCouldNotConnect
	};

// utility to convert file system times to TTime format
void fileTimeToTime(TUint32 t,TTime& aTime, TFileTimeType aType);

// utility to produce a canonicalized pathname (i.e. with a drive letter) suitable to pass to the FS device.

void CanonicalizePathname(const TDesC& aName, TInt aDrive, THostFileName& aHostName);

class CSessionFs;

class CSVPHostMountCB : public CMountCB
	{
public:
	CSVPHostMountCB();
	~CSVPHostMountCB();
	void MountL(TBool aForceMount);
	TInt ReMount();
	void Dismounted();
	void VolumeL(TVolumeInfo& aVolume) const;
	void SetVolumeL(TDes& aName);
	void MkDirL(const TDesC& aName);
	void RmDirL(const TDesC& aName);
	void DeleteL(const TDesC& aName);
	void RenameL(const TDesC& anOldName,const TDesC& anNewName);
	void ReplaceL(const TDesC& anOldName,const TDesC& anNewName);
	void EntryL(const TDesC& aName,TEntry& anEntry) const;
	void SetEntryL(const TDesC& aName,const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	void FileOpenL(const TDesC& aName,TUint aMode,TFileOpen anOpen,CFileCB* aFile);
	void DirOpenL(const TDesC& aName,CDirCB* aDir);
	void RawReadL(TInt64 aPos,TInt aLength,const TAny* aTrg,TInt anOffset,const RMessagePtr2& aMessage) const;
	void RawWriteL(TInt64 aPos,TInt aLength,const TAny* aSrc,TInt anOffset,const RMessagePtr2& aMessage);
	void ReadUidL(const TDesC& aName,TEntry& anEntry) const;
	void GetShortNameL(const TDesC& aLongName,TDes& aShortName);
	void GetLongNameL(const TDesC& aShortName,TDes& aLongName);
	void IsFileInRom(const TDesC& aFileName,TUint8*& aFileStart);
	void ReadSectionL(const TDesC& aName,TInt aPos,TAny* aTrg,TInt aLength,const RMessagePtr2& aMessage);

	inline const CSVPHostMountCB& Mount() const { return *this; }
	inline RSVPHostFsDriver& Device() { return iDevice; }
	

private:
	TBool IsRomDrive() const;
	RSVPHostFsDriver iDevice;
	};


class RConsole;
class CSVPHostFileCB : public CFileCB
	{
public:
	CSVPHostFileCB();
	~CSVPHostFileCB();
	void RenameL(const TDesC& aNewName);
	void ReadL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	void WriteL(TInt aPos,TInt& aLength,const TAny* aDes,const RMessagePtr2& aMessage);
	TInt Address(TInt& aPos) const;
	void SetSizeL(TInt aSize);
	void SetEntryL(const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask);
	void FlushDataL();
	void FlushAllL();
	void CheckPos(TInt aPos);
	void SetHandle(TUint aHandle) { iHandle = aHandle; }
	TUint Handle() const { return iHandle; }
private:
	TBool IsRomDrive() const;
private:
	TInt iCurrentPos;
	//	TUint8* iFilePtr;
	TUint iHandle;
	};

class CSVPHostDirCB : public CDirCB
	{
public:
	CSVPHostDirCB(/*CSessionFs* aSession*/);
	~CSVPHostDirCB();
	void ReadL(TEntry& anEntry);
	inline void SetFullName(const TDesC& aName) {iFullName.Set(aName,NULL,NULL);}
	inline void SetHandle(TUint32 aHandle) { iHandle = aHandle; }

private:
	TBool MatchUid();
public:
	TEntry iEntry;
private:
	TUint32 iHandle;
	TParse iFullName;
	};

class CSVPHostFormatCB : public CFormatCB
	{
public:
	CSVPHostFormatCB(/*CSessionFs* aSession*/);
	~CSVPHostFormatCB();
public:
	virtual void DoFormatStepL();
	};

class CSVPHostFileSystem : public CFileSystem
	{
public:
	CSVPHostFileSystem();
	~CSVPHostFileSystem();
	TInt Install();
	TInt DefaultPath(TDes& aPath) const;
	void DriveInfo(TDriveInfo& anInfo,TInt aDriveNumber) const;
private:
	CMountCB* NewMountL() const;
	CFileCB* NewFileL() const;
	CDirCB* NewDirL() const;
	CFormatCB* NewFormatL() const;
public:
	static CFileSystem* NewL();

public:
	inline RSVPHostFsDriver& Device() { return iDevice; };
private:
	RSVPHostFsDriver iDevice;
	TUint32 iDriveMap[DRIVE_MAP_SIZE];
	};

#endif


