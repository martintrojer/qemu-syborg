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

#ifndef _SYBORG_KEYBOARD_H
#define _SYBORG_KEYBOARD_H

#include <kpower.h>
#include <e32keys.h>
#include <system.h>

#ifdef DEBUG
#define __DEBUG_PRINT(format...)    Kern::Printf(format)
#else
#define __DEBUG_PRINT(format...)    __KTRACE_OPT(KBOOT,Kern::Printf(format))
#endif

#define FIFO_SIZE 16

_LIT(KLitKeyboard,"Syborg Keyboard");
const TKeyboard	KConfigKeyboardType = EKeyboard_Full;
const TInt KConfigKeyboardDeviceKeys = 0;
const TInt KConfigKeyboardAppsKeys = 0;

class DKeyboardPs2Soc : public DPowerHandler
{
public:
  DKeyboardPs2Soc();
  TInt Create();
  TInt HalFunction(TInt aFunction, TAny* a1, TAny* a2);
  void KeyboardInfo(TKeyboardInfoV01& aInfo);
  void KeyboardOn();
  void KeyboardOff();
  void PowerUp();
  void PowerDown(TPowerState);
  void HandleMsg(TMessageBase* aMsg);

private:
  static void Isr(TAny* aPtr);
  static void RxDfc(TAny* aPtr);

 private:
  TInt FifoPop(void);
  void FifoPush(TInt val);

  TInt iKeyFifo[FIFO_SIZE];
  TInt iFifoPos;
  TInt iFifoCount;

private:
  TDfc iRxDfc;
  TBool iKeyboardOn;

public:
  TDfcQue* iDfcQ;
  TMessageQue iMsgQ;	

  enum {
    KBD_ID          = 0,
    KBD_DATA        = 1,
    KBD_FIFO_COUNT  = 2,
    KBD_INT_ENABLE  = 3
  };
};

const TUint8 KConvertCode[] =
  {
	/*00*/ EStdKeyNull,
	/*01*/ EStdKeyEscape,
	/*02*/ '1',
	/*03*/ '2',
	/*04*/ '3',
	/*05*/ '4',
	/*06*/ '5',
	/*07*/ '6',
	/*08*/ '7',
	/*09*/ '8',
	/*0a*/ '9',
	/*0b*/ '0',
	/*0c*/ EStdKeyMinus,
	/*0d*/ EStdKeyEquals, 
	/*0e*/ EStdKeyBackspace,
	/*0f*/ EStdKeyTab,
	
	/*10*/ 'Q',
	/*11*/ 'W',
	/*12*/ 'E',
	/*13*/ 'R',
	/*14*/ 'T',
	/*15*/ 'Y',
	/*16*/ 'U',
	/*17*/ 'I',
	/*18*/ 'O',
	/*19*/ 'P',
	/*1a*/ EStdKeySquareBracketLeft,
	/*1b*/ EStdKeySquareBracketRight,
	/*1c*/ EStdKeyEnter,
	/*1d*/ EStdKeyLeftCtrl,
	/*1e*/ 'A',
	/*1f*/ 'S',
	
	/*20*/ 'D',
	/*21*/ 'F',
	/*22*/ 'G',
	/*23*/ 'H',
	/*24*/ 'J',
	/*25*/ 'K',
	/*26*/ 'L',
	/*27*/ EStdKeySemiColon,
	/*28*/ EStdKeySingleQuote,
	/*29*/ EStdKeyNull,
	/*2a*/ EStdKeyLeftShift,
	/*2b*/ EStdKeyHash,
	/*2c*/ 'Z',
	/*2d*/ 'X',
	/*2e*/ 'C',
	/*2f*/ 'V',
	
	/*30*/ 'B',
	/*31*/ 'N',
	/*32*/ 'M',
	/*33*/ EStdKeyComma,
	/*34*/ EStdKeyFullStop,
	/*35*/ EStdKeyForwardSlash,
	/*36*/ EStdKeyRightShift,
	/*37*/ EStdKeyPrintScreen,
	/*38*/ EStdKeyLeftAlt,
	/*39*/ EStdKeySpace,
	/*3a*/ EStdKeyCapsLock,
	/*3b*/ EStdKeyMenu,    // EStdKeyF1,
	/*3c*/ EStdKeyF2,
	/*3d*/ EStdKeyF3,
	/*3e*/ EStdKeyF4,

	/*3f*/ EStdKeyDevice0,	//EStdKeyF5,	SF: Left soft-key [EStdKeyApplication0]

	/*40*/ EStdKeyDevice3,	//EStdKeyF6,	SF: OK key
	/*41*/ EStdKeyDevice1,	//EStdKeyF7,	SF: Right soft-key
	/*42*/ EStdKeyApplication0, //EStdKeyF8, SF Menu key

	/*43*/ EStdKeyF9,
	/*44*/ EStdKeyF10,
	/*45*/ EStdKeyNull,
	/*46*/ EStdKeyScrollLock,
	/*47*/ EStdKeyHome,
	/*48*/ EStdKeyUpArrow,
	/*49*/ EStdKeyPageUp,
	/*4a*/ EStdKeyNull,
	/*4b*/ EStdKeyLeftArrow,
	/*4c*/ EStdKeyNull,
	/*4d*/ EStdKeyRightArrow,
	/*4e*/ EStdKeyNull,

	/*4f*/ EStdKeyEnd,

	
	/*50*/ EStdKeyDownArrow,
	/*51*/ EStdKeyPageDown,
	/*52*/ EStdKeyInsert,
	/*53*/ EStdKeyDelete,
	/*54*/ EStdKeyNull,
	/*55*/ EStdKeyNull,
	/*56*/ EStdKeyBackSlash,
	/*57*/ EStdKeyF11,
	/*58*/ EStdKeyF12,
	/*59*/ EStdKeyNull,
	/*5a*/ EStdKeyNull,
	/*5b*/ EStdKeyNull,
	/*5c*/ EStdKeyNull,
	/*5d*/ EStdKeyNull,
	/*5e*/ EStdKeyNull,
	/*5f*/ EStdKeyNull,

	/*60*/ EStdKeyNull,
	/*61*/ EStdKeyNull,
	/*62*/ EStdKeyNull,
	/*63*/ EStdKeyNull,
	/*64*/ EStdKeyNull,
	/*65*/ EStdKeyNull,
	/*66*/ EStdKeyNull,
	/*67*/ EStdKeyNull,
	/*68*/ EStdKeyNull,
	/*69*/ EStdKeyNull,
	/*6a*/ EStdKeyNull,
	/*6b*/ EStdKeyNull,
	/*6c*/ EStdKeyNull,
	/*6d*/ EStdKeyNull,
	/*6e*/ EStdKeyNull,
	/*6f*/ EStdKeyNull,
	
	/*70*/ EStdKeyNull,
	/*71*/ EStdKeyNull,
	/*72*/ EStdKeyNull,
	/*73*/ EStdKeyNull,
	/*74*/ EStdKeyNull,
	/*75*/ EStdKeyNull,
	/*76*/ EStdKeyNull,
	/*77*/ EStdKeyNull,
	/*78*/ EStdKeyNull,
	/*79*/ EStdKeyNull,
	/*7a*/ EStdKeyNull,
	/*7b*/ EStdKeyNull,
	/*7c*/ EStdKeyNull,
	/*7d*/ EStdKeyNull,
	/*7e*/ EStdKeyNull,
	/*7f*/ EStdKeyNull,
	
	/*80*/ EStdKeyNull,
	/*81*/ EStdKeyNull,
	/*82*/ EStdKeyNull,
	/*83*/ EStdKeyNull,
	/*84*/ EStdKeyNull,
	/*85*/ EStdKeyNull,
	/*86*/ EStdKeyNull,
	/*87*/ EStdKeyNull,
	/*88*/ EStdKeyNull,
	/*89*/ EStdKeyNull,
	/*8a*/ EStdKeyNull,
	/*8b*/ EStdKeyNull,
	/*8c*/ EStdKeyNull,
	/*8d*/ EStdKeyNull,
	/*8e*/ EStdKeyNull,
	/*8f*/ EStdKeyNull,
	
	/*90*/ EStdKeyNull,
	/*91*/ EStdKeyNull,
	/*92*/ EStdKeyNull,
	/*93*/ EStdKeyNull,
	/*94*/ EStdKeyNull,
	/*95*/ EStdKeyNull,
	/*96*/ EStdKeyNull,
	/*97*/ EStdKeyNull,
	/*98*/ EStdKeyNull,
	/*99*/ EStdKeyNull,
	/*9a*/ EStdKeyNull,
	/*9b*/ EStdKeyNull,
	/*9c*/ EStdKeyNull,
	/*9d*/ EStdKeyNull,
	/*9e*/ EStdKeyNull,
	/*9f*/ EStdKeyNull,

  	/*a0*/ EStdKeyNull,
	/*a1*/ EStdKeyNull,
	/*a2*/ EStdKeyNull,
	/*a3*/ EStdKeyNull,
	/*a4*/ EStdKeyNull,
	/*a5*/ EStdKeyNull,
	/*a6*/ EStdKeyNull,
	/*a7*/ EStdKeyNull,
	/*a8*/ EStdKeyNull,
	/*a9*/ EStdKeyNull,
	/*aa*/ EStdKeyNull,
	/*ab*/ EStdKeyNull,
	/*ac*/ EStdKeyNull,
	/*ad*/ EStdKeyNull,
	/*ae*/ EStdKeyNull,
	/*af*/ EStdKeyNull,

  	/*b0*/ EStdKeyNull,
	/*b1*/ EStdKeyNull,
	/*b2*/ EStdKeyNull,
	/*b3*/ EStdKeyNull,
	/*b4*/ EStdKeyNull,
	/*b5*/ EStdKeyNull,
	/*b6*/ EStdKeyNull,
	/*b7*/ EStdKeyNull,
	/*b8*/ EStdKeyNull,
	/*b9*/ EStdKeyNull,
	/*ba*/ EStdKeyNull,
	/*bb*/ EStdKeyNull,
	/*bc*/ EStdKeyNull,
	/*bd*/ EStdKeyNull,
	/*be*/ EStdKeyNull,
	/*bf*/ EStdKeyNull,

  	/*c0*/ EStdKeyNull,
	/*c1*/ EStdKeyNull,
	/*c2*/ EStdKeyNull,
	/*c3*/ EStdKeyNull,
	/*c4*/ EStdKeyNull,
	/*c5*/ EStdKeyNull,
	/*c6*/ EStdKeyNull,
	/*c7*/ EStdKeyNull,
	/*c8*/ EStdKeyNull,
	/*c9*/ EStdKeyNull,
	/*ca*/ EStdKeyNull,
	/*cb*/ EStdKeyNull,
	/*cc*/ EStdKeyNull,
	/*cd*/ EStdKeyNull,
	/*ce*/ EStdKeyNull,
	/*cf*/ EStdKeyNull,

  	/*d0*/ EStdKeyNull,
	/*d1*/ EStdKeyNull,
	/*d2*/ EStdKeyNull,
	/*d3*/ EStdKeyNull,
	/*d4*/ EStdKeyNull,
	/*d5*/ EStdKeyNull,
	/*d6*/ EStdKeyNull,
	/*d7*/ EStdKeyNull,
	/*d8*/ EStdKeyNull,
	/*d9*/ EStdKeyNull,
	/*da*/ EStdKeyNull,
	/*db*/ EStdKeyNull,
	/*dc*/ EStdKeyNull,
	/*dd*/ EStdKeyNull,
	/*de*/ EStdKeyNull,
	/*df*/ EStdKeyNull,

	/*e0*/ EStdKeyNull,
	/*e1*/ EStdKeyNull,
	/*e2*/ EStdKeyNull,
	/*e3*/ EStdKeyNull,
	/*e4*/ EStdKeyNull,
	/*e5*/ EStdKeyNull,
	/*e6*/ EStdKeyNull,
	/*e7*/ EStdKeyNull,
	/*e8*/ EStdKeyNull,
	/*e9*/ EStdKeyNull,
	/*ea*/ EStdKeyNull,
	/*eb*/ EStdKeyNull,
	/*ec*/ EStdKeyNull,
	/*ed*/ EStdKeyNull,
	/*ee*/ EStdKeyNull,
	/*ef*/ EStdKeyNull,
};

#endif
