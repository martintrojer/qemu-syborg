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

#include "virtio_iohandler.h"
#include "virtio_queue.h"
#include "virtio_io.h"

namespace VirtIo
{

DIoHandler::DIoHandler( TAny* aVirtIoBase, TUint aIntNum, TDfcQue* aDfcQue )
	: iVirtIoBase( aVirtIoBase ), iIntNum( aIntNum ), iDfcQue( aDfcQue ),
	iDfc( ServeDfc, this, 0 ),
	iVirtIo( 0 ), iQueueCount( 3 ), iQueue( 0 ),
	iClientCount( 0 )
	{
	SYBORG_VIRTIO_DEBUG("DIoHandler IoBase %x DfcQ %x", iVirtIoBase, iDfcQue);
	}

TInt DIoHandler::Construct()
	{
	TInt err = KErrNone;
	TUint i;

	SYBORG_VIRTIO_DEBUG("Creating DIo");

	InstallIsr();

	iVirtIo = new DIo(iVirtIoBase);
	if (iVirtIo == NULL )
		{ return KErrNoMemory; }
	iVirtIo->SetStatus( EStatusAcknowledge
			| EStatusDriverFound );

	iQueue = new DQueue*[iQueueCount];
	
	if (iQueue == NULL )
		{ Wipe(); return KErrNoMemory; }
	
	// This is to make Wipe work
	for ( i = 0; i < iQueueCount; ++i )
		{ iQueue[i] = NULL; }
	
	for ( i = 0; i < iQueueCount; ++i )
		{
		SYBORG_VIRTIO_DEBUG("Creating DQueue %d",i);
		DQueue* q = new DQueue(
			*iVirtIo, i, iVirtIo->GetQueueCount( i ) );
		if (!q)
			{ Wipe(); return KErrNoMemory; }
		err = q->Construct();
		if (err != KErrNone)
			{ Wipe(); return err; }
		iQueue[i] = q;
		}
		
	iVirtIo->SetStatus(
		EStatusAcknowledge
		| EStatusDriverFound
		| EStatusDriverInitialised );

	iVirtIo->EnableInterrupt( ETrue );
	Interrupt::Enable(iIntNum);
	return KErrNone;
	}

void DIoHandler::Wipe()
	{
	if (iQueue)
		{
		for ( TUint i = 0; i < iQueueCount; ++i )
			{
			if (iQueue[i])
				{ delete iQueue[i]; }
			}
		delete[] iQueue;
		iQueue = NULL;
		}
	if (iVirtIo)
		{
		delete iVirtIo;
		iVirtIo = NULL;
		}
	}

DIoHandler::~DIoHandler()
	{

	Interrupt::Disable(iIntNum);
	UninstallIsr();
	iVirtIo->EnableInterrupt( EFalse );
	iDfc.Cancel();
	iVirtIo->ClearInteruptStatus();

	WaitForCompletion();

	iVirtIo->SetStatus( EStatusAcknowledge
		| EStatusDriverFound );
	iVirtIo->SetStatus( EStatusAcknowledge );

	Wipe();
	}
	
MQueue& DIoHandler::Queue( TUint id )
	{ return *(iQueue[id]); }	

void DIoHandler::ScheduleCallback()
	{
	iDfc.Enque();
	}


// Waits until device processes all pending requests
// This code should be really handled by each queue individually
void DIoHandler::WaitForCompletion()
	{
	SYBORG_VIRTIO_DEBUG("WaitForCompletion : {");

	TInt st = Kern::PollingWait( &DIoHandler::CheckProcessing, this, 50, 100 );

	ASSERT( st == KErrNone );

	for ( TUint i = 0; i < iQueueCount; ++i )
		{
		while ( iQueue[i]->Completed() )
			{ InterruptHandler(); }
		}

	SYBORG_VIRTIO_DEBUG("WaitForCompletion : }");
	}

TBool DIoHandler::CheckProcessing( TAny* aSelf )
	{
	DIoHandler* self = reinterpret_cast<DIoHandler*>( aSelf );
	for ( TUint i = 0; i < self->iQueueCount; ++i )
		{
		if (self->iQueue[i]->Processing()!=0)
			{ return EFalse; }
		}
	return ETrue;
	}

void DIoHandler::InstallIsr()
	{
	iDfc.SetDfcQ( iDfcQue );
	Interrupt::Bind(iIntNum,ServeIsr,this);
	}

void DIoHandler::UninstallIsr()
	{
	Interrupt::Unbind(iIntNum);
	iDfc.Cancel();
	}

void DIoHandler::ServeIsr( TAny* aSelf )
	{
	DIoHandler* self = reinterpret_cast<DIoHandler*>( aSelf );
	Interrupt::Clear( self->iIntNum );
	self->iVirtIo->ClearInteruptStatus();
	self->iDfc.Add();
	}

void DIoHandler::ServeDfc( TAny* aSelf )
	{
	reinterpret_cast<DIoHandler*>( aSelf )->InterruptHandler();
	}

	
// Although the function notifies all clients
// usually only one of them is an adressee
// the rest would just check flags/compare numbers and return.
// If at least one client did some crucial processing 
// (indicating that by returning EFalse from VirtIoCallback)
// then NotifyClients returns EFalse as well.
TBool DIoHandler::NotifyClients( MQueue& aQueue, Token aToken, TUint aBytesTransferred )
	{
	TBool r = ETrue;
	for ( TUint i = 0 ; i < iClientCount; ++i )
		{
		r &= iClients[i]->VirtIoCallback( *this, aQueue, aToken, aBytesTransferred );
		}
	return r;
	}
	
// Here buffers processed by the device are iterated.
// After the first serious buffer processing (as indicated by NotifyClients)
// further buffer processing is Deferred in another DFC callback.
void DIoHandler::InterruptHandler()
	{
	for ( TUint q = 0; q < iQueueCount; ++q )
		{
		TUint transferred;
		TUint cnt = ETrue;
		do 
			{
			Token t = Queue(q).GetBuf(transferred);
			if (t)
				{ cnt = NotifyClients( Queue(q), t, transferred); }
			else 
				{ break; }
			} while(cnt);
		if (!cnt)
			{
			ScheduleCallback(); 
			break;
			}
		}
	}

void DIoHandler::UnregisterClient( MIoCallback* aClient )
	{
	ASSERT( iClientCount );
	TUint i;
	for ( i = 0; i < iClientCount; ++i)
		{
		if (iClients[i] == aClient )
			{ break; }
		}
	ASSERT( i < iClientCount );
	--iClientCount;
	// move the rest of the table one slot to the front
	for ( ; i < iClientCount; ++i) 
		{ iClients[i] = iClients[i+1]; }
	}	

} // namespace VirtIo
