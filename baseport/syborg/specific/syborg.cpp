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
*
* Description:
*
*/

#include <syborg_priv.h>

//#define DEBUG

#ifdef DEBUG
#define __DEBUG_PRINT(format...)    Kern::Printf(format)
#else
#define __DEBUG_PRINT(format...)    __KTRACE_OPT(KBOOT,Kern::Printf(format))
#endif

void TSyborg::Init3()
{
//  SetTimerMode(KHwBaseCounterTimer, ETimerModePeriodic);
//  EnableTimer(KHwBaseCounterTimer, EEnable);
}

EXPORT_C TBool TSyborg::IsTimerEnabled(TUint aTimerBase)
{
  __DEBUG_PRINT("TSyborg::IsTimerEnabled");
  return ReadReg(aTimerBase, 1);
}

EXPORT_C void TSyborg::EnableTimerInterrupt(TUint aTimerBase)
{
  __DEBUG_PRINT("TSyborg::EnableTimerInterrupt");
  WriteReg(aTimerBase,5,1);
}

EXPORT_C void TSyborg::DisableTimerInterrupt(TUint aTimerBase)
{
  __DEBUG_PRINT("TSyborg::DisableTimerInterrupt");
  WriteReg(aTimerBase,5,0);
}

EXPORT_C void TSyborg::EnableTimer(TUint aTimerBase, TState aState)
{
  __DEBUG_PRINT("TSyborg::EnableTimer");
  //  TUint32 mode = ReadReg(aTimerBase, 1);
  if (aState == EEnable)
	WriteReg(aTimerBase, 1, 1);
  else
	WriteReg(aTimerBase, 1, 0);
}

EXPORT_C TSyborg::TTimerMode TSyborg::TimerMode(TUint aTimerBase)
{
  __DEBUG_PRINT("TSyborg::TimerMode");
  if (ReadReg(aTimerBase, 2))
	return ETimerModeOneShot;
  else
	return ETimerModePeriodic;
}

EXPORT_C void TSyborg::SetTimerMode(TUint aTimerBase, TTimerMode aValue)
{
  __DEBUG_PRINT("TSyborg::SetTimerMode");
  if (aValue == ETimerModePeriodic)
	WriteReg(aTimerBase, 2, 0);
  else
	WriteReg(aTimerBase, 2, 1);
}

// Return base address of debug UART
// (as selected in obey file or with eshell debugport command)
EXPORT_C TUint32 TSyborg::DebugPortAddr()
{
	// Defaults to UART 0
    switch (Kern::SuperPage().iDebugPort)
    {
        case KNullDebugPort:        // debug supressed
            return (TUint32)KNullDebugPort;
        case 1:
            return KHwBaseUart1;
        case 2:
            return KHwBaseUart2;
        case 3:
            return KHwBaseUart3;
        case 0:
        default:
            return KHwBaseUart0;
    }
}

EXPORT_C void TSyborg::MarkDebugPortOff()
{
	TheVariant.iDebugPortBase = 0;
}

EXPORT_C TInt TSyborg::VideoRamSize()
{

  return 4*854*854; // Now allow for 854 x 854 display, instead of 480 x 640

}

// !@!
EXPORT_C TPhysAddr TSyborg::VideoRamPhys()
{
#if 0
  __KTRACE_OPT(KEXTENSION,Kern::Printf("TSyborg::VideoRamPhys: VideoRamPhys=0x%x", Syborg::VideoRamPhys));
#endif
  return 0;

}

EXPORT_C TPhysAddr TSyborg::VideoRamPhysSecure()
{
#if 0
  return Syborg::VideoRamPhysSecure;
#endif
  return 0;
}

