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

#ifndef VIRTIO_DEFS_H
#define VIRTIO_DEFS_H

#include <e32def.h>
#include <kernel.h> // fot TPhysAddr

/// @file virtio_defs.h
/// most of definitions here come from VirtIo spec
/// and was made compatible to the qemu backend (which means diverged from spec)

namespace VirtIo
{

#define POW2ALIGN1(x) ((x)|((x)>>1))
#define POW2ALIGN2(x) ((x)|((x)>>2))
#define POW2ALIGN4(x) ((x)|((x)>>4))
#define POW2ALIGN8(x) ((x)|((x)>>8))
#define POW2ALIGN16(x) ((x)|((x)>>16))

///@brief Alignes \a x to the closest bigger or equal power2 value
#define POW2ALIGN(x) (POW2ALIGN16(POW2ALIGN8(POW2ALIGN4(POW2ALIGN2(POW2ALIGN1((x)-1)))))+1)

static const TUint KVirtIoAlignment = 0x1000;

enum 
	{
	EStatusReset = 0,
	EStatusAcknowledge = 1,
	EStatusDriverFound = 2,
	EStatusDriverInitialised = 4,
	EStatusFailed = 0x80
	};

enum 
	{
    EIoID             = 0,
    EIoDevType        = 1,
    EIoHostFeatures  = 2,
    EIoGuestFeatures = 3,
    EIoQueueBase     = 4,
    EIoQueueSize      = 5,
    EIoQueueSelect      = 6,
    EIoQueueNotify   = 7,
    EIoStatus	  	  = 8,
    EIoInterruptEnable     = 9,
    EIoInterruptStatus     = 10
	};

struct TRingDesc
	{
	enum 
		{
		EFlagNext = 1,
		EFlagWrite = 2
		};

	TUint64 iAddr;	// physical address
	TUint32 iLen;
	TUint16 iFlags;
	TUint16 iNext;
	};

struct TRingAvail
	{
	enum { EFlagInhibitNotifications = 1 };
	TUint16 iFlags;
	TUint16 iIdx;
	TUint16 iRing[1];
	};

struct TRingUsed
	{
	enum { EFlagInhibitNotifications = 1 };
	TUint16 iFlags;
	TUint16 iIdx;
		struct 
		{
		TUint32 iId;
		TUint32 iLen;
		} iRing[1];
	};

/// @brief Represents element of scatter gather list
struct TAddrLen
	{
	TPhysAddr iAddr;
	TUint32 iLen;
	};

/// Gives a pointer resulting with aliging \a v with \a a.
/// @note \a a must be power of 2.
template <typename T> T* Align( T* v, TUint a )
	{ return (T*)( ((TUint8*)v) + ((-(TUint)v)&(a-1)) ); }
	
/// Gives a pointer resulting with aliging \a v with \a a.
/// @note \a a must be power of 2.
template <typename T> T Align( T v, TUint a )
	{ return (v)+((-v)&(a-1)); }

}

#endif
