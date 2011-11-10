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
* NTT DOCOMO, INC. -- Fix bug 2186 - QEMU baseport serial driver doesn't work with multiple UARTs and is unreliable
*
* Description: Minimalistic serial driver
*
*/

#ifndef _SYBORG_SERIAL_H
#define _SYBORG_SERIAL_H

#include <comm.h>
#include <e32hal.h>
#include "system.h"

const TInt KMinimumLddMajorVersion=1;
const TInt KMinimumLddMinorVersion=1;
const TInt KMinimumLddBuild=1;

/**
 * Serial Ports Device Driver
 *
 *
 **/
class DDriverSyborgComm : public DPhysicalDevice
	{
public:
  DDriverSyborgComm();
  ~DDriverSyborgComm();
  virtual TInt Install();
  virtual void GetCaps(TDes8 &aDes) const;
  virtual TInt Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);
  virtual TInt Validate(TInt aUnit, const TDesC8* anInfo, const TVersion &aVer);

// interface for single comm port.
public:
  TInt OpenDfcQueue();
  void CloseDfcQueue();
  TDfcQue* DfcQueue();

private:
	TDynamicDfcQue* 	iDfcQueue;				// single Dfc queue for all comm ports
	TUint8  			iDfcQRefCount;			// reference count for dfc queue
	};



/**
 * Driver for a single Comm port
 **/
class DCommSyborgSoc : public DComm
	{
public:
	DCommSyborgSoc(DDriverSyborgComm* aDriverSyborgComm);
	~DCommSyborgSoc();
	TInt DoCreate(TInt aUnit, const TDesC8* anInfo);
public:
	virtual TInt Start();
	virtual void Stop(TStopMode aMode);
	virtual void Break(TBool aState);
	virtual void EnableTransmit();
	virtual TUint Signals() const;
	virtual void SetSignals(TUint aSetMask, TUint aClearMask);
	virtual TInt ValidateConfig(const TCommConfigV01 &aConfig) const;
	virtual void Configure(TCommConfigV01 &aConfig);
	virtual void Caps(TDes8 &aCaps) const;
	virtual TInt DisableIrqs();
	virtual void RestoreIrqs(TInt aIrq);
	virtual TDfcQue* DfcQ(TInt aUnit);
	virtual void CheckConfig(TCommConfigV01& aConfig);
	static void Isr(TAny* aPtr);

public:
	DDriverSyborgComm* iDriverSyborgComm;
	TLinAddr iPortAddr;
	TInt iIrq;
	TBool iDfcQueueOpened;

	enum {
		SERIAL_ID           = 0,
		SERIAL_DATA         = 1,
		SERIAL_FIFO_COUNT   = 2,
		SERIAL_INT_ENABLE   = 3,
		SERIAL_DMA_TX_ADDR  = 4,
		SERIAL_DMA_TX_COUNT = 5, /* triggers dma */
		SERIAL_DMA_RX_ADDR  = 6,
		SERIAL_DMA_RX_COUNT = 7 /* triggers dma */
		};
	};

#endif
