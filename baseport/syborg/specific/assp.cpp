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
* NTT DOCOMO, INC - Fix for bug 1292 "E32test t_mstim.exe failed to load test LDD"
*
* Description: implementation of class Syborg
*
*/

#include <syborg_priv.h>
#include <hal_data.h>

#ifdef __EMI_SUPPORT__
#include <emi.h>
#endif

//#define ASSP_DEBUG

#ifdef ASSP_DEBUG
#define __DEBUG_PRINT(format...)    Kern::Printf(format)
#else
#define __DEBUG_PRINT(format...)    __KTRACE_OPT(KBOOT,Kern::Printf(format))
#endif

Syborg* Syborg::Variant=NULL;
// !@!
#if 0
TPhysAddr Syborg::VideoRamPhys;
TPhysAddr Syborg::VideoRamPhysSecure;		// Secure display memory
#endif

GLDEF_D Syborg TheVariant;

// Wait for interrupt idle routine
extern TInt SyborgWFIIdle();

DECLARE_STANDARD_ASSP()

EXPORT_C Asic* VariantInitialise()
{
	return &TheVariant;
}

void Syborg::Idle()
{
	// Use the basic Wait For Interrupt call to idle
	TInt irq = NKern::DisableAllInterrupts();
#ifdef __EMI_SUPPORT__
	EMI::EnterIdle();
#endif
	SyborgWFIIdle();
#ifdef __EMI_SUPPORT__
	EMI::LeaveIdle();
#endif
	NKern::RestoreInterrupts(irq);
}

TUint32 Syborg::NanoWaitCalibration()
{
	return 17;	// 2 cycles approx 17ns at 125MHz
}

TInt Syborg::VariantHal(TInt aFunction, TAny* a1, TAny* a2)
{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("Syborg::VariantHal(%d, %0x, %0x)",aFunction,a1,a2));

	switch(aFunction)
    {
		case EVariantHalVariantInfo:
		{
			TVariantInfoV01Buf infoBuf;
			TVariantInfoV01& info = infoBuf();
			TUint clock=0;
			info.iRomVersion = Epoc::RomHeader().iVersion;
// NTT Docomo - Defect 1292 fix - E32test t_mstim.exe failed to load test LDD - start
			const TInt KMachineUid_Syborg = 0x10005152;
    		info.iMachineUniqueId = (TInt64(KMachineUid_Syborg)<<32);
// NTT Docomo - Defect 1292 fix - E32test t_mstim.exe failed to load test LDD - end
   			info.iLedCapabilities = (8<<16) + KLedMaskGreen1;
			info.iProcessorClockInKHz = clock;
			info.iSpeedFactor = clock/25;
			Kern::InfoCopy(*(TDes8*)a1,infoBuf);
			break;
	    }
		case EVariantHalDebugPortSet:
	    {
			TUint32 thePort = (TUint32)a1;
			switch(thePort) // Accept UART(0-3)
		    {
				case 0:
				case 1:
			    case 2:
                case 3:
                {
                    TSyborg::MarkDebugPortOff();           // mark port is not initialised
					Kern::SuperPage().iDebugPort = thePort; // update the super page
                    Syborg::DebugInit();                   // init debug port        
                    break;
                }
				case (TUint32)KNullDebugPort:               // debug output supressed
                {
                    TSyborg::MarkDebugPortOff();           // mark port is not initialised
					Kern::SuperPage().iDebugPort = thePort; // update the super page
					break;
                }
				default:
					return KErrNotSupported;
		    }
			break;
	    }
		case EVariantHalDebugPortGet:
	    {
			TUint32 thePort = Kern::SuperPage().iDebugPort;
			kumemput32(a1, &thePort, sizeof(TUint32));
			break;
	    }
		case EVariantHalSwitches:
	    {
		  TUint32 x = 0; //Register32(KHwBaseSystemReg+KHoRoSystemSw);
			kumemput32(a1, &x, sizeof(x));
			break;
	    }
		case EVariantHalLedMaskSet:
	    {
		  //SetRegister32(KHwBaseSystemReg+KHoRwSystemLed, (TUint32)a1 & 0xFF);
			break;
	    }
		case EVariantHalLedMaskGet:
	    {
		  TUint32 x = 0; //Register32(KHwBaseSystemReg+KHoRwSystemLed);
			kumemput32(a1, &x, sizeof(x));
			break;
	    }
		case EVariantHalCustomRestartReason:
	    {
			// Restart reason is stored in super page
		  TInt x = (Kern::SuperPage().iHwStartupReason); // & KmRestartCustomReasons) >> KsRestartCustomReasons;
			kumemput32(a1, &x, sizeof(TInt));
			break;
	    }
		case EVariantHalCustomRestart:
	    {
            __KERNEL_CAPABILITY_CHECK(
                if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,
			       __PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EVariantHalCustomRestart")))
				    return KErrPermissionDenied;
            )
			  Kern::Restart(0);
			break;
	    }
		default:
    	{
			return KErrNotSupported;
	    }
    }
	return KErrNone;
}

TPtr8 Syborg::MachineConfiguration()
{
	return TPtr8((TUint8*)&Kern::MachineConfig(),40,40);
}

EXPORT_C Syborg::Syborg()
{
	iDebugPortBase = TSyborg::DebugPortAddr(); // initialised in bootstrap
}

EXPORT_C TMachineStartupType Syborg::StartupReason()
{
	TUint s = Kern::SuperPage().iHwStartupReason;
	__DEBUG_PRINT("Syborg::StartupReason CPU page value 0x%08X", s);
	return EStartupColdReset;
}

EXPORT_C void Syborg::Init1()
{
	__DEBUG_PRINT("Syborg::Init1()");

	// Enable the CLCD in the System registers
	SyborgInterrupt::Init1();
}

EXPORT_C void Syborg::Init3()
{
	NTimerQ& m = *(NTimerQ*)NTimerQ::TimerAddress();
	m.iRounding = -5;
		
	TInt r = Interrupt::Bind(EIntTimer1,SyborgInterrupt::MsTimerTick,&m);
	if (r != KErrNone)
	{
		Kern::Fault("BindMsTick",r);
	}

	TSyborg::Init3();
	TSyborg::ClearTimerInt(KHwBaseCounterTimer);

	r = Interrupt::Enable(EIntTimer1);
	if (r != KErrNone)
	{
		Kern::Fault("EnbMsTick",r);
	}

	TSyborg::SetTimerLoad(KHwBaseCounterTimer, K1000HzTickMatchLoad);
	//	TSyborg::SetTimerLoad(KHwBaseCounterTimer, 1000000);
	TSyborg::SetTimerMode(KHwBaseCounterTimer, TSyborg::ETimerModePeriodic);	
	TSyborg::EnableTimerInterrupt(KHwBaseCounterTimer);
	TSyborg::EnableTimer(KHwBaseCounterTimer, TSyborg::EEnable);
	
	SyborgInterrupt::Init3();

	// !@!
#if 0
	// Allocate physical RAM for video
	TInt vSize = TSyborg::VideoRamSize();

	r = Epoc::AllocPhysicalRam(vSize,Syborg::VideoRamPhys);
	if (r != KErrNone)
	{
		Kern::Fault("AllocVideoRam",r);
	}

	// Allocate physical RAM for secure display
	r = Epoc::AllocPhysicalRam(vSize,Syborg::VideoRamPhysSecure);
	if (r != KErrNone)
	{
		Kern::Fault("AllocVideoRam 2",r);
	}
#endif
	__DEBUG_PRINT("Finished Syborg::Init3()");
}

EXPORT_C void Syborg::DebugInit()
{
	iDebugPortBase = TSyborg::DebugPortAddr();

    if(iDebugPortBase != (TUint32)KNullDebugPort)   // supress debug output
    {
	  TUint8 res = ReadReg(iDebugPortBase, 0);
    }
}

//
// Output a character to the debug port
//
EXPORT_C void Syborg::DebugOutput(TUint aLetter)
{
	if(!iDebugPortBase)
    {
		DebugInit();
	}

    if(iDebugPortBase != (TUint32)KNullDebugPort)   // supress debug output
    {
	  WriteReg(iDebugPortBase,1,aLetter);
    }
}

EXPORT_C TInt Syborg::MsTickPeriod()
{
	return KMsTickPeriod;
}

EXPORT_C TInt Syborg::SystemTimeInSecondsFrom2000(TInt& aTime)
{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("RTC READ: %d",aTime));
	return KErrNone;
}

EXPORT_C TInt Syborg::SetSystemTimeInSecondsFrom2000(TInt aTime)
{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("Set RTC: %d",aTime));					// do this here to allow the value to be loaded...
	return KErrNone;
}
