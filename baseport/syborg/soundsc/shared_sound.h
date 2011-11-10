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

#ifndef __SYBORGSHARED_SOUND_H__
#define __SYBORGSHARED_SOUND_H__

#include <soundsc.h>

#ifndef DISABLE_SYBORG_SOUND_DEBUG
#define SYBORG_SOUND_DEBUG(x...) __KTRACE_OPT(KSOUND1, Kern::Printf(x))
#else
#define SYBORG_SOUND_DEBUG(x...)
#endif

#undef ASSERT
#define ASSERT(x) (x) || (Kern::Printf("Sound.pdd: ASSERTION FAILED: "#x),0);

#include "virtio_audio.h"
#include "virtio.h"
#include "virtio_audio_defs.h"

/// @brief defines the maximum size for a single audio data transfer
static const TInt KMaxTransferLength = 256 * 1024;

namespace VirtIo
{
class DIoHandler;
}

class DDriverSyborgSoundScPddFactory;


class DDriverSyborgSoundScPdd : public DSoundScPdd, VirtIo::MIoCallback
	{
public:

	DDriverSyborgSoundScPdd(DDriverSyborgSoundScPddFactory* aPhysicalDevice, 
		TInt aUnitType, VirtIo::DIoHandler *aIoHandler, TUint aDataQueueId);
	~DDriverSyborgSoundScPdd();
	TInt DoCreate();
	void GetChunkCreateInfo(TChunkCreateInfo& aChunkCreateInfo);
	void Caps(TDes8& aCapsBuf) const;
	TInt MaxTransferLen() const;
	TInt SetConfig(const TDesC8& aConfigBuf);
	TInt SetVolume(TInt aVolume);
	TInt StartTransfer();
	TInt TransferData(TUint aTransferID, TLinAddr aLinAddr, TPhysAddr aPhysAddr, TInt aNumBytes);
	void StopTransfer();
	TInt PauseTransfer();
	TInt ResumeTransfer();
	TInt PowerUp();
	void PowerDown();
	TInt CustomConfig(TInt aFunction, TAny* aParam);
	void SetCaps();
	TDfcQue* DfcQ();
	TDfcQue* DfcQ( TInt aUnit );
	
	TInt CalculateBufferTime(TInt aNumBytes);
private:

	// implementation of VirtIo::MIoCallback
	virtual TBool VirtIoCallback( VirtIo::MIoHandler& aVirtIoHandler, VirtIo::MQueue& aQueue, 
		VirtIo::Token aToken, TUint aBytesTransferred );

public:
	
	DDriverSyborgSoundScPddFactory*	iPhysicalDevice;
	
	TInt						iUnitType; //Play or Record

	VirtIo::MIoHandler* iIoHandler;
	
	TUint iDataQueueId;
	
private:

	TSoundFormatsSupportedV02	iCaps;
	
	TCurrentSoundFormatV02		iConfig;

	VirtIo::Audio::DControl* iAudioControl;
	
	};

#endif 
