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

#include "shared_sound.h"
#include "variant_sound.h"
#include "../specific/syborg.h"

#include "virtio.h"
#include "virtio_audio.h"
#include "virtio_iohandler.h"

using namespace VirtIo;

static TInt GetSampleRate( TSoundRate aRate)
	{
	switch(aRate)
		{
		case ESoundRate7350Hz: 	return 7350;
		case ESoundRate8000Hz: 	return 8000;
		case ESoundRate8820Hz: 	return 8820;
		case ESoundRate9600Hz: 	return 9600;
		case ESoundRate11025Hz: return 11025;
		case ESoundRate12000Hz: return 12000;
		case ESoundRate14700Hz:	return 14700;
		case ESoundRate16000Hz: return 16000;
		case ESoundRate22050Hz: return 22050;
		case ESoundRate24000Hz: return 24000;
		case ESoundRate29400Hz: return 29400;
		case ESoundRate32000Hz: return 32000;
		case ESoundRate44100Hz: return 44100;
		case ESoundRate48000Hz: return 48000;
		}
	return KErrNotFound;
	}

static TInt GetSoundEncoding( TSoundEncoding aV )
	{
	switch (aV)
		{
		case ESoundEncoding8BitPCM: return Audio::EFormatS8;
		case ESoundEncoding16BitPCM: return Audio::EFormatS16;
		case ESoundEncoding24BitPCM: break; // not supported
		}
	return -KErrNotFound;
	}
static TInt GetChannels( TInt aV )
	{
	switch (aV)
		{
		case KSoundMonoChannel: return 1;
		case KSoundStereoChannel: return 2;
		}
	return KErrNotFound;
	}

DDriverSyborgSoundScPdd::DDriverSyborgSoundScPdd(DDriverSyborgSoundScPddFactory* aPhysicalDevice, 
	TInt aUnitType, VirtIo::DIoHandler* aIoHandler, TUint aDataQueueId )
	: iPhysicalDevice(aPhysicalDevice),  iUnitType(aUnitType), iIoHandler( aIoHandler ), iDataQueueId( aDataQueueId )
	{
	}

DDriverSyborgSoundScPdd::~DDriverSyborgSoundScPdd()
	{
	SYBORG_SOUND_DEBUG("~DDriverSyborgSoundScPdd()");
	iIoHandler->UnregisterClient( this );
	delete iAudioControl;
	SYBORG_SOUND_DEBUG("~DDriverSyborgSoundScPdd() - done");
	}

TInt DDriverSyborgSoundScPdd::DoCreate()
	{
	SetCaps();
	
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::DoCreate TxPdd");

	SYBORG_SOUND_DEBUG("Registering with IOHandler %x", iIoHandler );
	iIoHandler->RegisterClient( this );
	SYBORG_SOUND_DEBUG("Registered with IoHandler... Done %x", iIoHandler);
	
	iAudioControl = new Audio::DControl( *iIoHandler, iDataQueueId );
	iAudioControl->Construct();
	Audio::StreamDirection direction = static_cast<Audio::StreamDirection>(
		(iUnitType == KSoundScRxUnit0)?Audio::EDirectionRecord
		:(iUnitType == KSoundScTxUnit0)?Audio::EDirectionPlayback:-1 ); 
        
        iAudioControl->Setup( direction, 2, Audio::EFormatS16, 48000 );
		
	return KErrNone;
	}

void DDriverSyborgSoundScPdd::GetChunkCreateInfo(TChunkCreateInfo& aChunkCreateInfo)
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::GetChunkCreateInfo TxPdd");
	
	aChunkCreateInfo.iType = TChunkCreateInfo::ESharedKernelMultiple;
	aChunkCreateInfo.iMapAttr = EMapAttrFullyBlocking; 	// No caching
	aChunkCreateInfo.iOwnsMemory = ETrue; 				// Using RAM pages
	aChunkCreateInfo.iDestroyedDfc = NULL; 				// No chunk destroy DFC
	}

void DDriverSyborgSoundScPdd::Caps(TDes8& aCapsBuf) const
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::Caps TxPdd");
	
	// Fill the structure with zeros in case it is a newer version than we know about
	aCapsBuf.FillZ(aCapsBuf.MaxLength());

	// And copy the capabilities into the packaged structure
	TPtrC8 ptr((const TUint8*) &iCaps, sizeof(iCaps));
	aCapsBuf = ptr.Left(Min(ptr.Length(), aCapsBuf.MaxLength()));
	}

TInt DDriverSyborgSoundScPdd::SetConfig(const TDesC8& aConfigBuf)
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::SetConfig TxPdd");
	
	// Read the new configuration from the LDD
	TCurrentSoundFormatV02 config;
	TPtr8 ptr((TUint8*) &config, sizeof(config));
	Kern::InfoCopy(ptr, aConfigBuf);
	
	TInt channels = GetChannels(config.iChannels);
	Audio::FormatId encoding = static_cast<Audio::FormatId>( GetSoundEncoding(config.iEncoding) );
	TInt freq = GetSampleRate(config.iRate);
	Audio::StreamDirection direction = static_cast<Audio::StreamDirection>(
		(iUnitType == KSoundScRxUnit0)?Audio::EDirectionRecord
		:(iUnitType == KSoundScTxUnit0)?Audio::EDirectionPlayback:-1 );

	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::SetConfig c %x, e %x, f %x, d %x",
		channels, encoding, freq, direction );
	
	if ( (channels < 0 )
		|| ( encoding < 0 )
		|| ( freq < 0 ) 
		|| ( direction < 0 )
		)
		{
		SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::SetConfig failed");		
		return KErrArgument;
		}
	
	TInt st = iAudioControl->Setup( direction, channels, encoding, freq );
	if (st !=KErrNone)
		{
		SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::SetConfig failed %d", st);				
		return st;
		}
	
	iConfig = config;
	
	return KErrNone;
	}


TInt DDriverSyborgSoundScPdd::SetVolume(TInt aVolume)
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::Setvolume TxPdd");
	
	return KErrNone;
	}


TInt DDriverSyborgSoundScPdd::StartTransfer()
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::starttransfer TxPdd S");
	iAudioControl->SendCommand( Audio::DControl::ERun );
	return KErrNone;
	}
	
	
TInt DDriverSyborgSoundScPdd::CalculateBufferTime(TInt aNumBytes)
	{
	TUint samplerate=GetSampleRate( iConfig.iRate );

	// integer division by number of channels
	aNumBytes /= iConfig.iChannels;

	// integer division by bytes per sample
	switch(iConfig.iEncoding)
		{
		case ESoundEncoding8BitPCM: break;
		case ESoundEncoding16BitPCM: aNumBytes /= 2; break;
		case ESoundEncoding24BitPCM: aNumBytes /= 3; break;
		}

	return (aNumBytes * 1000) / samplerate; //return time in milliseconds
	}

TInt DDriverSyborgSoundScPdd::TransferData(TUint aTransferID, TLinAddr aLinAddr, TPhysAddr aPhysAddr, TInt aNumBytes)
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::TransferData unit %x aTId=%x, linAddr=%x,phAddr=%x,len=%x", 
		iUnitType, aTransferID, aLinAddr, aPhysAddr, aNumBytes);
	
	iAudioControl->SendDataBuffer( 
		reinterpret_cast<TAny*>( aLinAddr ), aNumBytes, reinterpret_cast<Token>( aTransferID ) );

	return KErrNone;
	}

void DDriverSyborgSoundScPdd::StopTransfer()
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::stoptransfer TxPdd");
	
	iAudioControl->SendCommand( Audio::DControl::EStop );
	}


TInt DDriverSyborgSoundScPdd::PauseTransfer()
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::pausetransfer TxPdd");
	iAudioControl->SendCommand( Audio::DControl::EPause );
	return KErrNone;
	}


TInt DDriverSyborgSoundScPdd::ResumeTransfer()
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::resumetransfer TxPdd");
	iAudioControl->SendCommand( Audio::DControl::EResume );
	return KErrNone;
	}

TInt DDriverSyborgSoundScPdd::PowerUp()
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::PowerUp TxPdd");
	return KErrNone;
	}

void DDriverSyborgSoundScPdd::PowerDown()
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::Powerdown TxPdd");
	}

TInt DDriverSyborgSoundScPdd::CustomConfig(TInt /*aFunction*/,TAny* /*aParam*/)
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::CustomConfig TxPdd");
	return KErrNotSupported;
	}

TBool DDriverSyborgSoundScPdd::VirtIoCallback( MIoHandler& aVirtIoHandler, MQueue& aQueue, 
	Token aToken, TUint aBytesTransferred )
	{
	if ( &aQueue != &iAudioControl->DataQueue() )
		{ return ETrue; }
			
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::VirtIoCallback t%x, s%x", aToken, aBytesTransferred);
	
	if ( iCaps.iDirection == ESoundDirPlayback )
		{
		Ldd()->PlayCallback( (TUint) aToken, KErrNone, aBytesTransferred );
		}
	else
		{
		Ldd()->RecordCallback( (TUint) aToken, KErrNone, aBytesTransferred );
		}
		
	return EFalse; // cannot process any more buffers in this go due to a bug in LDD?
	}

TDfcQue*DDriverSyborgSoundScPdd::DfcQ()
	{
	return iPhysicalDevice->iDfcQ;
	}
	
TDfcQue*DDriverSyborgSoundScPdd::DfcQ( TInt /* aUinit */ )
	{
	return iPhysicalDevice->iDfcQ;
	}


TInt DDriverSyborgSoundScPdd::MaxTransferLen() const
	{
	return KMaxTransferLength; 
	}


void DDriverSyborgSoundScPdd::SetCaps()
	{
	SYBORG_SOUND_DEBUG("DDriverSyborgSoundScPdd::SetCaps TxPdd");
	
	if(iUnitType == KSoundScTxUnit0)
		{
		// The data transfer direction for this unit is play
		iCaps.iDirection = ESoundDirPlayback;
		}
	else if(iUnitType == KSoundScRxUnit0)
		{
		// The data transfer direction for this unit is play
		iCaps.iDirection = ESoundDirRecord;
		}
	
	// This unit supports both mono and stereo
	iCaps.iChannels = (KSoundMonoChannel | KSoundStereoChannel);

	// This unit supports only some of the sample rates offered by Symbian OS
	iCaps.iRates = (KSoundRate8000Hz | KSoundRate11025Hz | KSoundRate12000Hz | KSoundRate16000Hz |
					KSoundRate22050Hz | KSoundRate24000Hz | KSoundRate32000Hz | KSoundRate44100Hz |
					KSoundRate48000Hz);

	// This unit only supports 16bit PCM encoding
	iCaps.iEncodings = KSoundEncoding16BitPCM;

	// This unit only supports interleaved data format when playing stereo;  that is, a PCM data
	// stream where the left and right channel samples are interleaved as L-R-L-R-L-R etc.
	iCaps.iDataFormats = KSoundDataFormatInterleaved;

	// The iRequestMinSize member is named badly.  It is actually the value of which the length samples
	// must be a multiple of.  ie.  The sample length % iRequestMinSize must == 0.  This value must always
	// be a power of 2
	iCaps.iRequestMinSize = 4;

	// The logarithm to base 2 of the alignment required for request arguments.  DMA requests must be
	// aligned to a 32 bit boundary
	iCaps.iRequestAlignment = 2;

	// This unit is not capable of detecting changes in hardware configuration
	iCaps.iHwConfigNotificationSupport = EFalse;
	}




