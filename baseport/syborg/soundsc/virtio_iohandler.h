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

#ifndef VIRTIO_IOHANDLER_H
#define VIRTIO_IOHANDLER_H

#include "virtio.h"
#include <e32def.h>

namespace VirtIo
{

class DQueue;

class DIo;


class DIoHandler : public DBase, public MIoHandler
	{
	static const TUint KMaxClients = 4;
public:
	DIoHandler( TAny* aVirtIoBase, TUint aIntNum, TDfcQue* aDfcQue );
	
	virtual void RegisterClient( MIoCallback* aClient )
		{
		ASSERT( iClientCount < KMaxClients );
		iClients[iClientCount++] = aClient;
		}
	
	virtual void UnregisterClient( MIoCallback* aClient );

	TInt Construct();
	
	
	virtual ~DIoHandler();
	
	virtual MQueue& Queue( TUint id );

private:	
	void ScheduleCallback();
	
	void WaitForCompletion();
	
	void Wipe();
	
	static TBool CheckProcessing( TAny* aSelf );
	
	void InstallIsr();
	
	void UninstallIsr();
	
	static void ServeIsr( TAny* aSelf );

	static void ServeDfc( TAny* aSelf );
	
	TBool NotifyClients( MQueue& aQueue, Token aToken, TUint aBytesTransferred );
	
	void InterruptHandler();
	
	TAny* iVirtIoBase;
	TUint iIntNum;
	
	TDfcQue* iDfcQue;
	TDfc iDfc;
	
	//managed
	DIo* iVirtIo;
	
	TUint iQueueCount;
	//double managed
	DQueue** iQueue;
		
	TUint iClientCount;	
	MIoCallback* iClients[KMaxClients];
	
	};


} // namespace VirtIo

#endif
