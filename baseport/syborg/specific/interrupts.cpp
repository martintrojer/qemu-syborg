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
* Description: Syborg interrupt control and dispatch
*
*/

#include <syborg_priv.h>

SInterruptHandler SyborgInterrupt::Handlers[KNumSyborgInts];

void SyborgInterrupt::DisableAndClearAll()
{
  WriteReg(KHwBaseSic, 3, 0);
}

void SyborgInterrupt::Init1()
{
	__KTRACE_OPT(KBOOT,Kern::Printf("SyborgInterrupt::Init1()"));

	for(TUint i = 0; i < KNumSyborgInts; i++)
	{
		Handlers[i].iPtr = (TAny*)i;
		Handlers[i].iIsr = Spurious;
	}

	DisableAndClearAll();
	
	Arm::SetIrqHandler((TLinAddr)SyborgInterrupt::IrqDispatch);
	Arm::SetFiqHandler((TLinAddr)SyborgInterrupt::FiqDispatch);
}

void SyborgInterrupt::Init3()
{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("SyborgInterrupt::Init3()"));
}

void SyborgInterrupt::Spurious(TAny* anId)
{
	// Handle an unexpected interrupt
	Kern::Fault("SpuriousInt", (TInt)anId);
}

EXPORT_C TInt Interrupt::Bind(TInt anId, TIsr anIsr, TAny* aPtr)
{
  __KTRACE_OPT(KHARDWARE,Kern::Printf("Interrupt::Bind(anId=%d anIsr=0x%X aPtr=0x%X)",anId,anIsr,aPtr));
	if((anId >= 0) && ((TUint)anId < KNumSyborgInts))
	{
		SInterruptHandler& h = SyborgInterrupt::Handlers[anId];
		TInt irq = NKern::DisableAllInterrupts();
        TInt r;
		if(h.iIsr != SyborgInterrupt::Spurious)
		{
            r = KErrInUse;
	    }
		else
		{
    	    h.iPtr = aPtr;
	    	h.iIsr = anIsr;
            r = KErrNone;
		}
		NKern::RestoreInterrupts(irq);
		return r;
	}
	return KErrArgument;
}

EXPORT_C TInt Interrupt::Unbind(TInt anId)
{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("Interrupt::Unbind(%d)",anId));
	if((anId >= 0) && ((TUint)anId < KNumSyborgInts))
	{
		SInterruptHandler& h = SyborgInterrupt::Handlers[anId];
		TInt irq = NKern::DisableAllInterrupts();
    	TInt r;
		if(h.iIsr == SyborgInterrupt::Spurious)
		{
			r = KErrGeneral;
		}
		else
		{
            // Reset pointer to handler back to default
			h.iPtr = (TAny*)anId;
			h.iIsr = SyborgInterrupt::Spurious;	
			// Disable the interrupt
			TSyborg::DisableInt(anId);
			r = KErrNone;
        }
		NKern::RestoreInterrupts(irq);
    	return r;
	}
	return KErrArgument;
}

EXPORT_C TInt Interrupt::Enable(TInt anId)
{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("Interrupt::Enable(%d)",anId));
	if((anId >= 0) && ((TUint)anId < KNumSyborgInts))
	{
		if(SyborgInterrupt::Handlers[anId].iIsr == SyborgInterrupt::Spurious)
		{
	        return KErrNotReady;
		}
		else
		{
		  TSyborg::EnableInt(anId);
		  return KErrNone;
		}
	}
	return KErrArgument;
}

EXPORT_C TInt Interrupt::Disable(TInt anId)
{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("Interrupt::Disable(%d)",anId));
	if((anId >= 0) && ((TUint)anId < KNumSyborgInts))
	{
	  TSyborg::DisableInt(anId);
	  return KErrNone;
	}
	return KErrArgument;
}

EXPORT_C TInt Interrupt::Clear(TInt anId)
{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("Interrupt::Clear(%d)",anId));
	if((anId >= 0) && ((TUint)anId < KNumSyborgInts))
	{
        return KErrNone;
	}
	return KErrArgument;
}

EXPORT_C TInt Interrupt::SetPriority(TInt anId, TInt aPriority)
{
	__KTRACE_OPT(KHARDWARE,Kern::Printf("Interrupt::SetPriority(anId=%d aPriority=0x%X)",anId,aPriority));
	return KErrNotSupported;
}

