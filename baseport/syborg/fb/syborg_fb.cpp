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
* Accenture Ltd - Syborg framebuffer improvements, now auto determines frame size from board model, performance and memory improvements
*
* Description: Minimalistic frame buffer driver
*
*/

#include <syborg_priv.h>
#include "syborg_fb.h"

TPhysAddr Syborg::VideoRamPhys;
TPhysAddr Syborg::VideoRamPhysSecure;		// Secure display memory

TPhysAddr TSyborg::VideoRamPhys()
{
  __KTRACE_OPT(KEXTENSION,Kern::Printf("TSyborg::VideoRamPhys: VideoRamPhys=0x%x", Syborg::VideoRamPhys));
  return Syborg::VideoRamPhys;
}

TPhysAddr TSyborg::VideoRamPhysSecure()
{
  return Syborg::VideoRamPhysSecure;
}

LOCAL_C TInt DoHalFunction(TAny* aPtr, TInt aFunction, TAny* a1, TAny* a2)
{
  DLcdPowerHandler* pH=(DLcdPowerHandler*)aPtr;
  return pH->HalFunction(aFunction,a1,a2);
}

static void rxMsg(TAny* aPtr)
{
  DLcdPowerHandler& h=*(DLcdPowerHandler*)aPtr;
  TMessageBase* pM = h.iMsgQ.iMessage;
  if(pM)
	h.HandleMsg(pM);
}

static void power_up_dfc(TAny* aPtr)
{
  ((DLcdPowerHandler*)aPtr)->PowerUpDfc();
}

void power_down_dfc(TAny* aPtr)
{
  ((DLcdPowerHandler*)aPtr)->PowerDownDfc();
}

void DLcdPowerHandler::DisplayOn()
{ 
  PowerUpLcd(iSecureDisplay);
  iDisplayOn = ETrue;
}

void DLcdPowerHandler::DisplayOff()
{
  PowerDownLcd();
  iDisplayOn = EFalse;
}

void DLcdPowerHandler::SwitchDisplay(TBool aSecure)
 {
   if(aSecure)
	 {
	   if(!iSecureDisplay)
		 {
		   DisplayOff();
		   iSecureDisplay = ETrue;
		   DisplayOn();
		 }
	 }
   else
	 {
	   if(iSecureDisplay)
		 {
		   DisplayOff();
		   iSecureDisplay = EFalse;
		   DisplayOn();
		 }
	 }
 }

void DLcdPowerHandler::PowerUpDfc()
{
  DisplayOn();
  PowerUpDone();
}

void DLcdPowerHandler::PowerDownDfc()
{
  DisplayOff();
  PowerDownDone();
}

void DLcdPowerHandler::PowerDown(TPowerState)
{
  iPowerDownDfc.Enque();
}

void DLcdPowerHandler::PowerUp()
{
  iPowerUpDfc.Enque();
}

void DLcdPowerHandler::PowerUpLcd(TBool aSecure)
{

  WriteReg(iPortAddr, FB_ENABLED, 0);
  WriteReg(iPortAddr, FB_BASE, aSecure ? iSecurevRamPhys : ivRamPhys);

  WriteReg(iPortAddr, FB_BLANK, 0);
  WriteReg(iPortAddr, FB_BPP, 32);
  WriteReg(iPortAddr, FB_COLOR_ORDER, 0);
  WriteReg(iPortAddr, FB_BYTE_ORDER, 0);
  WriteReg(iPortAddr, FB_PIXEL_ORDER, 0);
  WriteReg(iPortAddr, FB_INT_MASK, 0);
  WriteReg(iPortAddr, FB_ENABLED, 1);

  // We don't write the Height and Width of the framebuffer, this is controlled by the board model

}

void DLcdPowerHandler::PowerDownLcd()
{
  WriteReg(iPortAddr, FB_BLANK, 1);
}

DLcdPowerHandler::DLcdPowerHandler()
	:	DPowerHandler(KLitLcd),
		iMsgQ(rxMsg,this,NULL,1),
		iPowerUpDfc(&power_up_dfc,this,6),
		iPowerDownDfc(&power_down_dfc,this,7)
{
}

void DLcdPowerHandler::ScreenInfo(TScreenInfoV01& anInfo)
{
  anInfo.iWindowHandleValid = EFalse;
  anInfo.iWindowHandle = NULL;
  anInfo.iScreenAddressValid = ETrue;
  anInfo.iScreenAddress = (TAny *)(iChunk->LinearAddress());
  anInfo.iScreenSize.iWidth = iVideoInfo.iSizeInPixels.iWidth;
  anInfo.iScreenSize.iHeight = iVideoInfo.iSizeInPixels.iHeight;
}

void DLcdPowerHandler::HandleMsg(TMessageBase* aMsg)
{
  if(aMsg->iValue)
	DisplayOn();
  else
	DisplayOff();
  aMsg->Complete(KErrNone,ETrue);
}

void DLcdPowerHandler::WsSwitchOnScreen()
{
  TThreadMessage& m = Kern::Message();
  m.iValue = ETrue;
  m.SendReceive(&iMsgQ);
}

void DLcdPowerHandler::WsSwitchOffScreen()
{
  TThreadMessage& m = Kern::Message();
  m.iValue = EFalse;
  m.SendReceive(&iMsgQ);
}

TInt DLcdPowerHandler::GetCurrentDisplayModeInfo(TVideoInfoV01& aInfo, TBool aSecure)
{
  NKern::FMWait(&iLock);
  if(aSecure)
	aInfo = iSecureVideoInfo;
  else
	aInfo = iVideoInfo;
  NKern::FMSignal(&iLock);
  return KErrNone;
}

TInt DLcdPowerHandler::GetSpecifiedDisplayModeInfo(TInt aMode, TVideoInfoV01& aInfo)
{
  if(aMode < 0 || aMode >= KConfigLcdNumberOfDisplayModes)
	return KErrArgument;
  
  NKern::FMWait(&iLock);
  aInfo = iVideoInfo;
  NKern::FMSignal(&iLock);

  if(aMode != aInfo.iDisplayMode)
	{

	  aInfo.iOffsetToFirstPixel = KConfigOffsetToFirstPixel;

	  aInfo.iIsPalettized       = KConfigIsPalettized;

	  aInfo.iOffsetBetweenLines = iVideoInfo.iSizeInPixels.iWidth*4; //Offset depends on width of framebuffer

	  aInfo.iBitsPerPixel       = KConfigBitsPerPixel;
	}
  return KErrNone;
}

TInt DLcdPowerHandler::AllocateFrameBuffer()
{
// Allocate physical RAM for video
	
	//read width and height of display from board model and allocate size
	TInt width = ReadReg(iPortAddr, FB_WIDTH);
	TInt height = ReadReg(iPortAddr, FB_HEIGHT);
	
	TInt vSize = 4*width*height; //*4 as 32bits per pixel

	NKern::ThreadEnterCS();
	TInt r = Epoc::AllocPhysicalRam(vSize,Syborg::VideoRamPhys);
	if (r != KErrNone)
	{
	        NKern::ThreadLeaveCS();
		Kern::Fault("AllocVideoRam",r);
	}

	// Map the video RAM
	ivRamPhys = TSyborg::VideoRamPhys();

	r = DPlatChunkHw::New(iChunk,ivRamPhys,vSize,EMapAttrUserRw|EMapAttrBufferedC);

	NKern::ThreadLeaveCS();

	if(r != KErrNone)
	  return r;

	TUint* pV = (TUint*)iChunk->LinearAddress();

	// Allocate physical RAM for secure display
	NKern::ThreadEnterCS();
	r = Epoc::AllocPhysicalRam(vSize,Syborg::VideoRamPhysSecure);
	if (r != KErrNone)
	{
	        NKern::ThreadLeaveCS();
		Kern::Fault("AllocVideoRam 2",r);
	}
	iSecurevRamPhys = ivRamPhys + vSize;
	TInt r2 = DPlatChunkHw::New(iSecureChunk,iSecurevRamPhys,vSize,EMapAttrUserRw|EMapAttrBufferedC);

	NKern::ThreadLeaveCS();

	if(r2 != KErrNone)
	  return r2;

	TUint* pV2 = (TUint*)iSecureChunk->LinearAddress();

	//width and height set by reading board model
	iVideoInfo.iSizeInPixels.iWidth  = width;
	iVideoInfo.iSizeInPixels.iHeight = height;

	//offset between lines depends on width of screen
	iVideoInfo.iOffsetBetweenLines = width*4;

	iVideoInfo.iDisplayMode = KConfigLcdDisplayMode;
	iVideoInfo.iOffsetToFirstPixel = KConfigOffsetToFirstPixel;
	
	iVideoInfo.iIsPalettized = KConfigIsPalettized;
	iVideoInfo.iBitsPerPixel = KConfigBitsPerPixel;
	iVideoInfo.iSizeInTwips.iWidth = KConfigLcdWidthInTwips;
	iVideoInfo.iSizeInTwips.iHeight = KConfigLcdHeightInTwips;
	iVideoInfo.iIsMono = KConfigIsMono;
	iVideoInfo.iVideoAddress = (TInt)pV;
	iVideoInfo.iIsPixelOrderLandscape = KConfigPixelOrderLandscape;
	iVideoInfo.iIsPixelOrderRGB = KConfigPixelOrderRGB;

	iSecureVideoInfo = iVideoInfo;
	iSecureVideoInfo.iVideoAddress = (TInt)pV2;
	
	return KErrNone;
}


TInt DLcdPowerHandler::SetDisplayMode(TInt aMode)
{
  if(aMode < 0 || aMode >= KConfigLcdNumberOfDisplayModes)
	return KErrArgument;

  // store the current mode
  iVideoInfo.iDisplayMode = aMode;

  // store the current mode for secure screen
  iSecureVideoInfo.iDisplayMode = aMode;

  return KErrNone;
}

TInt DLcdPowerHandler::HalFunction(TInt aFunction, TAny* a1, TAny* a2)
{
  TInt r=KErrNone;
  switch(aFunction)
	{
	case EDisplayHalScreenInfo:
	  {
		TPckgBuf<TScreenInfoV01> vPckg;
		ScreenInfo(vPckg());
		Kern::InfoCopy(*(TDes8*)a1,vPckg);
		break;
	  }
	case EDisplayHalWsRegisterSwitchOnScreenHandling:
	  {
		iWsSwitchOnScreen=(TBool)a1;
		break;
	  }
	case EDisplayHalWsSwitchOnScreen:
	  {
		WsSwitchOnScreen();
		break;
	  }
	case EDisplayHalModeCount:
	  {
		TInt ndm = KConfigLcdNumberOfDisplayModes;
		kumemput32(a1, &ndm, sizeof(ndm));
		break;
	  }
	case EDisplayHalSetMode:
	  {
		__KTRACE_OPT(KEXTENSION,Kern::Printf("EDisplayHalSetMode"));
		__SECURE_KERNEL(
						if(!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetMode")))
						return KErrPermissionDenied;
						)
		  r = SetDisplayMode((TInt)a1);
		break;
	  }
	case EDisplayHalMode:
	  {
		kumemput32(a1, &iVideoInfo.iDisplayMode, sizeof(iVideoInfo.iDisplayMode));
		r = KErrNone;
		break;
	  }
	case EDisplayHalSetPaletteEntry:
	  {
		__SECURE_KERNEL(
						if(!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetPaletteEntry")))
						return KErrPermissionDenied;
						)
		  r = KErrNotSupported;
		break;
	  }
	case EDisplayHalPaletteEntry:
	  {
		TInt entry;
		kumemget32(&entry, a1, sizeof(TInt));
		r = KErrNotSupported;
		break;
	  }
	case EDisplayHalSetState:
	  {
		__SECURE_KERNEL(
						if(!Kern::CurrentThreadHasCapability(ECapabilityPowerMgmt,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetState")))
						return KErrPermissionDenied;
						)
		  if((TBool)a1)
			WsSwitchOnScreen();
		  else
			WsSwitchOffScreen();
		break;
	  }
	case EDisplayHalState:
	  {
		kumemput32(a1, &iDisplayOn, sizeof(TBool));
		break;
	  }
	case EDisplayHalColors:
	  {
		TInt mdc = 1<<24;
		kumemput32(a1, &mdc, sizeof(mdc));
		break;
	  }
	case EDisplayHalCurrentModeInfo:
	  {
		TPckgBuf<TVideoInfoV01> vPckg;
		r = GetCurrentDisplayModeInfo(vPckg(), (TBool)a2);
		if(KErrNone == r)
		  Kern::InfoCopy(*(TDes8*)a1,vPckg);
		break;
	  }
	case EDisplayHalSpecifiedModeInfo:
	  {
		TPckgBuf<TVideoInfoV01> vPckg;
		TInt mode;
		kumemget32(&mode, a1, sizeof(mode));
		r = GetSpecifiedDisplayModeInfo(mode, vPckg());
		if(KErrNone == r)
		  Kern::InfoCopy(*(TDes8*)a2,vPckg);
		break;
	  }	
	case EDisplayHalSecure:
	  {
		kumemput32(a1, &iSecureDisplay, sizeof(TBool));
		break;
	  }
	case EDisplayHalSetSecure:
	  {
		__SECURE_KERNEL(
						if(!Kern::CurrentThreadHasCapability(ECapabilityMultimediaDD,__PLATSEC_DIAGNOSTIC_STRING("Checked by Hal function EDisplayHalSetSecure")))
						return KErrPermissionDenied;
						)
		  SwitchDisplay((TBool)a1);
		break;
	  }
	default:
	  {
		r = KErrNotSupported;
		break;
	  }			
	}
  return r;
}

TInt DLcdPowerHandler::Create()
{
	iDfcQ = Kern::DfcQue0();	// use low priority DFC queue for this driver 

	iPortAddr = KHwBaseClcd;

	AllocateFrameBuffer();
	TInt r = Kern::AddHalEntry(EHalGroupDisplay,DoHalFunction,this);
	if(r != KErrNone)
	  return r;

	iPowerUpDfc.SetDfcQ(iDfcQ);
	iPowerDownDfc.SetDfcQ(iDfcQ);
	iMsgQ.SetDfcQ(iDfcQ);
	iMsgQ.Receive();

	Add();
	DisplayOn();

	return KErrNone;
}

DECLARE_STANDARD_EXTENSION()
{
  TInt r = KErrNoMemory;
  DLcdPowerHandler* pH=new DLcdPowerHandler;
  if(pH)
	r = pH->Create();
  
  return r;
}
