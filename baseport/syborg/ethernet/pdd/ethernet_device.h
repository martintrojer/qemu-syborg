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
* Description: Register definitions and base classes for Ethernet Virtio. 
*
*/

#ifndef __QEMUETHERNET_
#define __QEMUETHERNET_

#include <ethernet.h>
#include <system.h>

#define SVPDBG
#ifdef SVPDBG
#define DP(format...) Kern::Printf(format)
#else
#define DP(format...)
#endif

#define ETHERNET_PAYLOAD_SIZE 1500

#define VIRTIO_CONFIG_S_ACKNOWLEDGE     1
#define VIRTIO_CONFIG_S_DRIVER          2
#define VIRTIO_CONFIG_S_DRIVER_OK       4
#define VIRTIO_CONFIG_S_FAILED          0x80

/* This marks a buffer as continuing via the next field. */
#define VRING_DESC_F_NEXT       1
/* This marks a buffer as write-only (otherwise read-only). */
#define VRING_DESC_F_WRITE      2

#define TX_QUEUE 1
#define RX_QUEUE 0

#define VNET_MAC ((volatile TUint8 *)(0xc600c000 + 0x100))
#define VNET ((volatile TUint32 *)0xc600c000)

enum {
    VIRTIO_ID             = 0,
    VIRTIO_DEVTYPE        = 1,
    VIRTIO_HOST_FEATURES  = 2,
    VIRTIO_GUEST_FEATURES = 3,
    VIRTIO_QUEUE_BASE     = 4,
    VIRTIO_QUEUE_NUM      = 5,
    VIRTIO_QUEUE_SEL      = 6,
    VIRTIO_QUEUE_NOTIFY   = 7,
    VIRTIO_STATUS	  	  = 8,
    VIRTIO_INT_ENABLE     = 9,
    VIRTIO_INT_STATUS     = 10
};

typedef struct
{
#define VIRTIO_NET_HDR_F_NEEDS_CSUM     1       // Use csum_start, csum_offset
 TUint8 flags;
#define VIRTIO_NET_HDR_GSO_NONE         0       // Not a GSO frame
#define VIRTIO_NET_HDR_GSO_TCPV4        1       // GSO frame, IPv4 TCP (TSO)
#define VIRTIO_NET_HDR_GSO_UDP          3       // GSO frame, IPv4 UDP (UFO)
#define VIRTIO_NET_HDR_GSO_TCPV6        4       // GSO frame, IPv6 TCP
#define VIRTIO_NET_HDR_GSO_ECN          0x80    // TCP has ECN set
  TUint8 gso_type;
  TUint16 hdr_len;
  TUint16 gso_size;
  TUint16 csum_start;
  TUint16 csum_offset;
} virtio_net_hdr;

typedef struct
{
  TUint64 addr;
  TUint32 len;
  TUint16 flags;
  TUint16 next;
} vring_desc;

typedef struct
{
  TUint16 flags;
  TUint16 idx;
  TUint16 ring[16];

} vring_avail;

typedef struct
{
  TUint16 flags;
  TUint16 idx;
  struct {
  	TUint32 id;
  	TUint32 len;
  } ring[16];
} vring_used;


//
// Class to identify the device as ethernet device for Symbian
//
class EthernetDevice : public DEthernet
{
public:
	EthernetDevice();
	~EthernetDevice();
	TInt DoCreate(TInt aUnit, const TDesC8* anInfo);
	static void Isr(TAny* aPtr);
	
	//
	// Functions that we must implement as we are inheriting from abstract base class
	//
	virtual TInt Start();
	virtual void Stop(TStopMode aMode);
	virtual void GetConfig(TEthernetConfigV01 &aConfig) const;
	virtual TInt ValidateConfig(const TEthernetConfigV01 &aConfig) const;
	virtual void CheckConfig(TEthernetConfigV01 &aConfig);
	virtual TInt Configure(TEthernetConfigV01 &aConfig);
	virtual void MacConfigure(TEthernetConfigV01 &aConfig);
	virtual void Caps(TDes8 &aCaps) const;
	virtual TInt DisableIrqs();
	virtual void RestoreIrqs(TInt aIrq);
	virtual TDfcQue* DfcQ(TInt aUnit);
	virtual TInt Send(TBuf8<KMaxEthernetPacket+32> &aBuffer);
	virtual TInt ReceiveFrame(TBuf8<KMaxEthernetPacket+32> &aBuffer, TBool okToUse);
							  						  
private:

   TEthernetConfigV01   iDefaultConfig;
	TInt  			   	iInterruptId;
	TDfc				      iRxDfc;
	TUint32				   iCreated;
	TUint32				   iStarted;
	
	//Virtio structs for transmitting data to QEMU.
	volatile vring_desc  *iTxDesc;
	volatile vring_avail *iTxAvail;
	volatile vring_used  *iTxUsed;
   //Ring size hardcoded in QEMU.    
	TUint tx_ring_size;
	
   //Virtio structs for receiving data from QEMU. 
	volatile vring_desc  *iRxDesc;
	volatile vring_avail *iRxAvail;
	volatile vring_used  *iRxUsed;
	
	TUint rx_ring_size;
   TUint rx_last_used;

   //Virtio packet header definitions. 
	virtio_net_hdr rx_header;
	virtio_net_hdr tx_header;
	
	//Buffers for Rx/Tx transmission.
	TBuf8<KMaxEthernetPacket+32> iTxBuffer;
	TBuf8<KMaxEthernetPacket+32> iRxBuffer;
		
	static void RxDfc(TAny* aPtr);
	
	//QEMU specific member functions
	void AllocRings();
	void* AllocAligned(TUint16 size);
	void AddRx();
	void SetMacAddress(void);
};

#endif // __QEMUETHERNET_
