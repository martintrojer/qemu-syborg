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
//	CSVPHostDirCB							//
//////////////////////////////////////////////////////////////////////////	

CSVPHostDirCB::CSVPHostDirCB(/*CSessionFs* aSession*/)
//
// Constructor
//
        : CDirCB(/*aSession*/),iEntry()
	{
	__DECLARE_NAME(_S("CSVPHostDirCB"));
	}

CSVPHostDirCB::~CSVPHostDirCB()
//
// Destructor
//
	{
	DP(_L("** (SVPHOSTDIR) CSVPHostDirCB::~CSVPHostDirCB"));

	if ((TInt)iHandle == KErrBadHandle) return;

	SVP_HOST_FS_DEVICE().DirClose(Drive().DriveNumber(), iHandle);
	}

TBool CSVPHostDirCB::MatchUid()
//
// Match the uid ?
//
	{
	if (iUidType[0]!=TUid::Null() || iUidType[1]!=TUid::Null() || iUidType[2]!=TUid::Null())
		return(ETrue);
	return(EFalse);
	}

void fileTimeToTime(TUint32 t,TTime& aTime, TFileTimeType aType)
//
//	Convert a time (in seconds from the epoc) into a TTIME
//
	{
	TInt64 at =((TInt64)t) * 1000000;
	TTime time = TTime(at);
	if (aType == EWindows) 
		{
		time += TTimeIntervalYears(1970);
		time += TTimeIntervalDays(1);
		}
	aTime=time;
	}

LOCAL_C TBool CompareUid(const TUidType& aUidTrg, const TUidType& aUidSuitor)
//
// Compare the suitor to the target pattern
//
	{
	
	if (aUidTrg[0]!=TUid::Null() && aUidTrg[0]!=aUidSuitor[0])
		return(EFalse);
	if (aUidTrg[1]!=TUid::Null() && aUidTrg[1]!=aUidSuitor[1])
		return(EFalse);
	if (aUidTrg[2]!=TUid::Null() && aUidTrg[2]!=aUidSuitor[2])
		return(EFalse);
	return(ETrue);
	}


void CSVPHostDirCB::ReadL(TEntry& anEntry)
//
//	Read the next entry from the directory
//
	{
	DP(_L("** (SVPHOSTDIR) CSVPHostDirCB::ReadL"));

	TUint driveNumber = Drive().DriveNumber() ; 
	FOREVER
		{
		if (!iPending)
			{
			TSVPHostFsDirReadInfo info(driveNumber, iHandle);
			User::LeaveIfError(SVP_HOST_FS_DEVICE().DirRead(info));

			TPtr name((TUint16 *)&info.iName[0], info.iLength, KMaxPath);

			if (name==_L(".") || name==_L(".."))
			  continue;

			iEntry.iName = name;
			iEntry.iAtt=info.iAtt&KEntryAttMaskSupported;
			iEntry.iSize=info.iSize;
			fileTimeToTime(info.iModified,iEntry.iModified, info.iTimeType);
			DP(_L("%S %d %x %x"), &name, iEntry.iAtt, iEntry.iSize, iEntry.iModified);
			}
		iPending=EFalse;
		anEntry=iEntry;


		if (Mount().MatchEntryAtt(anEntry.iAtt&KEntryAttMaskSupported,iAtt) == EFalse)
			continue;

		if (iFullName.NameAndExt()==_L("*.*") || anEntry.iName.MatchF(iFullName.NameAndExt())!=KErrNotFound) 

   			{
			if (MatchUid())
				{
				TParse fileName;
				TBuf<KMaxFileName> path=iFullName.Path();
				fileName.Set(anEntry.iName,&path,NULL);
				(*(CSVPHostMountCB*)&Mount()).ReadUidL(fileName.FullName(),anEntry);
				if (CompareUid(iUidType,anEntry.iType))
					break;
				}
			else
				break;
			}
		break;
		}

	if ((iAtt&KEntryAttAllowUid)==0 || anEntry.iAtt&KEntryAttDir || MatchUid())
		return;
	TParse fileName;
	TBuf<KMaxFileName> path=iFullName.Path();
	fileName.Set(anEntry.iName,&path,NULL);
	(*(CSVPHostMountCB*)&Mount()).ReadUidL(fileName.FullName(),anEntry);

	return;
	}


//////////////////////////////////////////////////////////////////////////
// CSVPHostFormatCB							//
//////////////////////////////////////////////////////////////////////////	

CSVPHostFormatCB::CSVPHostFormatCB(/*CSessionFs* aSession*/)//???JCS
//
// Constructor
//
	: CFormatCB(/*aSession*/)
	{
	__DECLARE_NAME(_S("CSVPHostFormatCB"));
	}

CSVPHostFormatCB::~CSVPHostFormatCB()
//
// Destructor
//
	{}

void CSVPHostFormatCB::DoFormatStepL()
//
// Do Formatting
//
	{
	iCurrentStep=0;
	User::Leave(KErrNotSupported);
	}

