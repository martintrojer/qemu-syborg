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

#include <ethernet.h>
#include <e32hal.h>
#include <e32cmn.h> 
#include "ethernet_device.h"

// Constants specific to this file
const TInt K1000mSecDelay	= 1000;

/////////////////////////////////////////
// Implementation of EthernetDevice class
/////////////////////////////////////////

EthernetDevice::EthernetDevice() : iRxDfc(RxDfc, this)
{
	DP("** (PDD) EthernetDevice()");
		
	iInterruptId=-1;
	iCreated = FALSE;
	iStarted = FALSE;
	iRxDfc.SetDfcQ(Kern::DfcQue0());
}

EthernetDevice::~EthernetDevice()
{
	DP("** (PDD) ~EthernetDevice()");
	DP("** (PDD) ~EthernetDevice Unbind Interrupt");
	Interrupt::Unbind(EIntNet0);
}

void EthernetDevice::GetConfig(TEthernetConfigV01 &aConfig) const
{
	DP("** (PDD) EthernetDevice::GetConfig");
	aConfig = iDefaultConfig;
}

TInt EthernetDevice::ValidateConfig(const TEthernetConfigV01 &aConfig) const
{
	DP("** (PDD) EthernetDevice::ValidateConfig");
		
	switch(aConfig.iEthSpeed)
	{
		case KEthSpeedUnknown:
			return KErrNotSupported;
		default:		
			break;
	}

	switch(aConfig.iEthDuplex)
	{
		case KEthDuplexUnknown:
			return KErrNotSupported;
		default:
			break;
	}

	return KErrNone;
}

void EthernetDevice::Caps(TDes8 &aCaps) const
{
	DP("** (PDD) EthernetDevice::Caps");
	TEthernetCaps capsBuf;

	aCaps.FillZ(aCaps.MaxLength());
	aCaps=capsBuf.Left(Min(capsBuf.Length(),aCaps.MaxLength()));
}

TDfcQue* EthernetDevice::DfcQ(TInt aUnit)
{
	DP("** (PDD) EthernetDevice::DfcQ");
	return Kern::DfcQue0();
}

TInt EthernetDevice::DisableIrqs()
{
	DP("** (PDD) EthernetDevice::DisableIrqs");
	return NKern::DisableInterrupts(1);
}

void EthernetDevice::RestoreIrqs(TInt aLevel)
{
	DP("** (PDD) EthernetDevice::RestoreIrqs");
	NKern::RestoreInterrupts(aLevel);
}

//
// Enable Ethernet interrupt line, allocate memory for Rx/Tx paths and send QEMU frames for Rx transmission.  
//
TInt EthernetDevice::DoCreate(TInt aUnit, const TDesC8* anInfo)
{
	DP("** (PDD) Enter: EthernetDevice::DoCreate");
	
	if(iCreated)
		return KErrNone;
	
	// Bind to Ethernet interrupt
	Interrupt::Bind(EIntNet0,Isr,this);
	SetMacAddress();
	
	DP("** (PDD) EthernetDevice::DoCreate - VNET[VIRTIO_STATUS]");
	
   VNET[VIRTIO_STATUS] = VIRTIO_CONFIG_S_ACKNOWLEDGE
     			| VIRTIO_CONFIG_S_DRIVER;
	 
   AllocRings();
	VNET[VIRTIO_STATUS] |= VIRTIO_CONFIG_S_DRIVER_OK;
   AddRx();
	VNET[VIRTIO_INT_ENABLE] = 1;
	  
	iCreated = TRUE;
	
	DP("** (PDD) Exit: EthernetDevice::DoCreate");
	
	return KErrNone;
}

TInt EthernetDevice::Start()
{
	DP("** (PDD) EthernetDevice::Start()");
		
	if(iStarted)
		return KErrNone;
		   			       		   			   
    // wait for 1 sec as negotiation is going on... hopefully auto-neg completes by then
 	NKern::Sleep(K1000mSecDelay);
 	Interrupt::Enable(EIntNet0);
	
	iStarted = TRUE;
	
	return KErrNone;
}

void EthernetDevice::Stop(TStopMode aMode)
{
	DP("** (PDD) EthernetDevice::Stop(TStopMode aMode)");
		
	switch (aMode)
	{
		case EStopNormal:
		case EStopEmergency:
			iRxDfc.Cancel();
         //Should we disable QEMU interrupts here?
			break;					
        default:
            break;
	}
	
	iStarted = FALSE;	
	Interrupt::Disable(EIntNet0);
}

//
// Transmit Tx data to QEMU and increment iTxAvail idx counter to indicate to QEMU that a new frame is available.  
//
TInt EthernetDevice::Send(TBuf8<KMaxEthernetPacket+32> &aBuffer)
{
	DP("** (PDD) EthernetDevice::Send");
		
	TInt err = KErrNone;
	TUint32 length = aBuffer.Length();
	iTxBuffer = aBuffer;

   DP ("** (PDD) Value of iTxAvail->idx = %d\n", iTxAvail->idx);	
   TInt ring_slot = iTxAvail->idx & (tx_ring_size - 1);
   DP ("** (PDD) Value of ring_slot = %d\n", ring_slot);

   memset(&tx_header, 0, sizeof(tx_header));

   iTxDesc[0].addr = Epoc::LinearToPhysical((TUint32)&tx_header);
   iTxDesc[0].len = sizeof(tx_header);
   iTxDesc[0].flags = VRING_DESC_F_NEXT;
   iTxDesc[0].next = 1;
   iTxDesc[1].addr = Epoc::LinearToPhysical((TUint32) iTxBuffer.Ptr());
   iTxDesc[1].len = length;
   iTxDesc[1].flags = 0;
   iTxAvail->ring[ring_slot] = 0;

   iTxAvail->idx++;   
   DP ("** (PDD)iTxAvail->idx = %x, rx_last_used=%d\n", iTxAvail->idx, rx_last_used);
  
   VNET[VIRTIO_QUEUE_NOTIFY] = TX_QUEUE;

   DP ("** (PDD) iTxDesc[0].addr = %x\n", iTxDesc[0].addr);
   DP ("** (PDD) iTxDesc[0].len = %d\n", iTxDesc[0].len);
   DP ("** (PDD) iTxDesc[0].flags = %d\n", iTxDesc[0].flags);
   DP ("** (PDD) iTxDesc[0].next = %d\n", iTxDesc[0].next);
   DP ("** (PDD) iTxDesc[1].addr = %x\n", iTxDesc[1].addr);
   DP ("** (PDD) iTxDesc[1].len = %d\n", iTxDesc[1].len);
   DP ("** (PDD) iTxDesc[1].flags = %d\n", iTxDesc[1].flags);
    					 	   		
	return err;
}

TInt EthernetDevice::ReceiveFrame(TBuf8<KMaxEthernetPacket+32> &aBuffer, 
									TBool okToUse)
{
	DP("** (PDD) EthernetDevice::ReceiveFrame");
    
   //If no buffer available dump frame
   if (!okToUse)
   {
    DP("** (PDD) EthernetDevice::ReceiveFrame - dumping frame");
    return KErrGeneral;
   }
   
   aBuffer.Copy(iRxBuffer.Ptr(), ETHERNET_PAYLOAD_SIZE); 
   AddRx();
 
	return KErrNone;
}

void EthernetDevice::Isr(TAny* aPtr)
{
	DP("** (PDD) EthernetDevice::Isr(TAny* aPtr)");

   Interrupt::Clear(EIntNet0);	
   VNET[VIRTIO_INT_STATUS] = 1;
   EthernetDevice& d=*(EthernetDevice*)aPtr;
   d.iRxDfc.Add();                   			
}

void EthernetDevice::RxDfc(TAny* aPtr)
{
	DP("** (PDD) EthernetDevice::RxDfc");
	EthernetDevice& d=*(EthernetDevice*)aPtr;
   
   TInt x = VNET[VIRTIO_INT_STATUS];
   DP("** (PDD) EthernetDevice::RxDfc - value of x=%d", x);

   TInt ring_slot = d.iRxUsed->idx & (d.rx_ring_size - 1);

   DP("**(PDD) RxDfc  (d.iTxAvail->idx) = %d (d.iRxAvail->idx) = %d", d.iTxAvail->idx, d.iRxAvail->idx);
   DP("**(PDD) RxDfc  (d.iRxUsed->idx) = %d ", d.iRxUsed->idx);
   DP("**(PDD) RxDfc  ring_slot=%d,  (d.iRxUsed->ring[%d].id) = %d", ring_slot, ring_slot, d.iRxUsed->ring[ring_slot].id);
   DP("**(PDD) RxDfc  d.iRxDesc[1].next = %d", d.iRxDesc[1].next); 

   //check to see if this is a Rx or Tx
   if (d.iRxUsed->idx != d.rx_last_used)
   {
   DP("** (PDD) Received Rx Interrupt");
   d.rx_last_used++;
   DP("** (PDD) Value of rx_last_used=%d", d.rx_last_used);
   d.ReceiveIsr();
   }
}

void EthernetDevice::MacConfigure(TEthernetConfigV01 &aConfig)
{
	DP("** (PDD) EthernetDevice::MacConfigure");
	
	iDefaultConfig.iEthAddress[0] = aConfig.iEthAddress[0];
	iDefaultConfig.iEthAddress[1] = aConfig.iEthAddress[1];
	iDefaultConfig.iEthAddress[2] = aConfig.iEthAddress[2];
	iDefaultConfig.iEthAddress[3] = aConfig.iEthAddress[3];
	iDefaultConfig.iEthAddress[4] = aConfig.iEthAddress[4];
	iDefaultConfig.iEthAddress[5] = aConfig.iEthAddress[5];
	
	DP ("** (PDD) macaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
			iDefaultConfig.iEthAddress[0], 
			iDefaultConfig.iEthAddress[1], 
			iDefaultConfig.iEthAddress[2], 
			iDefaultConfig.iEthAddress[3], 
			iDefaultConfig.iEthAddress[4], 
			iDefaultConfig.iEthAddress[5]);
}

//Descriptor list and vring_used must start on a 4k page boundary. 
TAny * EthernetDevice::AllocAligned(TUint16 size)
{
	DP("** (PDD) Enter: EthernetDevice::alloc_aligned");
	DP("** (PDD) size=%d",size);

   TAny * p = Kern::Alloc(size + 4095);
	
   DP("** (PDD) BEFORE: p=%x",p);
   p = (TAny *)(((TUint32)p + 4095) & ~4095);
   DP("** (PDD) AFTER: p=%x",p);
	
	DP("** (PDD) Exit: EthernetDevice::alloc_aligned");
	
	return p;
}
	
TAny EthernetDevice::AllocRings()
{	
	DP("** (PDD) Enter: EthernetDevice::AllocRings");
	
	TUint size;
	TUint used_offset;
	TUint32 p;

	VNET[VIRTIO_QUEUE_SEL] = TX_QUEUE;
   tx_ring_size = VNET[VIRTIO_QUEUE_NUM];

	DP ("** (PDD) tx_ring_size = %d\n", tx_ring_size);
	
	size = (tx_ring_size * (16 + 2)) + 4;
	used_offset = (size + 4095) & ~1024;
	DP ("** (PDD) used_offset = %d", used_offset);
	size = used_offset + 4 + (tx_ring_size * 8);
	DP ("** (PDD) size = %d", size);
	
	p = (TUint32)AllocAligned(size);
	DP ("** (PDD) p = %x\n", p);

	VNET[VIRTIO_QUEUE_BASE] = Epoc::LinearToPhysical(p);

	iTxDesc = reinterpret_cast<vring_desc*>(p);
	iTxAvail = reinterpret_cast<vring_avail*>(p + tx_ring_size * 16);
	iTxUsed = reinterpret_cast<vring_used*>(p + used_offset); 
    
   DP ("** (PDD) iTxDesc = %x", iTxDesc);
   DP ("** (PDD) iTxAvail = %x", iTxAvail);
   DP ("** (PDD) iTxUsed = %x", iTxUsed);
    
	VNET[VIRTIO_QUEUE_SEL] = RX_QUEUE;
	rx_ring_size = VNET[VIRTIO_QUEUE_NUM];
	
	size = (rx_ring_size * (16 + 2)) + 4;
	used_offset = (size + 4095) & ~4095;
	size = used_offset + 4 + (rx_ring_size * 8);
	p = (TUint32)AllocAligned(size);
    DP ("** (PDD)  p = %x\n", p);
	VNET[VIRTIO_QUEUE_BASE] = Epoc::LinearToPhysical(p);
	
	iRxDesc = reinterpret_cast<vring_desc*>(p);
	iRxAvail = reinterpret_cast<vring_avail*>(p + rx_ring_size * 16);
	iRxUsed = reinterpret_cast<vring_used*>(p + used_offset);

    DP ("** (PDD) iRxDesc = %x", iRxDesc);
    DP ("** (PDD) iRxAvail = %x", iRxAvail);
    DP ("** (PDD) iRxUsed = %x", iRxUsed);

	DP("** (PDD) Exit: EthernetDevice::AllocRings");
}

void EthernetDevice::AddRx()
{
    DP("** (PDD) Enter: EthernetDevice::AddRx"); 
   
    TInt n = iRxAvail->idx & (rx_ring_size - 1);
    memset(&rx_header, 0, sizeof(rx_header));
    iRxDesc[0].addr = Epoc::LinearToPhysical((TUint32)&rx_header);
    iRxDesc[0].len = sizeof(rx_header);
    iRxDesc[0].flags = VRING_DESC_F_NEXT | VRING_DESC_F_WRITE;
    iRxDesc[0].next = 1;
    iRxDesc[1].addr = Epoc::LinearToPhysical((TUint32) iRxBuffer.Ptr());
    iRxDesc[1].len = ETHERNET_PAYLOAD_SIZE;
    iRxDesc[1].flags = VRING_DESC_F_WRITE;
    iRxAvail->ring[n] = 0;
    iRxAvail->idx++;
    VNET[VIRTIO_QUEUE_NOTIFY] = RX_QUEUE;

    DP("** (PDD) Exit: EthernetDevice::AddRx");
}



void EthernetDevice::SetMacAddress(void)
	{
	DP("** (PDD) Enter: EthernetDevice::SetMacAddress");
	
	iDefaultConfig.iEthAddress[0] = VNET_MAC[0];
	iDefaultConfig.iEthAddress[1] = VNET_MAC[1];
	iDefaultConfig.iEthAddress[2] = VNET_MAC[2];
	iDefaultConfig.iEthAddress[3] = VNET_MAC[3];
	iDefaultConfig.iEthAddress[4] = VNET_MAC[4];
	iDefaultConfig.iEthAddress[5] = VNET_MAC[5];
	
	DP ("** (PDD) macaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
			iDefaultConfig.iEthAddress[0], 
			iDefaultConfig.iEthAddress[1], 
			iDefaultConfig.iEthAddress[2], 
			iDefaultConfig.iEthAddress[3], 
			iDefaultConfig.iEthAddress[4], 
			iDefaultConfig.iEthAddress[5]);
	
	DP("** (PDD) Exit: EthernetDevice::SetMacAddress");
	}

//
// Functions that are just stubs
//

void EthernetDevice::CheckConfig(TEthernetConfigV01& aConfig)
{
	DP("** (PDD) EthernetDevice::CheckConfig");
}

TInt EthernetDevice::Configure(TEthernetConfigV01 &aConfig)
{
	DP("** (PDD) EthernetDevice::Configure");
	return KErrNone;
}
