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

#include <monitor.h>
#include <system_priv.h>

#include "syborg_serial.h"

void CrashDebugger::InitUart()
{
  TUint32 debugPortBase = VARIANT_INTERFACE::DebugPortAddr();
  TUint8 res = ReadReg(debugPortBase, 0);
  WriteReg(debugPortBase,DCommSyborgSoc::SERIAL_INT_ENABLE,0);
}

void CrashDebugger::UartOut(TUint aChar)
{
  TUint32 debugPortBase = VARIANT_INTERFACE::DebugPortAddr();
  WriteReg(debugPortBase,DCommSyborgSoc::SERIAL_DATA,aChar);
}

TUint8 CrashDebugger::UartIn()
{
  TUint32 debugPortBase = VARIANT_INTERFACE::DebugPortAddr();
  while (ReadReg(debugPortBase,DCommSyborgSoc::SERIAL_FIFO_COUNT)==0)
	;

  return ReadReg(debugPortBase,DCommSyborgSoc::SERIAL_DATA);
}

TBool CrashDebugger::CheckPower()
{
	return EFalse;
}
