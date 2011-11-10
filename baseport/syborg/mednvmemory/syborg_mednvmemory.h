/*
* Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description: Minimalistic non volatile memory driver
*
*/

#ifndef _SYBORG_MEDNVMEMORY_H
#define _SYBORG_MEDNVMEMORY_H

#include <kpower.h>
#include <e32keys.h>
#include <system.h>

//#define DEBUG_MEDNVMEMORY
#ifdef DEBUG_MEDNVMEMORY
#define __DEBUG_PRINT(format...)    Kern::Printf(format)
#else
#define __DEBUG_PRINT(format...)    __KTRACE_OPT(KLOCDRV,Kern::Printf(format))
#endif

#define R_NVMEM_ID                                0x0000
#define R_NVMEM_TRANSACTION_OFFSET                0x0004
#define R_NVMEM_TRANSACTION_SIZE                  0x0008
#define R_NVMEM_TRANSACTION_DIRECTION             0x000c
#define R_NVMEM_TRANSACTION_EXECUTE               0x0010
#define R_NVMEM_SHARED_MEMORY_BASE                0x0014
#define R_NVMEM_NV_MEMORY_SIZE                    0x0018
#define R_NVMEM_SHARED_MEMORY_SIZE                0x001c
#define R_NVMEM_STATUS                            0x0020
#define R_NVMEM_ENABLE                            0x0024
#define R_NVMEM_LASTREG                           0x0028  // not a register, address of last register

#define NVMEM_TRANSACTION_UNDEFINED 0
#define NVMEM_TRANSACTION_READ 1
#define NVMEM_TRANSACTION_WRITE 2

// 512 Byte sectors
const TInt KDiskSectorShift	= 9;
const TInt KDiskSectorSize	= 0x200;

// Dfc thread priority 
const TInt KNVMemDfcThreadPriority = 24;

// Transfer buffer size 256kBytes 
const TInt KNVMemTransferBufferSize = 0x40000;
// Transfer buffer size in sectors
const TInt KNVMemTransferBufferSectorCount = KNVMemTransferBufferSize/KDiskSectorSize;

#endif
