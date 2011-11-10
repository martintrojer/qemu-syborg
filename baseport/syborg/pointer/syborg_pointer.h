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
* Description: Minimalistic pointer driver
*
*/

#ifndef _SYBORG_POINTER_H
#define _SYBORG_POINTER_H

#include <kernel.h>
#include <system.h>
#include <videodriver.h>
#include <hal.h>

#define FIFO_SIZE 16

#ifdef DEBUG
#define __DEBUG_PRINT(format...)    Kern::Printf(format)
#else
#define __DEBUG_PRINT(format...)    __KTRACE_OPT(KBOOT,Kern::Printf(format))
#endif

class TPointerRv
{
public:
  TPointerRv();
  virtual ~TPointerRv();
  static TInt DoPointerHalFunction(TAny* aThis, TInt aFunction, TAny* a1, TAny* a2);
  TInt PointerHalFunction(TInt aFunction, TAny* a1, TAny* a2);
  void Init3();

 private:
  struct PData {
	TInt x;
	TInt y;
	TInt z;
	TInt but;
  };

  struct PData* FifoPop(void);
  void FifoPush(struct PData*);

  struct PData iPDataFifo[FIFO_SIZE];
  TInt iFifoPos;
  TInt iFifoCount;

 private:
  static void Isr(TAny* aPtr);
  static void RxDfc(TAny* aPtr );
  static void Process(TPointerRv *i, struct PData *);

  void DisplayPointer();



  //TDfc iRxDfc;
  TDfcQue iDfcQue;
  TDfc* iRxDfc;

	
  TBool iPointerOn;       // cursor visiability
  TInt  iScreenWidth;
  TInt  iScreenHeight;
  TInt  iDisplayMode;

  TInt  iVideoMem;
  TInt  iOffSetBetweenEachLine;


  TInt ix,iy;
  TInt iLastBut;


  TBool iFirstTime;
  TInt iXleft;
  TInt iXright;
  TInt iYtop;
  TInt iYbottom;
  TInt iImageStore[100];

  
 public:

  enum {
    POINTER_ID          = 0,
    POINTER_LATCH       = 1,
    POINTER_FIFO_COUNT  = 2,
    POINTER_X           = 3,
    POINTER_Y           = 4,
    POINTER_Z           = 5,
    POINTER_BUTTONS     = 6,
    POINTER_INT_ENABLE  = 7
  };

 private:
  // Fixed point maths
  class Fixed {

  private:
	int g;
	const static int BP = 8;
	const static int BP2 = BP*2;
	enum FixedRaw { RAW };
    Fixed(FixedRaw, int guts) : g(guts) {}
	
  public:
    Fixed() : g(0) {}
    Fixed(const Fixed& a) : g( a.g ) {}
    Fixed(int a) : g( a << BP ) {}
	operator int() { return g>>BP; }
	Fixed operator +() const { return Fixed(RAW,g); }
	Fixed operator -() const { return Fixed(RAW,-g); }
	Fixed operator +(const Fixed& a) const { return Fixed(RAW, g + a.g); }
	Fixed operator -(const Fixed& a) const { return Fixed(RAW, g - a.g); }
	Fixed operator *(const Fixed& a) const { return Fixed(RAW,  (int)( ((long long)g * (long long)a.g ) >> BP)); }
	Fixed operator /(const Fixed& a) const { return Fixed(RAW, int( (((long long)g << BP2) / (long long)(a.g)) >> BP) ); }
  };

  Fixed iXFactor;
  Fixed iYFactor;
};

#endif
