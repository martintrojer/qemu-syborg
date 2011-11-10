/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
* NTT Docomo, Inc : BUG 1296
*
* Description:
*
*/

/**
 * @file
 * @internalTechnology
 *
 * The highrestimer.h header file defines how to access the high resoltion
 * timer, if one is supported.  This file is used by default if the variant does
 * not export one to \epoc32\include\nkern. 
 */

#ifndef __HIGHRESTIMER_H__
#define __HIGHRESTIMER_H__
//
// Copyright (c) 2008 Symbian Ltd. All rights reserved.
//

#include <syborg.h>

/**
 * Macro indicating that a high resolution timer is supported.
 */
#define HAS_HIGH_RES_TIMER

/**
 * Assembler macro to get the the current value of the high res timer and place
 * it in the specified register.
 * Reads the 32-bit value from a free-running counter that represents the current time. 
 * Syborg timers have 'microsecond resolution'. NB. This value comes from the host clock
 * and so there is a good chance that the kernels tick based notion of time (elapsed) and that
 * measured by the free running timer will get out of kilter. This affects t_cputime from e32test
 * amongst other things.
 */
//#define GET_HIGH_RES_TICK_COUNT(Rd) asm("nop");
// Hi-jacking r10 for tmp, not good if Rd is R10 -- grepping the the kernel shows it's not (for now)
#define GET_HIGH_RES_TICK_COUNT(Rd)						         \
  asm("push {r10}");										     \
  asm("mov r10, #1");										     \
  asm("ldr "#Rd", =%a0"               : : "i" (KHwBaseRtc + 4)); \
  asm("str r10, ["#Rd", #%a0]"        : : "i" (0));			     \
  asm("pop {r10}");                                              \
  asm("ldr "#Rd", =%a0"               : : "i" (KHwBaseRtc + 8)); \
  asm("ldr "#Rd", ["#Rd", #%a0]"      : : "i" (0));              

/**
 * The frequency of the timer in Hz.
 */
const TInt KHighResTimerFrequency = 1000000;

/**
 * Macro indicating that the timer counts up if defined.
 */
#define HIGH_RES_TIMER_COUNTS_UP

#endif  // __HIGHRESTIMER_H__
