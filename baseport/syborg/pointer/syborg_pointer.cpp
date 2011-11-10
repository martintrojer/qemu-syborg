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

//#define DEBUG

#include <syborg_priv.h>
#include "syborg_pointer.h"


_LIT(KPointerDfcQNamePre,"PointDFC");
const TInt KDfcQuePriority = 27;



TPointerRv::TPointerRv() //sosco :
//  iRxDfc(RxDfc,this,Kern::DfcQue0(),1)

{
  __DEBUG_PRINT("TPointerRv::TPointerRv()");


 TInt err = Kern::DfcQInit(&iDfcQue, KDfcQuePriority, &KPointerDfcQNamePre);
 if (err)
 	{
	__KTRACE_OPT(KPANIC, Kern::Printf("TPointerRv::TPointerRv() Error creating dfcq (%d)", err));
	}

  iRxDfc = new TDfc(RxDfc,this,&iDfcQue,1);
  __ASSERT_ALWAYS(iRxDfc!=NULL,Kern::Printf("Failed to create DFC"));


  TInt r = Interrupt::Bind(EIntPointer,Isr,this);
  if(r != KErrNone)
	__KTRACE_OPT(KPANIC, Kern::Printf("TPointerRv::TPointerRv() Interrupt::Bind(%d)=%d",
									  EIntPointer, r));
  iPointerOn = ETrue;
  iLastBut = 0;

  Interrupt::Enable(EIntPointer);
}

TPointerRv::~TPointerRv()
{
}

struct TPointerRv::PData* TPointerRv::FifoPop(void)
{
  struct TPointerRv::PData* val = &iPDataFifo[iFifoPos];
  iFifoPos++;
  iFifoCount--;

  if (iFifoPos == FIFO_SIZE)
	iFifoPos = 0;

  return val;
}

void TPointerRv::FifoPush(struct TPointerRv::PData* val)
{
  TInt slot;

  if (iFifoCount == FIFO_SIZE)
	return;

  slot = iFifoPos + iFifoCount;
  if (slot >= FIFO_SIZE)
	slot -= FIFO_SIZE;
  iPDataFifo[slot] = *val;
  iFifoCount++;

  //  __DEBUG_PRINT("TPointerRv::FifoPush %d %d %d %d",val->x, val->y, val->z, val->but);
  //  __DEBUG_PRINT("TPointerRv::FifoPush %d %d %d %d",iPDataFifo[slot].x, iPDataFifo[slot].y, iPDataFifo[slot].z, iPDataFifo[slot].but);

}

void TPointerRv::Init3()
{
  __DEBUG_PRINT("TPointerRv::Init3");

  TInt reg = ReadReg(KHwBaseKmiPointer,POINTER_ID);
  WriteReg(KHwBaseKmiPointer,POINTER_INT_ENABLE,1);

  TInt r = Kern::AddHalEntry(EHalGroupMouse,DoPointerHalFunction,this);
  if(r != KErrNone)
	__KTRACE_OPT(KPANIC, Kern::Printf("TPointerRv::Init3(): Kern::AddHalEntry()=%d", r));

  // Get information about the screen display mode
  TPckgBuf<TVideoInfoV01> buf;
  TVideoInfoV01& videoInfo = buf();

  r = Kern::HalFunction(EHalGroupDisplay, EDisplayHalCurrentModeInfo, (TAny*)&buf, NULL);
  if(r != KErrNone)
	__KTRACE_OPT(KPANIC, Kern::Printf("TPointerRv::Init3(): Kern::HalFunction(EDisplayHalCurrentModeInfo)=%d", r));

  iScreenWidth  = videoInfo.iSizeInPixels.iWidth;
  iScreenHeight = videoInfo.iSizeInPixels.iHeight;
  iDisplayMode  = videoInfo.iDisplayMode;

  iVideoMem = videoInfo.iVideoAddress + videoInfo.iOffsetToFirstPixel;
  iOffSetBetweenEachLine = iScreenWidth;

  ix = iy = 0;

  iXFactor = Fixed(iScreenWidth) / Fixed(0x6000);
  iYFactor = Fixed(iScreenHeight) / Fixed(0x8000);

  iFifoPos = iFifoCount = 0;


  iFirstTime= ETrue;

}


void TPointerRv::Process(TPointerRv *i, struct TPointerRv::PData *pd)
{
  TRawEvent e;

  //  __DEBUG_PRINT("Event: X=%d Y=%d Point %d", pd->x, pd->y, pd->but);
  //  __DEBUG_PRINT("  Last X=%d Y=%d Point %d", i->ix, i->iy, i->iLastBut);

  //  i->ix += pd->x;
  //  i->iy += pd->y;

  i->ix = int(Fixed(pd->x) * i->iXFactor);
  i->iy = int(Fixed(pd->y) * i->iYFactor);

  switch(pd->but)
	{
	case 0:  // Button released
	  switch(i->iLastBut)
		{
		case 0:
		  if( (pd->x!=0) || (pd->y!=0) ) {
			e.Set(TRawEvent::EPointerMove, i->ix, i->iy);
			__DEBUG_PRINT("1 EPointerMove (x:%d y:%d)", i->ix, i->iy);
		  }
		  goto fin;
		case 1: // Left
		  e.Set(TRawEvent::EButton1Up, i->ix, i->iy);
		  __DEBUG_PRINT("2 EButton1UP (x:%d y:%d)", i->ix, i->iy);
		  goto fin;
		case 2: // Right
		  e.Set(TRawEvent::EButton2Up, i->ix, i->iy);
		  __DEBUG_PRINT("3 EButton2UP (x:%d y:%d)", i->ix, i->iy);
		  goto fin;
		}
	case 1: // Left
	  if (i->iLastBut == 0) {
		e.Set(TRawEvent::EButton1Down, i->ix, i->iy);
		__DEBUG_PRINT("4 EButton1Down (x:%d y:%d)", i->ix, i->iy);
	  }
	  else if( (pd->x!=0) || (pd->y!=0) ) {
		e.Set(TRawEvent::EPointerMove, i->ix, i->iy);
		__DEBUG_PRINT("5 EPointerMove (x:%d y:%d)", i->ix, i->iy);
	  }
	  break;
	case 2: // Right
	  if (i->iLastBut == 0) {
		e.Set(TRawEvent::EButton2Down, i->ix, i->iy);
		__DEBUG_PRINT("6 EButton2Down (x:%d y:%d)", i->ix, i->iy);
	  }
	  else if( (pd->x!=0) || (pd->y!=0) ) {
		e.Set(TRawEvent::EPointerMove, i->ix, i->iy);
		__DEBUG_PRINT("7 EPointerMove (x:%d y:%d)", i->ix, i->iy);
	  }
	  break;

	}
 fin:


  i->DisplayPointer();


  Kern::AddEvent(e);
  i->iLastBut = pd->but;
}


void TPointerRv::DisplayPointer()
{

TUint32 *pMem =0;
TInt k=0;

	if(!iFirstTime)
	{

	//restore old pointer position
	pMem = (TUint32 *)iVideoMem;

	pMem+= iYtop* iOffSetBetweenEachLine;
	pMem+= iXleft;



	for(TInt i=0;i<(iYbottom-iYtop);i++)
	{
		for(TInt j=0;j<(iXright-iXleft);j++)
		{
			*pMem = iImageStore[k];
			pMem++;
			k++;

		}

		pMem+= (iOffSetBetweenEachLine - iXright) + iXleft;
	}
	}

	iFirstTime = EFalse;

	//10*10 pixel pointer centered around position and check are within allowed bounds of screen
	iXleft = ix - 5;

	if(iXleft<0)
		iXleft=0;

	iXright = ix + 5;

	if(iXright>iScreenWidth)
		iXright = iScreenWidth;

	iYtop = iy - 5;

	if(iYtop<0)
		iYtop=0;

	iYbottom = iy +5;

	if(iYbottom> iScreenHeight)
		iYbottom=iScreenHeight;

	pMem = (TUint32 *)iVideoMem;
	k=0;

	pMem+= iYtop* iOffSetBetweenEachLine;
	pMem+= iXleft;

	for(TInt i=0;i<(iYbottom-iYtop);i++)
	{
		for(TInt j=0;j<(iXright-iXleft);j++)
		{
			iImageStore[k] = *pMem;
			*pMem = 0;
			pMem++;
			k++;

		}

		pMem+= (iOffSetBetweenEachLine - iXright) + iXleft;
	}

}


void TPointerRv::RxDfc(TAny* aPtr)
{
  __DEBUG_PRINT("TPointerRv::RxDfc");

  TPointerRv* i = static_cast<TPointerRv*>(aPtr);

  while(i->iFifoCount>0) {
	struct TPointerRv::PData *pd= i->FifoPop();
	Process(i,pd);
  }
}

void TPointerRv::Isr(TAny* aPtr)
{
  __DEBUG_PRINT("TPointerRv::Isr");

  TPointerRv& k = *(TPointerRv*)aPtr;
  // interrupts are now auto clear


  while(ReadReg(KHwBaseKmiPointer, POINTER_FIFO_COUNT)!=0)
  {

	WriteReg(KHwBaseKmiPointer,POINTER_LATCH,1);
	// SOSCO: moved to here, as the buffer seems to be running one notch out,
	// writing to the pointer latch first seems to return the correct FIFO entry.

	struct TPointerRv::PData pd;
	pd.x = ReadReg(KHwBaseKmiPointer,POINTER_X);
	pd.y = ReadReg(KHwBaseKmiPointer,POINTER_Y);
	pd.z = ReadReg(KHwBaseKmiPointer,POINTER_Z);
	pd.but = ReadReg(KHwBaseKmiPointer,POINTER_BUTTONS);
	k.FifoPush(&pd);

// SOSCO - moved	WriteReg(KHwBaseKmiPointer,POINTER_LATCH,1);

  }

  //  WriteReg(KHwBaseKmiPointer,POINTER_CLEAR_INT,0);
  Interrupt::Clear(EIntPointer);

  k.iRxDfc->Add();

}

TInt TPointerRv::DoPointerHalFunction(TAny* aThis, TInt aFunction, TAny* a1, TAny* a2)
{
  return static_cast<TPointerRv*>(aThis)->PointerHalFunction(aFunction,a1,a2);
}

TInt TPointerRv::PointerHalFunction(TInt aFunction, TAny* a1, TAny* /*a2*/)
{
  __DEBUG_PRINT("TPointerRv::PointerHalFunction");

  TInt r=KErrNone;

  switch(aFunction)
    {
	case EMouseHalMouseState:
	  {
		kumemput32(a1, (TBool*)&iPointerOn, sizeof(TBool));
		break;
	  }
	case EMouseHalSetMouseState:
	  {

		  /* SOSCO - removed, causes platsec error
	        __SECURE_KERNEL(
	           if(!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,
		        __PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EMouseHalSetMouseState")))
		     return KErrPermissionDenied;
		   );
		   */

		if(((TBool)a1 == HAL::EMouseState_Visible) && (iPointerOn == (TBool)EFalse))
		  {
			iPointerOn=(TBool)ETrue;
		  }
		else if(((TBool)a1 == HAL::EMouseState_Invisible) && (iPointerOn==(TBool)ETrue))
		  {
			iPointerOn=(TBool)EFalse;
		  }
		break;
	  }
	case EMouseHalMouseInfo:
	  {
		TPckgBuf<TMouseInfoV01> vPckg;
		TMouseInfoV01& xyinfo = vPckg();
		xyinfo.iMouseButtons = 2;
		xyinfo.iMouseAreaSize.iWidth = iScreenWidth;
		xyinfo.iMouseAreaSize.iHeight = iScreenHeight;
		xyinfo.iOffsetToDisplay.iX = 0;
		xyinfo.iOffsetToDisplay.iY = 0;
		Kern::InfoCopy(*(TDes8*)a1,vPckg);
		break;
	  }
        case EMouseHalSetMouseSpeed:
	  {

		  /* SOSCO - removed, causes platsec error
	        __SECURE_KERNEL(
	           if(!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,
		        __PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EMouseHalSetMouseSpeed")))
		     return KErrPermissionDenied;
		   );
		   */

		// fall thru to NotSupported
	  }
        case EMouseHalSetMouseAcceleration:
	  {

		  /* SOSCO - removed, causes platsec error
	        __SECURE_KERNEL(
	           if(!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,
		        __PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EMouseHalSetMouseAcceleration")))
		     return KErrPermissionDenied;
		   );
		   */

		// fall thru to NotSupported
	  }
	default:
	  {
		r = KErrNotSupported;
		break;
	  }
	}
  return r;
}

DECLARE_STANDARD_EXTENSION()
{
  __DEBUG_PRINT("DECLARE_STANDARD_EXTENSION");
  TPointerRv *d = new TPointerRv;
  d->Init3();
  return KErrNone;
}
