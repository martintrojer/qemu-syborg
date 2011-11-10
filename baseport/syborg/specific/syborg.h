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
* NTT DOCOMO, INC : BUG 1296
* NTT DOCOMO, INC : BUG 3598
* NTT DOCOMO, INC - Fix for bug 1291 "E32test t_tock.exe failed to load Logical Device"
*
* Description:
*
*/

#ifndef __SYBORG_H__
#define __SYBORG_H__

#include <e32const.h>
#include <platform.h>
#include <e32hal.h>
#include <assp.h>
#include <kern_priv.h>
#include <mmboot.h>     // KPrimaryIOBase

#ifndef __SYBORG__
#define __SYBORG__
#endif

const TUint KHwBasePeripherals  = KPrimaryIOBase;       
const TUint KHwLinSeparation	= 0x1000;
const TInt KMsTickPeriod	= 10000;

const TUint KHwBaseSic				= KHwBasePeripherals + 0x00*KHwLinSeparation;
// intended for use as a free-running counter. Reading the value register of a free-running syborg counter returns a microsec value
const TUint KHwBaseRtc				= KHwBasePeripherals + 0x01*KHwLinSeparation;
// intended for use as an alarm generating timer with a resolution of 1 microsecond
const TUint KHwBaseCounterTimer			= KHwBasePeripherals + 0x02*KHwLinSeparation;
const TUint KHwBaseKmiKeyboard			= KHwBasePeripherals + 0x03*KHwLinSeparation;
const TUint KHwBaseKmiMouse			= KHwBasePeripherals + 0x04*KHwLinSeparation;
const TUint KHwBaseKmiPointer			= KHwBasePeripherals + 0x04*KHwLinSeparation;
const TUint KHwBaseClcd				= KHwBasePeripherals + 0x05*KHwLinSeparation;
const TUint KHwBaseUart0			= KHwBasePeripherals + 0x06*KHwLinSeparation;
const TUint KHwBaseUart1			= KHwBasePeripherals + 0x07*KHwLinSeparation;
const TUint KHwBaseUart2			= KHwBasePeripherals + 0x08*KHwLinSeparation;
const TUint KHwBaseUart3			= KHwBasePeripherals + 0x09*KHwLinSeparation;
const TUint KHwSVPHostFileSystemDevice		= KHwBasePeripherals + 0x0a*KHwLinSeparation;
const TUint KHwSVPSnapDevice			= KHwBasePeripherals + 0x0b*KHwLinSeparation;
const TUint KHwSVPNetDevice			= KHwBasePeripherals + 0x0c*KHwLinSeparation;
const TUint KHwSVPNandDevice			= KHwBasePeripherals + 0x0d*KHwLinSeparation;
const TUint KHwSVPAudioDevice			= KHwBasePeripherals + 0x0e*KHwLinSeparation;
const TUint KHwSVPWebcameraDevice		= KHwBasePeripherals + 0x0f*KHwLinSeparation;
const TUint KHwNVMemoryDevice			= KHwBasePeripherals + 0x10*KHwLinSeparation;
// NTT Docomo - Defect 1291 fix - E32test t_tock.exe failed to load Logical Device - start
const TUint KHwBaseCounterTimer2		= KHwBasePeripherals + 0x11*KHwLinSeparation;
const TUint KHwSVPPlatformDevice		= KHwBasePeripherals + 0x12*KHwLinSeparation;
// NTT Docomo - Defect 1291 fix - E32test t_tock.exe failed to load Logical Device - end
	
enum TSyborgInterruptId
{
  EIntTimer0 = 0,     /* RTC -- not used */
  EIntTimer1 = 1,     /* Interval Timer */
  EIntKeyboard = 2,
    EIntPointer = 3,
  //  EIntMouse = 3,
  EIntFb = 4,
  EIntSerial0 = 5,
  EIntSerial1 = 6,
  EIntSerial2 = 7,
  EIntSerial3 = 8,
  EIntNet0 = 9,
  EIntAudio0 = 10,
  EIntNVMemoryDevice = 12,
// NTT Docomo - Defect 1291 fix - E32test t_tock.exe failed to load Logical Device - start
  EIntTimer2 = 14
// NTT Docomo - Defect 1291 fix - E32test t_tock.exe failed to load Logical Device - end
};

// Timer Mode
const TUint KPeriodic			= 0x0;
const TUint KOneShot		    = 0x1;

class TSyborg
{
public:
	// generic enums
	enum TState
	{
		EEnable,
		EDisable
	};
	enum TLock
	{
		ELocked=0,
		EUnlocked=0xA05F
	};
    enum TTimerMode
	{
	  ETimerModePeriodic=KPeriodic,
	  ETimerModeOneShot=KOneShot,
	};
public:
    // Initialisation of class
	static void Init3();

    // Accessor methods for timers 
	IMPORT_C static void EnableTimerInterrupt(TUint aTimerBase);                    // Enable the timer interrupt
	IMPORT_C static void DisableTimerInterrupt(TUint aTimerBase);                   // Disable the timer interrupt
    IMPORT_C static void SetTimerLoad(TUint aTimerBase, TUint32 aValue);			// Set the starting count value for a timer
    IMPORT_C static TUint TimerLoad(TUint aTimerBase);							    // Read the load register (starting count value)
    IMPORT_C static TUint TimerValue(TUint aTimerBase);							    // Read the actual timer value
    IMPORT_C static void ClearTimerInt(TUint aTimerBase); 						    // Clear the timer interrupt
    IMPORT_C static TBool IsTimerEnabled(TUint aTimerBase);						    // Enquire as to whether the timer is enabled
    IMPORT_C static void EnableTimer(TUint aTimerBase, TState aState);			    // Enable/disable the timer (start/stop it running)
    IMPORT_C static TTimerMode TimerMode(TUint aTimerBase);						    // Find out what mode the timer is running in
    IMPORT_C static void SetTimerMode(TUint aTimerBase, TTimerMode aValue);		    // Set the timer mode (periodic or free running)
	
    // Accessor methods for interrupts
    IMPORT_C static void  EnableInt(TUint anId);					
    IMPORT_C static void  DisableInt(TUint anId);					
        
	IMPORT_C static TInt VideoRamSize();
	IMPORT_C static TPhysAddr VideoRamPhys();
	IMPORT_C static TPhysAddr VideoRamPhysSecure();

	// Debug Port Specific
	IMPORT_C static TUint32 DebugPortAddr();
	IMPORT_C static void MarkDebugPortOff();
};

static inline TUint32 ReadReg(TUint32 base, TUint32 aReg)
{
  return *(volatile TUint32 *)(base + (aReg << 2));
}

static inline void WriteReg(TUint32 base, TUint32 aReg, TUint32 aVal)
{
  *(volatile TUint32*)(base + (aReg<<2)) = aVal;
}

#endif  // __SYBORG_H__
