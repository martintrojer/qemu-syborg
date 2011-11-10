/*
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Accenture Ltd
*
* Contributors:
*
* Description: This file is a part of sound driver for Syborg adaptation.
*
*/

#ifndef VIRTIO_AUDIO_H
#define VIRTIO_AUDIO_H

#include "virtio_audio_defs.h"
#include "virtio_defs.h"
#include "virtio.h"

#include <e32def.h>

namespace VirtIo
{

namespace Audio
{

/// Encapsulates routines that affect VirtIo Audio device's Stream.
/// DControl object may share ioHandler with other DControl objects.
/// especially Control Queue.
class DControl : public DBase
	{
	static const TUint KCmdMaxBufCount = 8;
	static const TUint KControlQueueId = 0;
public:
	enum Command { ERun = 5, EStop, EPause, EResume, EShutDown };
	
	DControl( VirtIo::MIoHandler& aIoHandler, TUint32 aDataQueueId )
	: iIoHandler( aIoHandler ), iDataQueueId( aDataQueueId), iIsRunning( 0 )
		{}
		
	~DControl();
		
	TInt Construct();
	
	/// sets up stream parameters.
	TInt Setup( StreamDirection aDirection, TInt aChannelNum, 
		FormatId aFormat, TInt aFreq );
		
	/// @brief posts a data buffer. 
	///
	/// The virtaulAddr is going to be turned into 
	/// a physical address scatter gather list.
	TInt SendDataBuffer( TAny* virtaulAddr, TUint aSize, Token aToken );
	
	/// Sends a specific command
	void SendCommand( Command aCmd )
		{ AddCommand( aCmd ); ControlQueue().Sync(); }

	/// Return Data Queue of the stream.
	MQueue& DataQueue()
		{ return iIoHandler.Queue(iDataQueueId); }

	/// Returns Control Queue of the associated device.
	MQueue& ControlQueue()
		{ return iIoHandler.Queue(KControlQueueId); }	

private:
	// size of this struct needs to be power2 aligned
	// we extend original TCommand with padding to ensure this
	struct TCommandPadded: public TCommand
		{
		static const TUint KPaddingSize = POW2ALIGN(sizeof(TCommand))-sizeof(TCommand);
		TUint8 iPadding[KPaddingSize];
		};
	
	void AddCommand( TCommandPadded* aCmd, Token aToken );
	
	void AddCommand( TCommandPadded* aCmd, Token aToken, TAny* mem, TUint size );

	void AddCommand( Command aCmd );

	VirtIo::MIoHandler& iIoHandler;
	TUint iDataQueueId;
	
	TUint8* iCmdMem; // managed
	TCommandPadded* iCmd; // unmanaged
	TBufferInfo* iBufferInfo; // unmanaged
	
	StreamDirection iDirection;
    
    TUint iIsRunning;
	};

} // namespace Audio
} // namespace VirtIo


#endif
