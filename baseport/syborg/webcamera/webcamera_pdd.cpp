/*
* Copyright (c) 2010 ISB.
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* ISB - initial contribution.
*
* Contributors:
*
* Description: USB driver for test
*
*/

#include "webcamera_pdd.h"
#include <webcamera_driver.h>

#define DP(format...) Kern::Printf(format)

//Name for PDD
_LIT(KWebcameraPddName,"WebcameraDevice.pdd");

// ---------------------------------------------------------------
// ---------------------------------------------------------------

DWebcameraPddFactory::DWebcameraPddFactory()
{
  DP("DWebcameraPddFactory::DWebcameraPddFactory()");
  iVersion=TVersion(KCommsMajorVersionNumber,KCommsMinorVersionNumber,KCommsBuildVersionNumber);
}

TInt DWebcameraPddFactory::Install()
{
  DP("DWebcameraPddFactory::Install");
  return SetName(&KWebcameraPddName);
}

void DWebcameraPddFactory::GetCaps(TDes8 &aDes) const
{
  DP("DWebcameraPddFactory::GetCaps start");
  RWebcameraDevice::TCaps capsBuf;
  capsBuf.iVersion = iVersion;
  aDes.FillZ(aDes.MaxLength());
  TInt size=sizeof(capsBuf);
  if(size>aDes.MaxLength())
	  {
      size=aDes.MaxLength();
	  }
  aDes.Copy((TUint8*)&capsBuf,size);
  DP("DWebcameraPddFactory::GetCaps end");
}

TInt DWebcameraPddFactory::Create(DBase*& aChannel, TInt aUnit, const TDesC8* anInfo, const TVersion& aVer)
{
  DP("DWebcameraPddFactory::Create start");
  DWebcameraDriver* pD=new DWebcameraDriver;
  aChannel=pD;
  TInt r=KErrNoMemory;
  if (pD)
	  {
	  r=pD->DoCreate(aUnit,anInfo);
	  }
  DP("DWebcameraPddFactory::Create end");
  return r;
}

TInt DWebcameraPddFactory::Validate(TInt aUnit, const TDesC8* /*anInfo*/, const TVersion& aVer)
{
  DP("DWebcameraPddFactory::Validate start");
  if ((!Kern::QueryVersionSupported(iVersion,aVer)) || 
	  (!Kern::QueryVersionSupported(aVer,TVersion(KMinimumLddMajorVersion,KMinimumLddMinorVersion,KMinimumLddBuild))))
	  {
		return KErrNotSupported;
	  }
  DP("DWebcameraPddFactory::Validate end");
  return KErrNone;
}

// ---------------------------------------------------------------
// ---------------------------------------------------------------

DWebcameraDriver::DWebcameraDriver()
{
	DP("DWebcameraDriver::DWebcameraDriver start");
	DP("DWebcameraDriver::DWebcameraDriver end");
}

DWebcameraDriver::~DWebcameraDriver()
{
  DP("DWebcameraDriver::~DWebcameraDriver start");
  Interrupt::Unbind(iIrq);
  DP("DWebcameraDriver::~DWebcameraDriver end");
}

TInt DWebcameraDriver::DoCreate(TInt aUnit, const TDesC8* anInfo)
{
  DP("DWebcameraDriver::DoCreate start");
  iPortAddr=KHwSVPWebcameraDevice;
  iIrq = EIrqWebamera;
  Interrupt::Bind(iIrq,Isr,this);
  DP("DWebcameraDriver::DoCreate end");
  return KErrNone;
}

TInt DWebcameraDriver::StartViewerFinder(TUint aBuffer,TInt aSize)
{
  DP("DWebcameraDriver::StartViewerFinder start");
  iType=0;
  TUint32 temp=(TUint32)aBuffer;
  DP("temp=%x",temp);
  DP("iPortAddr=%x",iPortAddr);
  WriteReg(iPortAddr,WEBCAMERA_REG_DATA_TYPE, 0x0);
  WriteReg(iPortAddr,WEBCAMERA_REG_DMA_ADDR,temp);
  WriteReg(iPortAddr,WEBCAMERA_REG_DMA_SIZE, aSize);
  WriteReg(iPortAddr,WEBCAMERA_REG_INT_ENABLE, 0x1);
  Interrupt::Enable(iIrq);
  
  DP("DWebcameraDriver::StartViewerFinder END"); 
  return KErrNone;
}

TInt DWebcameraDriver::StartCapture(TUint aBuffer,TInt aSize)
{
  DP("DWebcameraDriver::StartCapture start");
  // Save a pointer to the buffer we need to put the 'recevied' data in
  iType=1;
  TUint32 temp=(TUint32)aBuffer;
  DP("temp=%x",temp);
  WriteReg(iPortAddr,WEBCAMERA_REG_DATA_TYPE, 0x1);
  WriteReg(iPortAddr,WEBCAMERA_REG_DMA_ADDR,temp);
  WriteReg(iPortAddr,WEBCAMERA_REG_DMA_SIZE, aSize);
  WriteReg(iPortAddr,WEBCAMERA_REG_INT_ENABLE, 0x1);
  Interrupt::Enable(iIrq);
  
  DP("DWebcameraDriver::StartCapture END"); 
  return KErrNone;
}

void DWebcameraDriver::Stop(TUSBStopMode aMode)
{
  DP("DWebcameraDriver::Stop start");
  WriteReg(iPortAddr, WEBCAMERA_REG_INT_ENABLE, 0x0);
  Interrupt::Disable(iIrq);
  DP("DWebcameraDriver::Stop end");
}

void DWebcameraDriver::Isr(TAny* aPtr)
{
  DP("DWebcameraDriver::Isr start");
  ((DWebcameraDriver*)aPtr)->receivedatacallback();
  DP("DWebcameraDriver::Isr end");
}

void DWebcameraDriver::receivedatacallback()
{
  DP("DWebcameraDriver::receivedatacallback start");
  TInt datasize=ReadReg(iPortAddr,WEBCAMERA_REG_DMA_SIZE);
  switch (iType)
	  {
	  case 0:
		  iLdd->GetOneFlameComplete(datasize);
		  break;
	  case 1:
		  iLdd->CaptureComplete(datasize);
		  break;
	  default:
		  //
		  break;
	  }
  WriteReg(iPortAddr,WEBCAMERA_REG_DMA_ADDR, 0);
  WriteReg(iPortAddr,WEBCAMERA_REG_DMA_SIZE, 0);
  WriteReg(iPortAddr,WEBCAMERA_REG_INT_ENABLE, 0x0);
  DP("DWebcameraDriver::receivedatacallback end");	  
}

// ---------------------------------------------------------------
// ---------------------------------------------------------------

DECLARE_STANDARD_PDD()
{
  DP("DECLARE_STANDARD_PDD()");
  return new DWebcameraPddFactory;
}

