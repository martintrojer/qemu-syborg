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

#ifndef _SYBORG_FB_H
#define _SYBORG_FB_H

#include <videodriver.h>
#include <nkern.h>
#include <kernel.h>
#include <kpower.h>
#include <system.h>


_LIT(KLitLcd,"SYBORG_FB");

const TInt	KConfigLcdWidthInTwips     = 9638;
const TInt	KConfigLcdHeightInTwips    = 7370;

const TBool KConfigIsMono              = 0;
const TBool KConfigIsPalettized        = 0;

const TInt  KConfigOffsetToFirstPixel  = 0;

const TBool KConfigPixelOrderRGB       = 0;
const TBool KConfigPixelOrderLandscape = 1;
const TInt  KConfigLcdDisplayMode       = 2;


const TInt  KConfigLcdNumberOfDisplayModes = 3;


const TInt  KConfigBitsPerPixel        = 24;




class DLcdPowerHandler : public DPowerHandler
{
public: // from DPowerHandler
  void PowerDown(TPowerState);
  void PowerUp();
public:	// to prevent a race condition with WServer trying to power up/down at the same time
  void PowerUpDfc();
  void PowerDownDfc();
public:
  DLcdPowerHandler();
  TInt Create();
  void DisplayOn();
  void DisplayOff();
  TInt HalFunction(TInt aFunction, TAny* a1, TAny* a2);

  void PowerUpLcd(TBool aSecure);
  void PowerDownLcd();

  void ScreenInfo(TScreenInfoV01& aInfo);
  void WsSwitchOnScreen();
  void WsSwitchOffScreen();
  void HandleMsg(TMessageBase* aMsg);
  void SwitchDisplay(TBool aSecure);

private:
  TInt GetCurrentDisplayModeInfo(TVideoInfoV01& aInfo, TBool aSecure);
  TInt GetSpecifiedDisplayModeInfo(TInt aMode, TVideoInfoV01& aInfo);
  TInt SetDisplayMode(TInt aMode);
  TInt AllocateFrameBuffer();

  TBool iDisplayOn;
  DPlatChunkHw* iChunk;
  DPlatChunkHw* iSecureChunk;
  TBool iWsSwitchOnScreen;
  TBool iSecureDisplay;

public:
  TDfcQue* iDfcQ;
  TMessageQue iMsgQ;					// to prevent a race condition with Power Manager trying to power up/down at the same time
  TDfc iPowerUpDfc;
  TDfc iPowerDownDfc;

private:
  TVideoInfoV01 iVideoInfo;
  TVideoInfoV01 iSecureVideoInfo;
  NFastMutex iLock;
  TPhysAddr ivRamPhys;
  TPhysAddr iSecurevRamPhys;

public:
  TLinAddr iPortAddr;

enum {
    FB_ID               = 0,
    FB_BASE             = 1,
    FB_HEIGHT           = 2,
    FB_WIDTH            = 3,
    FB_ORIENTATION      = 4,
    FB_BLANK            = 5,
    FB_INT_MASK         = 6,
    /* begin new interface */
    FB_INTERRUPT_CAUSE  = 7,
    FB_BPP              = 8,
    FB_COLOR_ORDER      = 9,
    FB_BYTE_ORDER       = 10,
    FB_PIXEL_ORDER      = 11,
    FB_ROW_PITCH        = 12,
    FB_ENABLED          = 13,
    FB_PALETTE_START    = 0x400 >> 2,
    FB_PALETTE_END   = FB_PALETTE_START+256-1,
    /* end new interface */
};

#define FB_INT_VSYNC            (1U << 0)
#define FB_INT_BASE_UPDATE_DONE (1U << 1)

};

#endif
