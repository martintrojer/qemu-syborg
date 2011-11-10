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

#ifndef __SYBORGVARIANT_SOUND_H__
#define __SYBORGVARIANT_SOUND_H__

#include "shared_sound.h"

static const TUint KAudioDfcQueuePriority = 28;

class DDriverSyborgSoundScPddFactory : public DPhysicalDevice
	{
public:

	DDriverSyborgSoundScPddFactory();
	~DDriverSyborgSoundScPddFactory();
	TInt Install();
	void GetCaps(TDes8 &aDes) const;
	TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
	TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
	
	VirtIo::MIoHandler* IoHandler();

public:

	/** The DFC queue to be used by both the LDD and the PDD to serialise access to the PDD */
	TDynamicDfcQue*		iDfcQ;
	
	VirtIo::DIoHandler *iIoHandler;
	
	};

#endif 
