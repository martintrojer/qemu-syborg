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

#include <ethernet.h>
#include <e32hal.h>
#include <system.h>
#include "ethernet_device.h"

#pragma diag_suppress 1441

_LIT(KEthPddName, "Ethernet.Syborg");

// needs ldd version..
const TInt KMinimumLddMajorVersion	= 1;
const TInt KMinimumLddMinorVersion	= 0;
const TInt KMinimumLddBuild			= 122;

//
// Class to identify the driver as PDD
//
class DDriverEthernet : public DPhysicalDevice
{
public:
	DDriverEthernet();
	
	//
	// Functions that we must implement as we are inheriting from abstract base class
	//
	virtual TInt Install();
	virtual void GetCaps(TDes8 &aDes) const;
	virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
	virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
};

//////////////////////////////////////////
// Implementation of DDriverEthernet class
//////////////////////////////////////////

DDriverEthernet::DDriverEthernet()
{
	DP("** (PDD) DDriverEthernet::DDriverEthernet()");
	__KTRACE_OPT(KHARDWARE, Kern::Printf("DDriverEthernet::DDriverEthernet()"));
	
	iUnitsMask=0x1;	
	iVersion=TVersion(KEthernetMajorVersionNumber,
					  KEthernetMinorVersionNumber,
					  KEthernetBuildVersionNumber);
}

TInt DDriverEthernet::Install()
{
	DP("** (PDD) DDriverEthernet::Install()");
	__KTRACE_OPT(KHARDWARE, Kern::Printf("DDriverEthernet::Install()"));
	
	return SetName(&KEthPddName);
}

void DDriverEthernet::GetCaps(TDes8 &aDes) const
{
	DP("** (PDD) DDriverEthernet::GetCaps");
	__KTRACE_OPT(KHARDWARE, Kern::Printf("DDriverEthernet::GetCaps"));
	
	TEthernetCaps capsBuf;

	aDes.FillZ(aDes.MaxLength());
	aDes=capsBuf.Left(Min(capsBuf.Length(),aDes.MaxLength()));
}

TInt DDriverEthernet::Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer)
{
	DP("** (PDD) DDriverEthernet::Create");
	__KTRACE_OPT(KHARDWARE, Kern::Printf("DDriverEthernet::Create"));
	
    TInt r = KErrNoMemory;

	EthernetDevice	*VirtioEthernet = new EthernetDevice;
	if(VirtioEthernet)
		{
		DP("** (PDD) DDriverEthernet:: EthernetDevice created successfully");
		r = VirtioEthernet->DoCreate(aUnit, anInfo);
		}
	aChannel = VirtioEthernet;
       return r;
}

TInt DDriverEthernet::Validate(TInt aUnit, const TDesC8* anInfo, const TVersion& aVer)
{
	DP("** (PDD) DDriverEthernet::Validate");
	__KTRACE_OPT(KHARDWARE, Kern::Printf("DDriverEthernet::Validate"));
	
	if((!Kern::QueryVersionSupported(iVersion,aVer)) || 
		(!Kern::QueryVersionSupported(aVer,TVersion(KMinimumLddMajorVersion,
													KMinimumLddMinorVersion,
													KMinimumLddBuild))))
	{
		return KErrNotSupported;
	}
	if(aUnit != 0)
	{
		return KErrNotSupported;
	}
	return KErrNone;
}

DECLARE_STANDARD_PDD()
{
	DP("** (DPhysicalDevice) Ethernet PDD Factory created");
	return new DDriverEthernet;
}
