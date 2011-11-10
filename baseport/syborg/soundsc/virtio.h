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

#ifndef VIRTIO_H
#define VIRTIO_H

/// @file virtio.h
/// @brief Delivers the API for dealing with VirtIo device.
///
/// Mainly a VirtIo device functionality is accessed through the following entities.
/// <li> MIo that deals with basic io space access
/// <li> MQueue that represents a device's transaction queue, which is usually more then one per device
/// <li> MIoHandler that represents a device, takes care of resource management, device initialisation and MIo and MQueue derived objects' lifetime.
///
/// @p The idea is that MIo, MQueue and MIoHandler comes as a specific set. On top of which it is possible to build various drivers (e.g. Audio or Ethernet) that are agnostic to these virtio specifics (e.g. flat device tree or full PCI).
///
/// Currently the implementation is not multithread safe. The assumption is that all the routines are driven from a single DFC.

#include "virtio_defs.h"

#include <e32lang.h>
#include <assp.h>


#ifndef DISABLE_SYBORG_SOUND_DEBUG
#define SYBORG_VIRTIO_DEBUG(x...) __KTRACE_OPT(KSOUND1, Kern::Printf(x))
#else
#define SYBORG_VIRTIO_DEBUG(x...)
#endif

#undef ASSERT
#define ASSERT(x) (x) || (Kern::Printf("VirtIo: ASSERTION FAILED: "#x),0);


namespace VirtIo
{

/// @brief type of a buffer token.
typedef TAny* Token;

/// @brief represents a virtual queue's API.
///
class MQueue
	{
public:

	/// @brief Adds a buffer at the end of the queue
	///
	/// @param aScatterList - physical address scatter list
	/// @param aOutNum - number of scatter buffers to be sent to the device
	/// @param aInNum - number of scatter buffers to be received from the device (starting at aOutNum index of scatter list)
	/// @param aToken - a value associated with buffer to be returned by getBuf or used with detachBuf
	virtual TInt AddBuf( const TAddrLen aScatterList[], TUint32 aOutNum, 
		TUint32 aInNum, Token aToken) = 0;

	/// @brief Returns buffer associated with the least recent completed transaction
	/// @return transaction's Token with \a len set to reflect amount of processed bytes or 
	/// 0 if no completed transaction was found. 
	virtual Token GetBuf( TUint& len ) = 0;
	
	/// @brief Posts queued buffers to the device
	virtual void Sync() = 0;
	
	/// @brief Cancels a specified buffer (if it is not yet posted)
	/// To be used at shutdown
	virtual TInt DetachBuf( Token aToken ) = 0;
	
	/// @brief reenable callbacks
	/// @return ETrue - callbacks reenabled, EFalse - callbacks not reenabled as there are pending buffers in
	/// the DQueue.
	virtual TBool Restart() = 0;
	
	/// @brief returns number of buffers that are posted to the DQueue 
	/// but not yet completed
	virtual TInt Processing() = 0;
	
	/// @brief returns number of times the GetBuf function could be called returning a buffer
	virtual TInt Completed() = 0;
	
	/// Returns Id of the queue
	virtual TUint Id() = 0;
	
	};


/// @brief API wrapping the VirtIo IO space access.
class MIo
	{
public:
	virtual void SetQueueBase( TUint aId, TAny* iDescRegion ) = 0;
	virtual TAny* GetQueueBase( TUint aId ) = 0;
	virtual void PostQueue( TUint aId ) = 0;
	virtual void GetDeviceIds( TUint32 &aPeripheralId, TUint32 &aDeviceId ) = 0;
	virtual TUint GetQueueCount( TUint aQId ) = 0;
	virtual TBool EnableInterrupt( TBool aEnable ) = 0;
	virtual void SetStatus( TUint status ) = 0;
	virtual void ClearInteruptStatus() = 0;
	virtual TBool InteruptStatus() = 0;
	
	};


class MIoCallback;

/// @brief represents a VirtIo device's API.
///
/// An API for a handler that takes care of resources, state and asyncrhonous communication of a VirtIo device. Allocates Queues.
class MIoHandler
	{
public:
	/// returns one of the device's queues.
	virtual MQueue& Queue( TUint aId ) = 0;
	
	/// registers a client for buffer notifications.
	virtual void RegisterClient( MIoCallback* aListener ) = 0;	
	
	/// unregisters a client for buffer notifications.
	virtual void UnregisterClient( MIoCallback* aListener )	= 0;
	};

/// an interface for a MIoHandler's client to implement.
class MIoCallback
	{
public:
	/// Notifies MIoHandler's client that that a buffer processing has been completed.
	///
	/// @return EFalse to defer subsequent buffer processing in the same callback.
	virtual TBool VirtIoCallback( 
		MIoHandler& aVirtIoHandler, MQueue& aQueue, 
		Token aToken, TUint aBytesTransferred ) = 0;
	};

/// @brief turns virtual address range into a phys scatter gather list
///
/// If virtual buffer contains a linear buffer then the 
/// function will create one SGL entry.
///
/// If function is called with aSGL=0 then it will count the linear physical
/// fragments the buffer is build of. 
///
///
/// @return KErrArgument if the virtual buffer contains nonconvertible addresses)
TInt LinearToSGL( TAny* aLinear, TUint aSize, TAddrLen aSGL[], TUint& aSGLCount );

}


#endif
