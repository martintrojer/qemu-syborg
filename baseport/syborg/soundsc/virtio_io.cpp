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

#include "virtio_io.h"

namespace VirtIo
{

void DIo::GetDeviceIds( TUint32 &aPeripheralId, TUint32 &aDeviceId )
	{
	aPeripheralId = read( EIoID );
	aDeviceId = read( EIoDevType );
	}
	
void DIo::SetQueueBase( TUint aQId, TAny* iDescRegion )
	{
	TUint32 phAddr = iDescRegion?Epoc::LinearToPhysical( reinterpret_cast<TUint32>( iDescRegion ) ):0;
	write(EIoQueueSelect, aQId);
	write(EIoQueueBase, phAddr );
	SYBORG_VIRTIO_DEBUG("SetQueueBase Q%d %x", aQId, phAddr );
	}		
	
	
TAny* DIo::GetQueueBase( TUint aQId)
	{
	write(EIoQueueSelect, aQId);
	return reinterpret_cast<TAny*>( read(EIoQueueBase) );
	}
	
} // namespace VirtIo
