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
//	CSVPHostFileCB							//
//////////////////////////////////////////////////////////////////////////	

CSVPHostFileCB::CSVPHostFileCB()
//
// Constructor
//
	{
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::CSVPHostFileCB()"));

	__DECLARE_NAME(_S("CSVPHostFileCB"));
	}

CSVPHostFileCB::~CSVPHostFileCB()
//
// Destructor
//
	{
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::~CSVPHostFileCB()"));
	if ((TInt)iHandle == KErrBadHandle) return;
	
	SVP_HOST_FS_DEVICE().FileClose(Drive().DriveNumber(), iHandle);
	}

TBool CSVPHostFileCB::IsRomDrive() const
//
// Returns ETrue if the drive number == EDriveZ
//
	{
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::IsRomDrive()"));
	return(EFalse);
	}


void CSVPHostFileCB::CheckPos(TInt /*aPos*/)
	{
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::CheckPos()"));
	}

void CSVPHostFileCB::ReadL(TInt aPos,TInt& aLength ,const TAny* aDes , const RMessagePtr2& aMessage)
	{
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::ReadL(%d,%d)"), aPos, aLength);

	TInt n = 0;
	TInt pos=aPos;
	TInt len=aLength;
	TBuf8<0x400> buf;

	if (aMessage.Handle() == KLocalMessageHandle)
		((TPtr8* )aDes)->SetLength(0);

	TUint driveNumber = Drive().DriveNumber() ; 
	while (len)
		{
		TInt s=Min(len,buf.MaxLength());
		TSVPHostFsFileReadInfo info(driveNumber, iHandle,s,pos+n,(char*)buf.Ptr());

		DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::ReadL pos %d s %d p 0x%08x"), pos+n, s, buf.Ptr());

		User::LeaveIfError(SVP_HOST_FS_DEVICE().FileRead(info));

		DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::ReadL read %d bytes"), info.iLength);

		buf.SetLength(info.iLength);

		DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::ReadL set buf length"));

		if (aMessage.Handle() == KLocalMessageHandle)
		        ((TPtr8* )aDes)->Append(buf);
		else
		        aMessage.WriteL(0,buf,n);
		
		n+=info.iLength;

		if (((TInt)info.iLength)<s)
		        break;
		len-=info.iLength;
		}
	aLength=n;
	iCurrentPos=n+aPos;
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::ReadL aLength %d iCurrentPos %d"), aLength, iCurrentPos);
	}



void CSVPHostFileCB::WriteL(TInt aPos, TInt& aLength, const TAny* aDes, const RMessagePtr2& aMessage)
	{
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::WriteL(%d,%d)"), aPos, aLength);

	TInt n=0;
	TInt pos=aPos;
	TInt len=aLength;
	TBuf8<0x400> buf;
	TUint driveNumber = Drive().DriveNumber() ; 
	while (len)
		{
		TInt s=Min(len,buf.MaxLength());

		if (aMessage.Handle() == KLocalMessageHandle)
			buf.Copy( ((TPtr8* )aDes)->MidTPtr(n, s) );
		else
			aMessage.ReadL(0,buf,n);

		TSVPHostFsFileWriteInfo info(driveNumber, iHandle,s,pos+n,(char*)buf.Ptr());

		DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::WriteL pos %d s %d p 0x%08x"), pos+n, s, buf.Ptr());

		User::LeaveIfError(SVP_HOST_FS_DEVICE().FileWrite(info));

		DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::WriteL wrote %d bytes"), info.iLength);

		if (((TInt)info.iLength)<s) 
		  {
		    DP(_L("** (SVPHOSTFIL) !!! CSVPHostFileCB::WriteL failed to write s %d bytes"), s);
			User::Leave(KErrCorrupt);
		  }

		len-=info.iLength;
		n+=info.iLength;
		}
	aLength=n;
	iCurrentPos=n+aPos;
	}

TInt CSVPHostFileCB::Address(TInt& /*aPos*/) const
//
//	If ROM file, do a memory map and return the address
//	Dummy implementation
//
	{
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::Address()"));
	return(KErrNone);
	}

void CSVPHostFileCB::SetSizeL(TInt aSize)
//
//	Set the file size
//
	{
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::SetSizeL()"));
	
	TSVPHostFsFileSetSizeInfo info(Drive().DriveNumber(), iHandle,aSize);

	User::LeaveIfError(SVP_HOST_FS_DEVICE().FileSetSize(info));

	}


void CSVPHostFileCB::SetEntryL(const TTime& aTime,TUint aSetAttMask,TUint aClearAttMask)
// TODO: Not tested
	{
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::SetEntryL()"));
	//User::Leave(KErrNotSupported);
	}

void CSVPHostFileCB::FlushAllL()
//
// Commit any buffered date to the media.
//
	{
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::FlushAll()"));

	User::LeaveIfError(SVP_HOST_FS_DEVICE().FlushAll(Drive().DriveNumber()));
	}


void CSVPHostFileCB::FlushDataL()
//
//	Commit any buffered date to the media
//	Dummy implementation of a pure virtual function
//
	{
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::FlushDataL()"));
	  
	User::LeaveIfError(SVP_HOST_FS_DEVICE().FlushData(Drive().DriveNumber()));
	}

void CSVPHostFileCB::RenameL(const TDesC& /*aNewName*/)
//
//	Rename the file while open
//	Dummy implementation of a pure virtual function
//
	{
	DP(_L("** (SVPHOSTFIL) CSVPHostFileCB::RenameL()"));

	}
