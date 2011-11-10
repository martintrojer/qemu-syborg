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
* Description: Device driver for SVP Host file system access
*
*/

#ifdef __WINS__
#error - this driver cannot be built for emulation
#endif

#include <e32def.h>
#include <e32cmn.h>
#include <u32std.h>
#include <kernel.h>
#include <kern_priv.h>
#include <arm.h>
#include <cache.h>
#include <nkern.h>
#include <u32hal.h>

#include <system.h>

#include "rsvphostfsdriver.h"

#include "libfdt.h"

// Debug messages - uncomment define below
//#define SVPDBG 
#ifdef SVPDBG
#define DP(format...) Kern::Printf(format)
#else
#define DP(format...)
#endif
		
#define SVP_HOST_FS_DEVICE_ID  0xc51d0008
#define SVP_PLATFORM_DEVICE_ID 0xc51d1000
		
class DSVPHostFsDriverFactory : public DLogicalDevice
{
public:

	DSVPHostFsDriverFactory();
	virtual TInt Install();
	virtual void GetCaps(TDes8& aDes) const;
	virtual TInt Create(DLogicalChannelBase*& aChannel);
};

class DSVPHostFsChannel : public DLogicalChannel
{
public:

	DSVPHostFsChannel(DLogicalDevice* aLogicalDevice);
	~DSVPHostFsChannel();

	virtual TInt DoCreate(TInt aUnit, const TDesC* anInfo, const TVersion& aVer);	
	virtual void HandleMsg(TMessageBase* aMsg);
	
protected:
	virtual void DoCancel(TInt aReqNo);
	virtual TInt DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2);
	virtual TInt DoControl(TInt aFunction, TAny *a1, TAny *a2);
	
private:
	TInt MkDir(TSVPHostFsMkDirInfo* aInfo);
	TInt RmDir(TSVPHostFsRmDirInfo* aInfo);
	TInt Delete(TSVPHostFsDeleteInfo* aInfo);
	TInt Rename(TSVPHostFsRenameInfo* aInfo);
	TInt Replace(TSVPHostFsReplaceInfo* aInfo);
	TInt Entry(TSVPHostFsEntryInfo* aInfo);
	TInt SetEntry(TSVPHostFsSetEntryInfo* aInfo);
	TInt FileOpen(TSVPHostFsFileOpenInfo* aInfo);
	TInt FileClose(TUint32 aDrive, TUint32 aHandle);
	TInt FileRead(TSVPHostFsFileReadInfo* aInfo);
	TInt FileWrite(TSVPHostFsFileWriteInfo* aInfo);
	TInt FileSetSize(TSVPHostFsFileSetSizeInfo* aInfo);
	TInt FileSetEntry(TSVPHostFsSetEntryInfo* aInfo);
	TInt DirOpen(TSVPHostFsDirOpenInfo* aInfo);
	TInt Flush(TUint32 aDrive);
	TInt DirClose(TUint32 aDrive, TUint32 aHandle);
	TInt DirRead(TSVPHostFsDirReadInfo* aInfo);

	TInt GetID(TUint32 aDrive, TUint32 * aId);

        TInt SetUpDrives();
	TInt GetDriveMap(TAny * aMap);

private:
	DThread* iClientThread;
	TDfcQue* iDFCQue;
	TUint32 iDriveMap[DRIVE_MAP_SIZE] ; 

};


#define RET_IF_ERROR(v, e) { if ((v = (e)) != KErrNone) return v; }


#define	EDeviceID	0
#define	EOp		1
#define ETreeStart	1
#define	EResult		2
#define	EArg0		3
#define	EArg1		4
#define	EArg2		5
#define	EArg3		6

static inline TUint32 SVPReadReg(TUint32 dev, TUint32 aReg)
	{
	DP("** ReadReg @ 0x%08x (%d)",dev,aReg);

	return *(volatile TUint32 *)(dev + (aReg << 2));
	}

static inline void SVPWriteReg(TUint32 dev, TUint32 aReg, TUint32 aVal)
	{
	DP("** WriteReg @ 0x%08x (%d,%d)",dev,aReg,aVal);

	*(volatile TUint32*)(dev + (aReg << 2)) = aVal;
	}

static inline void SVPInvoke(TUint32 dev, TUint32 aVal)
	{
	DP("** Invoke @ 0x%08x (%d)",dev,aVal);

	*(TUint32*)(dev + (EOp << 2)) = aVal;
	}

/////////////////////////////////////////////////////////////////////////
//
// DSVPHostFsDriverFactory
//
/////////////////////////////////////////////////////////////////////////

//
// DSVPHostFsDriverFactory constructor
//
DSVPHostFsDriverFactory::DSVPHostFsDriverFactory()
	{
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsDriverFactory::DSVPHostFsDriverFactory()");

	iVersion = TVersion(KMajorVersionNumber,KMinorVersionNumber,KBuildVersionNumber);    
	}

//
// DSVPHostFsDriverFactory::Create
//
TInt DSVPHostFsDriverFactory::Create(DLogicalChannelBase*& aChannel)
	{
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsDriverFactory::Create()");

	aChannel = new DSVPHostFsChannel(this);
	
	return aChannel ? KErrNone : KErrNoMemory;
	}

//
// DSVPHostFsDriverFactory::Install
//
TInt DSVPHostFsDriverFactory::Install()
	{
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsDriverFactory::Install()");

	return(SetName(&KSVPHostFsDriverName));
	}

//
// DSVPHostFsDriverFactory::Install
//
void DSVPHostFsDriverFactory::GetCaps(TDes8& aDes) const
	{
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsDriverFactory::GetCaps()");

	TCapsSVPHostFsDriver b;
	b.iVersion = TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber);
	aDes.FillZ(aDes.MaxLength());
	aDes.Copy((TUint8 *)&b, Min(aDes.MaxLength(), sizeof(b)));
	}


/////////////////////////////////////////////////////////////////////////
//
// DSVPHostFsChannel implementation
//
/////////////////////////////////////////////////////////////////////////

//
// DSVPHostFsChannel constructor
//
DSVPHostFsChannel::DSVPHostFsChannel(DLogicalDevice* aLogicalDevice)
	{
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::DSVPHostFsChannel()");

	iDevice = aLogicalDevice;
	
	iClientThread = &Kern::CurrentThread();
	iClientThread->Open();
	}

//
// DSVPHostFsChannel destructor
//
DSVPHostFsChannel::~DSVPHostFsChannel()
	{
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::~DSVPHostFsChannel()");
	Kern::SafeClose((DObject*&)iClientThread, NULL);
	}

//
// DSVPHostFsChannel::DoCreate
//
TInt DSVPHostFsChannel::DoCreate(TInt /*aUnit*/, const TDesC* anInfo, const TVersion& aVer)
	{
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::DoCreate()");

  	if (!Kern::QueryVersionSupported(TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber), aVer))
		return KErrNotSupported; 
  	
		       
	//Setup the driver for receiving client messages
	SetDfcQ(Kern::DfcQue0());
	iMsgQ.Receive();  	

	SetUpDrives();
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::DoCreate()- checking device");
	TUint id = SVPReadReg(KHwSVPHostFileSystemDevice, EDeviceID);
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::DoCreate()- checked device- 0x%08x -expected 0x%08x", 
		id, SVP_HOST_FS_DEVICE_ID);
	return id == SVP_HOST_FS_DEVICE_ID ? KErrNone :	KErrHardwareNotAvailable;
	}

TInt DSVPHostFsChannel::SetUpDrives()
	{
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::SetUpDrives() @ 0x%08x", KHwSVPPlatformDevice) ;
	TUint32 platId = SVPReadReg(KHwSVPPlatformDevice, EDeviceID); 
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::SetUpDrives()- checked device- 0x%08x -expected 0x%08x", 
		platId, SVP_PLATFORM_DEVICE_ID);
	if (platId != SVP_PLATFORM_DEVICE_ID) return KErrHardwareNotAvailable;

	TUint32 * fdt = (TUint32 *)((char *)(SVPReadReg(KHwSVPPlatformDevice, ETreeStart) + KHwSVPPlatformDevice)); 
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::SetUpDrives()- device tree @ 0x%08x", fdt);

	// Iteratate over the tree looking for "syborg,hostfs" nodes.
	const char * compatible = "syborg,hostfs" ; 
	int offset = fdt_node_offset_by_compatible(fdt, -1, compatible);
	while (offset != -FDT_ERR_NOTFOUND) 
		{
		if (offset < 0)
			{
			DP("FDT: Node not found %d", offset) ;
			return KErrHardwareNotAvailable	; 
			}

		int lenp1, lenp2 = 0; 
		TUint32 * deviceAddressp = (TUint32 *)fdt_getprop(fdt,offset,"reg",&lenp1); 

		if (!deviceAddressp)
			{
			DP("FDT format error: reg %d", lenp1);
			return KErrHardwareNotAvailable	; 
			}


		TUint32 * driveNumberp = (TUint32 *)fdt_getprop(fdt,offset,"drive-number",&lenp2);
		if (!driveNumberp)
			{
			DP("FDT format error: drive-number %d", lenp2);
			return KErrHardwareNotAvailable	; 
			}
		TUint32 deviceAddressPhys = bswap_32(*deviceAddressp);

#define PhysicalToLinear(addr) ((addr & (~(Epoc::LinearToPhysical(KPrimaryIOBase)))) | KPrimaryIOBase)
	
		TUint32 deviceAddressLin = PhysicalToLinear(deviceAddressPhys) ; 
		TUint32 driveNumber = bswap_32(*driveNumberp) ;
		DP("FDT: dev address phys 0x%08x lin 0x%08x len1 %d drive number %08x len2 %d", 
			deviceAddressPhys, deviceAddressLin, lenp1, driveNumber, lenp2) ; 
		TUint32 fsId = SVPReadReg(deviceAddressLin, EDeviceID) ; 
		DP("FDT: dev id 0x%08x", fsId) ; 
		if (fsId != SVP_HOST_FS_DEVICE_ID) return KErrHardwareNotAvailable ;

		// we have a disagreement about the base number of the drives: 0 or 1?
		iDriveMap[driveNumber-1] = deviceAddressLin ; 
		offset = fdt_node_offset_by_compatible(fdt, offset, compatible);
		}
	return KErrNone;
	}
	
//
// DSVPHostFsChannel::DoCancel
//
void DSVPHostFsChannel::DoCancel(TInt aReqNo)
	{
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::DoCancel() %x(%d)", aReqNo, aReqNo);
	}

//
// DSVPHostFsChannel::DoRequest
//
TInt DSVPHostFsChannel::DoRequest(TInt aReqNo, TRequestStatus* aStatus, TAny* a1, TAny* a2)
	{
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::DoRequest() %x(%d)", aReqNo, aReqNo);

	TInt err = KErrGeneral;         

	(void)aStatus;

	switch(aReqNo)
		{
	        case RSVPHostFsDriver::EMkDir:
			{
			err = MkDir((TSVPHostFsMkDirInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::ERmDir:
			{
			err = RmDir((TSVPHostFsRmDirInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::EDelete:
			{
			err = Delete((TSVPHostFsDeleteInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::ERename:
			{
			err = Rename((TSVPHostFsRenameInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::EReplace:
			{
			err = Replace((TSVPHostFsReplaceInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::EEntry:
			{
			err = Entry((TSVPHostFsEntryInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::ESetEntry:
			{
			err = SetEntry((TSVPHostFsSetEntryInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::EFileOpen:
			{
			err = FileOpen((TSVPHostFsFileOpenInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::EDirOpen:
			{
			err = DirOpen((TSVPHostFsDirOpenInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::EFileClose:
			{
			err = FileClose((TUint32)a1, (TUint32)a2);
			break;
			}
	        case RSVPHostFsDriver::EFileRead:
			{
			err = FileRead((TSVPHostFsFileReadInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::EFileWrite:
			{
			err = FileWrite((TSVPHostFsFileWriteInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::EFileSetSize:
			{
			err = FileSetSize((TSVPHostFsFileSetSizeInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::EFileFlushAll:
			{
		    err = Flush((TUint32)a1);
			break;
			}
	        case RSVPHostFsDriver::EDirClose:
			{
		        err = DirClose((TUint32)a1, (TUint32)a2);
			break;
			}
	        case RSVPHostFsDriver::EDirRead:
			{
		        err = DirRead((TSVPHostFsDirReadInfo*)a1);
			break;
			}
	        case RSVPHostFsDriver::EGetDeviceID:
			{
		  err = GetID((TUint32)a1, (TUint32*)a2); 
			break;
			}
	        case RSVPHostFsDriver::EGetDriveMap:
			{
		        err = GetDriveMap((TUint32*)a1);
			break;
			}

		default:
			{
			err = KErrGeneral;
			}
		}
	
	if (KErrNone != err)
		{

		if (err == KErrNotSupported)
			{
			err = KErrNone; // trap KErrNotSupported
			}
				
		DP("Error %d from DoRequest %d", err, aReqNo);

		}
	
	return err;
	}

//
// DSVPHostFsChannel::DoControl
//
TInt DSVPHostFsChannel::DoControl(TInt aFunction, TAny* a1, TAny* a2)
	{
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::DoControl()");

	DP("DoControl Function %d", aFunction);

	TInt err = KErrGeneral;         

	/* There should be a good reason to use a control rather than a request. */

	if (KErrNone != err)
		{

		if (err == KErrNotSupported)
			{
			err = KErrNone; // trap KErrNotSupported
			}

		DP("** (SVPHOSTFSDRIVER) Error %d from control function", err);
		}
	
	return err;
	}

void DSVPHostFsChannel::HandleMsg(TMessageBase* aMsg)
	{
	
	TThreadMessage& m = *(TThreadMessage*)aMsg;
	TInt id = m.iValue;

	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::HandleMsg() %x(%d)", id, id);

	if (id == (TInt)ECloseMsg)
		{
		m.Complete(KErrNone, EFalse); 
		return;
		}
	if (id == KMaxTInt)
		{
		// DoCancel
		DoCancel(m.Int0());
		m.Complete(KErrNone, ETrue); 
		return;
		}
	if (id < 0)
		{
		// DoRequest
		TRequestStatus* pStatus = (TRequestStatus*)m.Ptr0();
		TInt r = DoRequest(~id, pStatus, m.Ptr1(), m.Ptr2());
		//		if (r != KErrNone)
		Kern::RequestComplete(iClientThread,pStatus,r);
		m.Complete(KErrNone, ETrue);
		}
	else
		{
		// DoControl
		TInt r = DoControl(id, m.Ptr0(), m.Ptr1());
		m.Complete(r, ETrue);
		}
	}

TInt DSVPHostFsChannel::MkDir(TSVPHostFsMkDirInfo* aInfo)
	{
	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::MkDir()");

	TSVPHostFsMkDirInfo info;
	TInt err = KErrNone;

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, aInfo, (TUint8*)&info, sizeof(TSVPHostFsMkDirInfo)));

	if (!info.iName)
		return KErrArgument;

	TUint16 pathData[KMaxPath];
	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, info.iName, (TUint8*)pathData, info.iLength*2));
	
	TUint32 device = iDriveMap[info.iDrive];
	SVPWriteReg(device, EArg0, Epoc::LinearToPhysical((TUint32)pathData));
	SVPWriteReg(device, EArg1, info.iLength);
	SVPWriteReg(device, EArg2, info.iFlags);
	SVPInvoke(device, RSVPHostFsDriver::EMkDir);


	//return SVPReadReg(device, EResult);
	
	err = SVPReadReg(device, EResult);
	
	if(err == KErrPathNotFound)
		err = KErrNotFound;
	
	return err;


	}

TInt DSVPHostFsChannel::RmDir(TSVPHostFsRmDirInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::RmDir()");

	TSVPHostFsRmDirInfo info;
	TInt err = KErrNone;

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, aInfo, (TUint8*)&info, sizeof(TSVPHostFsRmDirInfo)));

	if (!info.iName)
		return KErrArgument;

	TUint16 pathData[KMaxPath];
	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, info.iName, (TUint8*)pathData, info.iLength*2));

	TUint32 device = iDriveMap[info.iDrive];
	SVPWriteReg(device, EArg0, Epoc::LinearToPhysical((TUint32)pathData));
	SVPWriteReg(device, EArg1, info.iLength);
	SVPInvoke(device, RSVPHostFsDriver::ERmDir);

	return SVPReadReg(device, EResult);

	}

TInt DSVPHostFsChannel::Delete(TSVPHostFsDeleteInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::Delete()");

	TSVPHostFsDeleteInfo info;
	TInt err = KErrNone;

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, aInfo, (TUint8*)&info, sizeof(TSVPHostFsDeleteInfo)));

	if (!info.iName)
		return KErrArgument;

	TUint16 pathData[KMaxPath];
	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, info.iName, (TUint8*)pathData, info.iLength*2));

	TUint32 device = iDriveMap[info.iDrive];
	SVPWriteReg(device, EArg0, Epoc::LinearToPhysical((TUint32)pathData));
	SVPWriteReg(device, EArg1, info.iLength);
	SVPInvoke(device, RSVPHostFsDriver::EDelete);

	return SVPReadReg(device, EResult);

	}

TInt DSVPHostFsChannel::Rename(TSVPHostFsRenameInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::Rename()");

	TSVPHostFsRenameInfo info;
	TInt err = KErrNone;

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, aInfo, (TUint8*)&info, sizeof(TSVPHostFsRenameInfo)));

	if (!info.iOldName)
		return KErrArgument;

	if (!info.iNewName)
		return KErrArgument;

	TUint16 oldPathData[KMaxPath];
	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, info.iOldName, (TUint8*)oldPathData, info.iOldLength*2));

	TUint16 newPathData[KMaxPath];
	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, info.iNewName, (TUint8*)newPathData, info.iNewLength*2));

	TUint32 device = iDriveMap[info.iDrive];
	SVPWriteReg(device, EArg0, Epoc::LinearToPhysical((TUint32)oldPathData));
	SVPWriteReg(device, EArg1, info.iOldLength);
	SVPWriteReg(device, EArg2, Epoc::LinearToPhysical((TUint32)newPathData));
	SVPWriteReg(device, EArg3, info.iNewLength);
	SVPInvoke(device, RSVPHostFsDriver::ERename);

	return SVPReadReg(device, EResult);

	}

TInt DSVPHostFsChannel::Replace(TSVPHostFsReplaceInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::Replace()");

	TSVPHostFsReplaceInfo info;
	TInt err = KErrNone;

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, aInfo, (TUint8*)&info, sizeof(TSVPHostFsReplaceInfo)));

	if (!info.iOldName)
		return KErrArgument;

	if (!info.iNewName)
		return KErrArgument;

	TUint16 oldPathData[KMaxPath];
	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, info.iOldName, (TUint8*)oldPathData, info.iOldLength*2));

	TUint16 newPathData[KMaxPath];
	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, info.iNewName, (TUint8*)newPathData, info.iNewLength*2));

	TUint32 device = iDriveMap[info.iDrive];
	SVPWriteReg(device, EArg0, Epoc::LinearToPhysical((TUint32)oldPathData));
	SVPWriteReg(device, EArg1, info.iOldLength);
	SVPWriteReg(device, EArg2, Epoc::LinearToPhysical((TUint32)newPathData));
	SVPWriteReg(device, EArg3, info.iNewLength);
	SVPInvoke(device, RSVPHostFsDriver::EReplace);

	return SVPReadReg(device, EResult);

	}

TInt DSVPHostFsChannel::Entry(TSVPHostFsEntryInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::Entry()");

	TSVPHostFsEntryInfo info;
	TInt err = KErrNone;

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, aInfo, (TUint8*)&info, sizeof(TSVPHostFsEntryInfo)));

	if (!info.iName)
		return KErrArgument;

	TUint16 pathData[KMaxPath];
	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, info.iName, (TUint8*)pathData, info.iLength*2));

	TUint32 device = iDriveMap[info.iDrive];
	SVPWriteReg(device, EArg0, Epoc::LinearToPhysical((TUint32)pathData));
	SVPWriteReg(device, EArg1, info.iLength);
	SVPInvoke(device, RSVPHostFsDriver::EEntry);


	//RET_IF_ERROR(err, SVPReadReg(device, EResult));
	
	err = SVPReadReg(device, EResult);
	
	if(err!=KErrNone)
		{
		if(err == KErrPathNotFound)
			err = KErrNotFound;
		
		return err;
		}

	

	TUint32 att = SVPReadReg(device, EArg0);
	TUint32 modified = SVPReadReg(device, EArg1);
	TUint32 filesize = SVPReadReg(device, EArg2);
	// TODO: Yuk! Hack alert! Say EWindows for now. But really should probably say EUnknown,
	// since the device won't tell us. Not sure if it can (easily) given remote mounting etc.
	// However this probably delays the problem. On the other hand it is probably best to make
	// the file service guess, since it need only guess once, and cache its guess.
	TUint32 filetimetype = EWindows;

	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iAtt, (TUint8*)&att, sizeof(att)));
	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iModified, (TUint8*)&modified, sizeof(modified)));
	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iSize, (TUint8*)&filesize, sizeof(filesize)));
	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iTimeType, (TUint8*)&filetimetype, sizeof(filetimetype)));
	return KErrNone;
	}

TInt DSVPHostFsChannel::SetEntry(TSVPHostFsSetEntryInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::SetEntry()");
	return KErrNotSupported;
	}

TInt DSVPHostFsChannel::Flush(TUint32 aDrive)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::Flush()");
	TUint32 device = iDriveMap[aDrive];
	SVPInvoke(device, RSVPHostFsDriver::EFileFlushAll);

	return KErrNone;
	}

TInt DSVPHostFsChannel::DirOpen(TSVPHostFsDirOpenInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::DirOpen()");

	TSVPHostFsDirOpenInfo info;
	TInt err = KErrNone;

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, aInfo, (TUint8*)&info, sizeof(TSVPHostFsDirOpenInfo)));

	if (!info.iName)
		return KErrArgument;

	TUint16 pathData[KMaxPath];
	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, info.iName, (TUint8*)pathData, info.iLength*2));

	TUint32 device = iDriveMap[info.iDrive];
	SVPWriteReg(device, EArg0, Epoc::LinearToPhysical((TUint32)pathData));
	SVPWriteReg(device, EArg1, info.iLength);
	SVPInvoke(device, RSVPHostFsDriver::EDirOpen);


	//RET_IF_ERROR(err, SVPReadReg(device, EResult));
	
	err = SVPReadReg(device, EResult);
	
	if(err!=KErrNone)
		{
		
		if(err==KErrPathNotFound)
			err=KErrNotFound;
		
		return err;
		}


	// handle is in arg 0
	TUint32 handle = SVPReadReg(device, EArg0);
	return Kern::ThreadRawWrite(iClientThread, &aInfo->iHandle, (TUint8*)&handle, sizeof(handle));
	}

TInt DSVPHostFsChannel::FileOpen(TSVPHostFsFileOpenInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::FileOpen()");

	TSVPHostFsFileOpenInfo info;
	TInt err = KErrNone;

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, aInfo, (TUint8*)&info, sizeof(TSVPHostFsFileOpenInfo)));

	if (!info.iName)
		return KErrArgument;

	TUint16 pathData[KMaxPath];
	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, info.iName, (TUint8*)pathData, info.iLength*2));

	TUint32 device = iDriveMap[info.iDrive];
	SVPWriteReg(device, EArg0, Epoc::LinearToPhysical((TUint32)pathData));
	SVPWriteReg(device, EArg1, info.iLength);
	SVPWriteReg(device, EArg2, info.iMode);
	SVPWriteReg(device, EArg3, info.iOpen);
	SVPInvoke(device, RSVPHostFsDriver::EFileOpen);


	//RET_IF_ERROR(err, SVPReadReg(device, EResult));
	
	err = SVPReadReg(device, EResult);
		
	if(err!=KErrNone)
		{
		if(err == KErrPathNotFound)
			err = KErrNotFound;
		
		return err;
		}

	

	TUint32 handle = SVPReadReg(device, EArg0);
	TUint32 att = SVPReadReg(device, EArg1);
	TUint32 modified = SVPReadReg(device, EArg2);
	TUint32 size = SVPReadReg(device, EArg3);
	TUint32 timeType = EWindows;

	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iHandle, (TUint8*)&handle, sizeof(handle)));
	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iAtt, (TUint8*)&att, sizeof(att)));
	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iModified, (TUint8*)&modified, sizeof(modified)));
	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iSize, (TUint8*)&size, sizeof(size)));
	return Kern::ThreadRawWrite(iClientThread, &aInfo->iTimeType, (TUint8*)&timeType, sizeof(timeType));

	}

TInt DSVPHostFsChannel::FileClose(TUint32 aDrive, TUint32 aHandle)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::FileClose()");

	TUint32 device = iDriveMap[aDrive];
	SVPWriteReg(device, EArg0, aHandle);
	SVPInvoke(device, RSVPHostFsDriver::EFileClose);
	return SVPReadReg(device, EResult);
	}

TInt DSVPHostFsChannel::FileRead(TSVPHostFsFileReadInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::FileRead()");

	char buf[0x400];
	TSVPHostFsFileReadInfo info;
	TInt err = KErrNone;

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, aInfo, (TUint8*)&info, sizeof(TSVPHostFsFileReadInfo)));

	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::FileRead handle %d length %d pos %d phys_addr 0x%08x info.iBuf 0x%08x",
	   info.iHandle, info.iLength, info.iPos, Epoc::LinearToPhysical((TUint32)buf), info.iBuf);

	TUint32 device = iDriveMap[info.iDrive];
	SVPWriteReg(device, EArg0, info.iHandle);
	SVPWriteReg(device, EArg1, info.iPos);	
	SVPWriteReg(device, EArg2, Epoc::LinearToPhysical((TUint32)buf));
	SVPWriteReg(device, EArg3, info.iLength);	
	SVPInvoke(device, RSVPHostFsDriver::EFileRead);

	RET_IF_ERROR(err, SVPReadReg(device, EResult));

	TUint32 len = SVPReadReg(device, EArg0);

	DP("** (SVPHOSTFSDRIVER)  Read %d bytes", len);

	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iLength, (TUint8*)&len, sizeof(len)));
	return Kern::ThreadRawWrite(iClientThread, (TUint8*)info.iBuf, (TUint8*)&buf, len);
	}

TInt DSVPHostFsChannel::FileWrite(TSVPHostFsFileWriteInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::FileWrite()");

	char buf[0x400];
	TSVPHostFsFileWriteInfo info;
	TInt err = KErrNone;

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, aInfo, (TUint8*)&info, sizeof(TSVPHostFsFileWriteInfo)));

	DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::FileWrite handle %d length %d pos %d phys_addr 0x%08x info.iBuf 0x%08x", 
	   info.iHandle, info.iLength, info.iPos, Epoc::LinearToPhysical((TUint32)buf), info.iBuf);

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, (TUint8 *)info.iBuf, buf, info.iLength));

	TUint32 device = iDriveMap[info.iDrive];
	SVPWriteReg(device, EArg0, info.iHandle);
	SVPWriteReg(device, EArg1, info.iPos);	
	SVPWriteReg(device, EArg2, Epoc::LinearToPhysical((TUint32)buf));
	SVPWriteReg(device, EArg3, info.iLength);	
	SVPInvoke(device, RSVPHostFsDriver::EFileWrite);


	// RET_IF_ERROR(err, SVPReadReg(device, EResult));
	
	err = SVPReadReg(device, EResult);
	
	if(err!=KErrNone)
		{
		if(err == KErrPathNotFound)
			err = KErrNotFound;
		
		return err;
		}

	

	TUint32 len = SVPReadReg(device, EArg0);

	DP("** (SVPHOSTFSDRIVER)  Wrote %d bytes", len);

        return Kern::ThreadRawWrite(iClientThread, &aInfo->iLength, (TUint8*)&len, sizeof(len));
	}

TInt DSVPHostFsChannel::FileSetSize(TSVPHostFsFileSetSizeInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::FileSetSize()");
  
	TSVPHostFsFileSetSizeInfo info;
	TInt err = KErrNone;

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, aInfo, (TUint8*)&info, sizeof(TSVPHostFsFileSetSizeInfo)));

	TUint32 device = iDriveMap[info.iDrive];
	SVPWriteReg(device, EArg0, info.iHandle);
	SVPWriteReg(device, EArg1, info.iLength);
	SVPInvoke(device, RSVPHostFsDriver::EFileSetSize);

	TUint32 res = SVPReadReg(device, EResult);

	RET_IF_ERROR(err, res);

	return res;
	}

TInt DSVPHostFsChannel::FileSetEntry(TSVPHostFsSetEntryInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::FileSetEntry()");


	return KErrNone;

	}

TInt DSVPHostFsChannel::DirClose(TUint32 aDrive, TUint32 aHandle)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::DirClose()");

	TUint32 device = iDriveMap[aDrive];
	SVPWriteReg(device, EArg0, aHandle);
	SVPInvoke(device, RSVPHostFsDriver::EDirClose);
	return SVPReadReg(device, EResult);
	}

TInt DSVPHostFsChannel::DirRead(TSVPHostFsDirReadInfo* aInfo)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::DirRead()");

	TUint16 name[KMaxPath];
	TSVPHostFsDirReadInfo info;
	TInt err = KErrNone;

	RET_IF_ERROR(err, Kern::ThreadRawRead(iClientThread, aInfo, (TUint8*)&info, sizeof(TSVPHostFsDirReadInfo)));


	TUint32 device = iDriveMap[info.iDrive];
	SVPWriteReg(device, EArg0, info.iHandle);
	SVPWriteReg(device, EArg1, Epoc::LinearToPhysical((TUint32)name));
	SVPWriteReg(device, EArg2, KMaxPath);
	SVPInvoke(device, RSVPHostFsDriver::EDirRead);

	RET_IF_ERROR(err, SVPReadReg(device, EResult));

	TUint32 att = SVPReadReg(device, EArg0);
	TUint32 modified = SVPReadReg(device, EArg1);
	TUint32 filesize = SVPReadReg(device, EArg2);
	TUint32 namesize = SVPReadReg(device, EArg3);
	// TODO: Yuk! Hack alert! Say EWindows for now. But really should probably say EUnknown,
	// since the device won't tell us. Not sure if it can (easily) given remote mounting etc.
	// However this probably delays the problem. On the other hand it is probably best to make
	// the file service guess, since it need only guess once, and cache its guess.
	TUint32 filetimetype = EWindows;

	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iAtt, (TUint8*)&att, sizeof(att)));
	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iModified, (TUint8*)&modified, sizeof(modified)));
	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iSize, (TUint8*)&filesize, sizeof(filesize)));
	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iTimeType, (TUint8*)&filetimetype, sizeof(filetimetype)));
	RET_IF_ERROR(err, Kern::ThreadRawWrite(iClientThread, &aInfo->iLength, (TUint8*)&namesize, sizeof(namesize)));
	return Kern::ThreadRawWrite(iClientThread, &aInfo->iName, (TUint8*)&name, namesize * sizeof(TUint16));
	}

TInt DSVPHostFsChannel::GetID(TUint32 aDrive, TUint32 * aId)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::GetID");
	TUint32 device = iDriveMap[aDrive];
        TUint32 id = SVPReadReg(device, EDeviceID);
	return Kern::ThreadRawWrite(iClientThread, aId, &id, sizeof(TUint32));
	}

TInt DSVPHostFsChannel::GetDriveMap(TAny * aMap)
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsChannel::GetDriveMap");
	return Kern::ThreadRawWrite(iClientThread, aMap, iDriveMap, sizeof(iDriveMap));
	}

DECLARE_EXTENSION_LDD()
	{
        DP("** (SVPHOSTFSDRIVER) DSVPHostFsDriverFactory created");
        return new DSVPHostFsDriverFactory;
	}

DECLARE_STANDARD_EXTENSION()
	{
        DP("** (SVPHOSTFSDRIVER) SVPHostFs extension entry point");

	DP("** (SVPHOSTFSDRIVER) Creating LocDrv device");
	TInt r;
	DSVPHostFsDriverFactory* device = new DSVPHostFsDriverFactory;
	if (device==NULL)
		r=KErrNoMemory;
	else
		r=Kern::InstallLogicalDevice(device);

	DP("** (SVPHOSTFSDRIVER) Installing LocDrv device in kernel returned %d",r);

	return r;
	}
