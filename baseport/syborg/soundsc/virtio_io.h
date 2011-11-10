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

#ifndef VIRTIO_IO_H
#define VIRTIO_IO_H

#include "virtio.h"
#include <e32def.h>

namespace VirtIo
{

class DIo : public DBase, public MIo
	{
public:
	DIo( TAny* aIoBase ): iIoBase( reinterpret_cast<TUint32*>( aIoBase ) )
		{ }
	
	virtual void GetDeviceIds( TUint32 &aPeripheralId, TUint32 &aDeviceId );
	
	virtual void SetQueueBase( TUint aQId, TAny* iDescRegion );
	
	virtual TAny* GetQueueBase( TUint aQId);
	
	virtual TUint GetQueueCount( TUint aQId )
		{
		write(EIoQueueSelect, aQId);
		return read(EIoQueueSize);
		}
	
	virtual void PostQueue( TUint aQId )
		{ write(EIoQueueNotify, aQId); SYBORG_VIRTIO_DEBUG("PostQueue Q%d", aQId ); }
		
	virtual TBool EnableInterrupt( TBool aEnable )
		{ return swap(EIoInterruptEnable, aEnable?1:0); }

	virtual void SetStatus( TUint status )
		{ write(EIoStatus, status ); }
		
	virtual void ClearInteruptStatus()
		{ write( EIoInterruptStatus, 1 ); }

	virtual TBool InteruptStatus()
		{ return read( EIoInterruptStatus ); }
		
	
	void Reset()
		{ SetStatus( EStatusReset ); };
	
	
private:	
	void write(TUint idx, TUint32 value )
		{ iIoBase[idx] = value; }
		
	TUint32 read(TUint idx)
		{ return iIoBase[idx]; }
		
	TUint32 swap(TUint idx, TUint32 value )
		{ 
		TUint32 t = iIoBase[idx]; 
		iIoBase[idx] = value; 
		return t;
		}
	
	volatile TUint32* iIoBase;

	};

} // namespace VirtIo

#endif
