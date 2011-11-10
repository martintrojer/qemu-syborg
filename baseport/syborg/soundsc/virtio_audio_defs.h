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

#ifndef VIRTIO_AUDIO_DEFS_H //x
#define VIRTIO_AUDIO_DEFS_H

#include <e32def.h>

/// @file virtio_audio_defs.h
/// most of definitions here come from VirtIo Audio spec
/// and was made compatible to the qemu backend (which means diverged from spec)

namespace VirtIo
{
namespace Audio
{

static const TUint KMaxSGLItemCountPerAudioBuffer = 64;

enum CommandId
	{
	ECmdSetEndian=1,
	ECmdSetChannels=2,
	ECmdSetFormat=3,
	ECmdSetFrequency=4,
	ECmdInit=5,
	ECmdRun=6
	};

enum FormatId
	{
	EFormatU8=0,
	EFormatS8=1,
	EFormatU16=2,
	EFormatS16=3,
	EFormatU32=4,
	EFormatS32=5
	};

enum RunType
	{
	EDoStop = 0,
	EDoRun = 1
	};

// note the values are opposite to what spec says
enum StreamDirection
	{
	EDirectionPlayback = 0,
	EDirectionRecord = 1,
    EDirectionNone = 2 // closes current stream
	};

struct TCommand
	{
	TUint32 iCommand;
	TUint32 iStream;
	TUint32 iArg;
	};
	
struct TBufferInfo
	{
	TUint32 bufferSize;
	TUint32 iA[3];	// not really documented
	};	

static const TUint32 KVirtIoPeripheralId = 0xc51d000a;
static const TUint32 KVirtIoDeviceId = 0xffff;

} // namespace Audio
} // namespace VirtIo

#endif

