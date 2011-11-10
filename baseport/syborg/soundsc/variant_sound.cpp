/*
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
* Accenture Ltd
*
* Description: This file is a part of sound driver for Syborg adaptation.
*
*/

#include "variant_sound.h"
#include "virtio_iohandler.h"
#include "../specific/syborg.h"

_LIT(KSoundScPddName, "SoundSc.Syborg");

DECLARE_STANDARD_PDD()
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPddFactory created\n");
	return new DDriverSyborgSoundScPddFactory;
	}


DDriverSyborgSoundScPddFactory::DDriverSyborgSoundScPddFactory()
	{
	iUnitsMask = ((1 << KSoundScTxUnit0) | (1 << KSoundScRxUnit0));

	iVersion = RSoundSc::VersionRequired();
	}


TInt DDriverSyborgSoundScPddFactory::Install()
	{
	_LIT(KAudioDFC, "AUDIO DFC");
	
	// LDD driver is going to use the same queue.
	TInt r = Kern::DynamicDfcQCreate(iDfcQ, KAudioDfcQueuePriority, KAudioDFC);

	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPddFactory::PDD install");

	if(r!=KErrNone)
		{
		SYBORG_SOUND_DEBUG("Creating audio DFC failed %d",r);
		return r;
		}
	// All PDD factories must have a unique name
	r = SetName(&KSoundScPddName);
	if (r!=KErrNone)
		{
		SYBORG_SOUND_DEBUG("Setting name %x",r);
		return r;
		}
	iIoHandler = new VirtIo::DIoHandler(	
		(TAny*)KHwSVPAudioDevice, 
		EIntAudio0,
		iDfcQ );
	
	if (iIoHandler == NULL)
		{
		iDfcQ->Destroy();
		return KErrNoMemory;
		}
	
	SYBORG_SOUND_DEBUG("Constructing IoHandler");
	
	r = iIoHandler->Construct();
	
	if ( r != KErrNone)
		{
		iDfcQ->Destroy();		
		delete iIoHandler;
		}
		
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPddFactory::PDD installed");
	
	return r;
	}
	
DDriverSyborgSoundScPddFactory::~DDriverSyborgSoundScPddFactory()
	{
	if (iIoHandler)
		{
		delete iIoHandler;
		iIoHandler = NULL;
		}
	if (iDfcQ)
		iDfcQ->Destroy();
	}
	

void DDriverSyborgSoundScPddFactory::GetCaps(TDes8& /*aDes*/) const
	{
	}


TInt DDriverSyborgSoundScPddFactory::Validate(TInt aUnit, const TDesC8* /*aInfo*/, const TVersion& aVer)
	{
	// Check that the version requested is less than or equal to the version of this PDD
	if (!Kern::QueryVersionSupported(RSoundSc::VersionRequired(), aVer))
		{
		SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPddFactory::Validate KErrNotSup1");
		return KErrNotSupported;
		}

	// Check the unit number specifies either playback or recording
	if ((aUnit != KSoundScTxUnit0) && (aUnit != KSoundScRxUnit0))
		{
		SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPddFactory::Validate KErrNotSup2");
		return KErrNotSupported;
		}

	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPddFactory::Validate KErrNone");
	return KErrNone;
	}

TInt DDriverSyborgSoundScPddFactory::Create(DBase*& aChannel, TInt aUnit, const TDesC8* /*aInfo*/, const TVersion& /*aVer*/)
	{

	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPddFactory::PDD create aUnit %d TxUnitId %d", aUnit, KSoundScTxUnit0);

	// Assume failure
	TInt r = KErrNoMemory;
	aChannel = NULL;

				
	DDriverSyborgSoundScPdd* pTxD = new DDriverSyborgSoundScPdd( this, aUnit, 
		iIoHandler, aUnit == KSoundScTxUnit0?1:2 );

	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPddFactory::TxPdd %x", pTxD);
		
	if (pTxD)
		{
		SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPddFactory::TxPdd2 %x", pTxD);
			
		r = pTxD->DoCreate();
			
		SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPddFactory::Create ret %d", r);
			
		}
	
	// If everything succeeded, save a pointer to the PDD.  This should only be done if DoCreate() succeeded,
	// as some LDDs have been known to access this pointer even if Create() returns an error!
	if (r == KErrNone)
		{
		aChannel = pTxD;
		SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPddFactory::TxPdd set AChannel %x", aChannel);
		}
	else
		{
		delete pTxD;
		}

	return r;
	}

	
VirtIo::MIoHandler* DDriverSyborgSoundScPddFactory::IoHandler()
	{
	return iIoHandler;
	}
