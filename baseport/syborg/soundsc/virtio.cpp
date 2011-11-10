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

#include "virtio.h"

namespace VirtIo 
{

#define MIN(x,y) (((x)<=(y))?(x):(y))
#define INVALID_PHYSADDR(x) ((x)==TPhysAddr(-1))

TInt LinearToSGL( TAny* aVirtual, TUint aSize, TAddrLen aSGL[], TUint& aSGLCount )
	{
	const TUint pageSize = Kern::RoundToPageSize(1);
	TUint8* virtAddr = reinterpret_cast<TUint8*>( aVirtual );
	TPhysAddr physAddr = Epoc::LinearToPhysical( TLinAddr(virtAddr) );
	if (INVALID_PHYSADDR(physAddr))
		{ return KErrArgument; }
	TUint sglLimit = aSGLCount;
	aSGLCount = 0;
	TUint left = aSize;
	while (left)
		{
		TPhysAddr startPhysAddr = physAddr;
		TUint8* startVirtAddr = virtAddr;
		while ( (left) 
			&& ( Epoc::LinearToPhysical( TLinAddr(virtAddr) ) == physAddr ) )
			{
			TUint size = MIN(left, pageSize);
			physAddr += size;
			virtAddr += size;
			left -= size;
			}
		if (INVALID_PHYSADDR(physAddr))
			{ return KErrArgument; }
		if ((aSGL) && (aSGLCount<sglLimit))
			{
			aSGL[aSGLCount].iAddr = (TUint32) startPhysAddr;
			aSGL[aSGLCount].iLen = virtAddr - startVirtAddr;
			}
		aSGLCount++;
		}
	return (aSGLCount<=sglLimit)?KErrNone:KErrNoMemory;
	}
	
} // namespace VirtIo	
