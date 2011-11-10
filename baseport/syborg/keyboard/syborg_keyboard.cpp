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
* Description: Minimalistic keyboard driver
*
*/

//#define DEBUG

#include "syborg_keyboard.h"

LOCAL_C TInt halFunction(TAny* aPtr, TInt aFunction, TAny* a1, TAny* a2)
{
  DKeyboardPs2Soc* pH=(DKeyboardPs2Soc*)aPtr;
  return pH->HalFunction(aFunction,a1,a2);
}

void rxMsg(TAny* aPtr)
{
  DKeyboardPs2Soc& h=*(DKeyboardPs2Soc*)aPtr;
  TMessageBase* pM=h.iMsgQ.iMessage;
  if (pM)
	h.HandleMsg(pM);
}

TInt DKeyboardPs2Soc::FifoPop(void)
{
  TInt val = iKeyFifo[iFifoPos];
  iFifoPos++;
  iFifoCount--;
  
  if (iFifoPos == FIFO_SIZE)
	iFifoPos = 0;

  return val;
}

void DKeyboardPs2Soc::FifoPush(TInt val)
{
  TInt slot;

  if (iFifoCount == FIFO_SIZE)
	return;
  
  slot = iFifoPos + iFifoCount;
  if (slot >= FIFO_SIZE)
	slot -= FIFO_SIZE;
  iKeyFifo[slot] = val;
  iFifoCount++;
}

void DKeyboardPs2Soc::Isr(TAny* aPtr)
{
  __DEBUG_PRINT("DKeyboardPs2Soc::Isr");
  DKeyboardPs2Soc& k = *(DKeyboardPs2Soc*)aPtr;

  // Is now auto-clearing
  while(ReadReg(KHwBaseKmiKeyboard, KBD_FIFO_COUNT)!=0)
	k.FifoPush(ReadReg(KHwBaseKmiKeyboard, KBD_DATA));
  
  //WriteReg(KHwBaseKmiKeyboard,KBD_CLEAR_INT, 0);
  Interrupt::Clear(EIntKeyboard); 
  k.iRxDfc.Add();
}

DKeyboardPs2Soc::DKeyboardPs2Soc()
  :	DPowerHandler(KLitKeyboard),
	iRxDfc(RxDfc,this,Kern::DfcQue0(),1),
	iMsgQ(rxMsg,this,NULL,1)
{
  iKeyboardOn = ETrue;
  iFifoPos = 0;
  iFifoCount = 0;
}

TInt DKeyboardPs2Soc::Create()
{
  __DEBUG_PRINT("DKeyboardPs2Soc::Create");

  TInt r=KErrNone;
  iDfcQ=Kern::DfcQue0();

  iFifoPos = iFifoCount = 0;

  r=Kern::AddHalEntry(EHalGroupKeyboard,halFunction,this);
  if (r!=KErrNone)
	return r;

  iMsgQ.SetDfcQ(iDfcQ);
  iMsgQ.Receive();

  r=Interrupt::Bind(EIntKeyboard,Isr,this);
  if (r==KErrNone) {
	Add();
	KeyboardOn();
  }
  return r;
}

void DKeyboardPs2Soc::PowerUp()
{
  PowerUpDone();
}

void DKeyboardPs2Soc::PowerDown(TPowerState)
{
  PowerDownDone();
}

void DKeyboardPs2Soc::KeyboardOn()
{
  __DEBUG_PRINT("DKeyboardPs2Soc::KeyboardOn");
  TInt reg = ReadReg(KHwBaseKmiKeyboard,KBD_ID);
  
  Interrupt::Enable(EIntKeyboard);
  WriteReg(KHwBaseKmiKeyboard,KBD_INT_ENABLE,1);
}

void DKeyboardPs2Soc::KeyboardOff()
{
  __DEBUG_PRINT("DKeyboardPs2Soc::KeyboardOff");
  Interrupt::Disable(EIntKeyboard);
  WriteReg(KHwBaseKmiKeyboard,KBD_INT_ENABLE,0);
}

void DKeyboardPs2Soc::HandleMsg(TMessageBase* aMsg)
{
  __DEBUG_PRINT("DKeyboardPs2Soc::HandleMsg");
  if (aMsg->iValue)
	KeyboardOn();
  else
	KeyboardOff();
  aMsg->Complete(KErrNone,ETrue);
}

void DKeyboardPs2Soc::KeyboardInfo(TKeyboardInfoV01& aInfo)
{
  aInfo.iKeyboardType=KConfigKeyboardType;
  aInfo.iDeviceKeys=KConfigKeyboardDeviceKeys;
  aInfo.iAppsKeys=KConfigKeyboardAppsKeys;
}

TInt DKeyboardPs2Soc::HalFunction(TInt aFunction, TAny* a1, TAny* a2)
{
  TInt r=KErrNone;

  __DEBUG_PRINT("DKeyboardPs2Soc::HalFunction");
  switch(aFunction)
	{
	case EKeyboardHalKeyboardInfo:
	  {
		TPckgBuf<TKeyboardInfoV01> kPckg;
		KeyboardInfo(kPckg());
		Kern::InfoCopy(*(TDes8*)a1,kPckg);
		break;
	  }
	  // UIKLAF Silent running/power management
	case EKeyboardHalSetKeyboardState:
	  {
		if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EKeyboardHalSetKeyboardState")))
		  return KErrPermissionDenied;
		if ((TBool)a1)
		  {
			TThreadMessage& m=Kern::Message();
			m.iValue = ETrue;
			m.SendReceive(&iMsgQ);
		  }
		else
		  {
			TThreadMessage& m=Kern::Message();
			m.iValue = EFalse;
			m.SendReceive(&iMsgQ);
		  }
	  }
	  break;
	case EKeyboardHalKeyboardState:
	  kumemput32(a1, &iKeyboardOn, sizeof(TBool));
	  break;
	default:
	  r=KErrNotSupported;
	  break;
	}
  return r;
}

void DKeyboardPs2Soc::RxDfc(TAny* aPtr)
{
  TRawEvent e;
  DKeyboardPs2Soc& k = *(DKeyboardPs2Soc*)aPtr;

  while(k.iFifoCount>0) {
	int keycode = k.FifoPop();
	int dwn = (keycode & 0x80000000) ? 0 : 1;
	__DEBUG_PRINT("DKeyboardPs2Soc::RxDfc %d %d", keycode, dwn);
  
	keycode &= ~(0x80000000);	
	if (dwn) {
	  __DEBUG_PRINT("kbd EKeyDown:%d",keycode);
	  e.Set(TRawEvent::EKeyDown,KConvertCode[keycode],0);
	}
	else {
	  __DEBUG_PRINT("kbd EKeyUp:%d",keycode);
	  e.Set(TRawEvent::EKeyUp,KConvertCode[keycode],0);
	}
	Kern::AddEvent(e);
  }
}

DECLARE_STANDARD_EXTENSION()
{
  TInt r=KErrNoMemory;
  DKeyboardPs2Soc* pK=new DKeyboardPs2Soc;
  if (pK)
	r=pK->Create();
  __KTRACE_OPT(KEXTENSION,__DEBUG_PRINT("Returns %d",r));
  return r;
}
