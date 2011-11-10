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

#include "virtio_audio.h"

namespace VirtIo
{
namespace Audio
{

static TBool CheckProcessing( TAny* aSelf )
	{
	MQueue* queue = reinterpret_cast<MQueue*>( aSelf );
	return queue->Processing() == 0;
	}

static void WaitForCompletion( MQueue& aQueue )
	{
	SYBORG_VIRTIO_DEBUG("AddBufferHelperWaitForCompletion : {");

	TInt st = Kern::PollingWait( &CheckProcessing, &aQueue, 50, 100 );
	ASSERT ( (st == KErrNone) && "Polling problem" )
	
	SYBORG_VIRTIO_DEBUG("AddBufferHelperWaitForCompletion : }");
	}

static void AddBufferHelper( MQueue& aQueue, TAddrLen aList[], TUint aBufInCount, TUint aBufOutCount, Token aToken)
	{
	TInt st = aQueue.AddBuf(aList, aBufInCount, aBufOutCount, aToken );
	if ( st == KErrNotReady )
		{
		SYBORG_VIRTIO_DEBUG("AddBufferHelper - no free descriptors at Control Queue, forcing wait");
		TUint transferred;
		Token t;
		while ( ( t = aQueue.GetBuf(transferred), t ) != 0 )
			{
			SYBORG_VIRTIO_DEBUG("flushing Q%x, T%x, L%x\n", 0, t, transferred );
			}
			
		st = aQueue.AddBuf(aList, aBufInCount, aBufOutCount, aToken ); 
		}
	ASSERT( st == KErrNone );
	}

DControl::~DControl()
	{
	if (iCmdMem)
		{ Kern::Free( iCmdMem ); }
	}

TInt DControl::Construct()
	{
	TUint cmdSize = sizeof(iCmd[0])*KCmdMaxBufCount;
	TUint size = cmdSize + sizeof(*iBufferInfo);
	// as we are going to align the allocated memory
	// we add extra alignment size minus 1
	iCmdMem = reinterpret_cast<TUint8*>( Kern::Alloc( size + sizeof(iCmd[0])-1 ) );
	if (!iCmdMem)
		{ return KErrNoMemory; }
		
	// let us align the memory address 
	// note: sizeof(iCmd[0]) is a power of 2
	ASSERT( sizeof(iCmd[0]) == POW2ALIGN(sizeof(iCmd[0])) );
	TUint8* alignedMem = iCmdMem 
		+ ( (-(TUint32)iCmdMem)&(sizeof(iCmd[0])-1) ); // adding missing amount of bytes
	
	iCmd = reinterpret_cast<TCommandPadded*>( alignedMem );
	iBufferInfo = reinterpret_cast<TBufferInfo*>( alignedMem + cmdSize );
	
	for (TUint i = 0; i< KCmdMaxBufCount; ++i)
		{
		iCmd[i].iStream = iDataQueueId - 1;
		}	
	iCmd[0].iCommand = Audio::ECmdSetEndian;
	iCmd[1].iCommand = Audio::ECmdSetChannels;
	iCmd[2].iCommand = Audio::ECmdSetFormat;
	iCmd[3].iCommand = Audio::ECmdSetFrequency;
	iCmd[4].iCommand = Audio::ECmdInit;
	iCmd[5].iCommand = Audio::ECmdRun;
	iCmd[5].iArg = Audio::EDoRun; //start stream
	iCmd[6].iCommand = Audio::ECmdRun;
	iCmd[6].iArg = Audio::EDoStop; //stop stream		
	iCmd[7].iCommand = Audio::ECmdRun;
	iCmd[7].iArg = Audio::EDoStop; //kind of pause
	iCmd[8].iCommand = Audio::ECmdRun;
	iCmd[8].iArg = Audio::EDoRun; //kind of resume
	iCmd[9].iCommand = Audio::ECmdInit; // kind of shutdown
    iCmd[9].iArg = EDirectionNone;
	return KErrNone;
	}

TInt DControl::Setup( StreamDirection aDirection, TInt aChannelNum, 
	FormatId aFormat, TInt aFreq)
	{
    if (iIsRunning 
        && (( aDirection != iDirection ) 
            || (aFormat != iCmd[2].iArg )
            || (aFreq != iCmd[3].iArg )
            || (aChannelNum != iCmd[1].iArg )
        ))
        { return KErrInUse; }

	iCmd[1].iArg = aChannelNum;
	iCmd[2].iArg = aFormat;
	iCmd[3].iArg = aFreq;		
	iCmd[4].iArg = iDirection = aDirection;
	AddCommand(&iCmd[1],(Token)1);
	AddCommand(&iCmd[2],(Token)2);
	AddCommand(&iCmd[3],(Token)3);
    if (!iIsRunning) {
            AddCommand(&iCmd[4],(Token)4, iBufferInfo, sizeof(*iBufferInfo) );
            AddCommand(&iCmd[6],(Token)6);
        }
	ControlQueue().Sync();
	return KErrNone;
	}

void DControl::AddCommand( TCommandPadded* aCmd, Token aToken )
	{
	TAddrLen list;
	list.iLen = sizeof(TCommand);
	list.iAddr = Epoc::LinearToPhysical((TUint32)aCmd);
	SYBORG_VIRTIO_DEBUG("AddCommand %x %x %x", aCmd->iCommand, aCmd->iStream, aCmd->iArg);
	AddBufferHelper( ControlQueue(), &list, 1, 0, aToken );
	}

void DControl::AddCommand( TCommandPadded* aCmd, Token aToken, 
	TAny* aMem, TUint aSize )
	{
	TAddrLen list[2];
	list[0].iLen = sizeof(TCommand);
	list[0].iAddr = Epoc::LinearToPhysical((TUint32)aCmd);
	list[1].iLen = aSize;
	list[1].iAddr = Epoc::LinearToPhysical((TUint32)aMem);
	AddBufferHelper( ControlQueue(),list, 1, 1, aToken );
	}

void DControl::AddCommand( Command aCmd )
	{
	TUint idx = aCmd;
	if ( (aCmd == EStop) || (aCmd == EShutDown) )
		{
		// due to bug on qemu's side we need to stop sending buffers
		// and wait for all pending buffers to get filled...
		WaitForCompletion(DataQueue());
        WaitForCompletion(ControlQueue());
        iIsRunning = 0;
        }
	AddCommand(&iCmd[idx], (Token)idx );
    if (aCmd == ERun )
        {
        iIsRunning = 1;
        }
	}

TInt DControl::SendDataBuffer( TAny* virtaulAddr, TUint aSize, Token aToken )
	{
	TAddrLen sgl[KMaxSGLItemCountPerAudioBuffer];
	TUint sglCount = KMaxSGLItemCountPerAudioBuffer;
	TInt st = LinearToSGL(virtaulAddr, aSize, sgl, sglCount );
	ASSERT( st != KErrNoMemory ); // failure means our fixed lenght sgl table is too small
	if (st!=KErrNone)
		{ return st; }
	st = DataQueue().AddBuf( sgl, 
		iDirection == EDirectionPlayback ? sglCount : 0,
		iDirection == EDirectionRecord ? sglCount : 0,
		aToken );
	if (st!=KErrNone)
		{ return st; }
	DataQueue().Sync();
	return KErrNone;
	}
	

} // namespace Audio
} // namespace VirtIo
