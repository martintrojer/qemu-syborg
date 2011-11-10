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

#ifndef VIRTIO_QUEUE_H
#define VIRTIO_QUEUE_H

#include "virtio.h"
#include <e32def.h>

namespace VirtIo
{

/// @brief implements VirtIo Queue.
/// 
///
/// @note functions are not enclosed with any synchronisation primitives.
/// However, implementation is believed to split operations in 3 groups with the following concurrency constraints:
/// <li> A GetBuf, DetachBuf
/// <li> B AddBuf
/// <li> C Processing, Completed, Sync
/// Group C can be executed concurrently with any other functions.
/// Group A can be executed concurrently with a group B function.
/// Group A or B function cannot be executed concurrently with function from the same group.
///
/// Memory barries would have to be introduced for SMP.
///
class DQueue : public DBase, public MQueue
	{
	typedef TUint16 TIdx;

	struct TTransactionInfo
		{
		Token iToken;
		TUint iTotalLen;
		};
	
	static const TUint KFreeDescriptorMarker = 0xFFFF;
	
public:

	/// @brief Creates a VirtIo Queue with a MIo object, Queue Id,
	/// and descriptor count/ring lenght.
	DQueue( MIo& aVirtIo, TUint aId, TUint aCount );
	
	/// Allocates resources
	TInt Construct();
		
	virtual ~DQueue();
	
public: // implementation of MQueue
	
	virtual TInt AddBuf( const TAddrLen aScatterList[], TUint32 aOutNum, TUint32 aInNum, Token aToken);
	
	virtual TInt DetachBuf( Token aToken );
	
	virtual Token GetBuf( TUint& aLen );
	
	virtual void Sync();
	
	virtual TUint Id() 
		{ return iId; }
	
	virtual TBool Restart()
		{ return ETrue;	}
	
	virtual TInt Processing();
			
	virtual TInt Completed();	
	
public:	// Debug functions 
	void DumpUsedPending();
	
	void DumpUsed(TUint usedSlot);
	void DumpAvailPending();
	
	void DumpAvail(TUint availSlot);
	void DumpDescs(TUint descId );
	
	virtual void DumpAll();
		
private:
	void Wipe();

	TUint Slot( TUint i )
		{ return i & (iCount - 1); }
				
	void FreeDescs( TUint firstDescIdx );
	
	TUint8* AllocMem( TUint aSize );
	
	TInt AllocQueue();
	
	void PreInitQueue();
	
	void PostInitQueue();
	
private:
	MIo& iVirtIo;
	const TUint iId;
	const TUint iCount;
	volatile TUint iNextUsedToRead;
	TUint iFirstEverToRead;
	volatile TUint iNextAvailToSync;
	TUint iFirstEverToSync;

	TUint iDescSize;
	TUint iAvailSize;
	TUint iTokenSize;
	TUint iUsedSize;
	
	TRingDesc* iDesc;
	TRingAvail* iAvail;
	volatile TRingUsed* iUsed;
	TTransactionInfo* iToken;
	
	// managed
	TUint8* iMem;
	TPhysAddr iPhysAddr;
	TBool iPhysMemReallyAllocated;
	DPlatChunkHw* iChunk;
	
	};


} // namespace VirtIo

#endif
