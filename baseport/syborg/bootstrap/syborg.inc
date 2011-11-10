;/*
;* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
;* All rights reserved.
;* This component and the accompanying materials are made available
;* under the terms of the License "Eclipse Public License v1.0"
;* which accompanies this distribution, and is available
;* at the URL "http://www.eclipse.org/legal/epl-v10.html".
;*
;* Initial Contributors:
;* Nokia Corporation - initial contribution.
;*
;* Contributors:
;* NTT DOCOMO, INC - Fix for bug 1291 "E32test t_tock.exe failed to load Logical Device"
;*
;* Description:
;*
;*/

;---------------------------------------------------------------------------
;  Physical Addresses of Memory                                             
;---------------------------------------------------------------------------

KHwRamBaseAddr		EQU    0x00000000

; More ram required to boot SF image
KHwRamSizeMb            EQU    512			; 512MB
;KHwRamSizeMb            EQU    256			; 256MB


KHwNorFlashBaseAddr     EQU    0x40000000
KHwNorFlashSize         EQU    0x04000000   ; 64MB
KHwNorFlashCodeSize     EQU    0x02000000   ; 32MB for code, 32MB for LFFS

;---------------------------------------------------------------------------
;  Physical Addresses of Peripherals
;---------------------------------------------------------------------------

KHwBaseRegisters	EQU     0xC0000000

KHwBaseSic		EQU	KHwBaseRegisters + 0x00000
KHwBaseRtc		EQU	KHwBaseRegisters + 0x01000
KHwBaseTimer		EQU	KHwBaseRegisters + 0x02000
KHwBaseKmiKeyboard	EQU     KHwBaseRegisters + 0x03000
KHwBaseKmiPointer	EQU     KHwBaseRegisters + 0x04000
KHwBaseClcd		EQU	KHwBaseRegisters + 0x05000
KHwBaseUart0		EQU	KHwBaseRegisters + 0x06000
KHwBaseUart1		EQU	KHwBaseRegisters + 0x07000
KHwBaseUart2		EQU	KHwBaseRegisters + 0x08000
KHwBaseUart3		EQU     KHwBaseRegisters + 0x09000
KHwBaseHostFs		EQU     KHwBaseRegisters + 0x0a000
KHwBaseSnap		EQU     KHwBaseRegisters + 0x0b000
KHwBaseNet		EQU     KHwBaseRegisters + 0x0c000
KHwBaseNand		EQU     KHwBaseRegisters + 0x0d000
KHwBaseAudio		EQU     KHwBaseRegisters + 0x0e000
KHwBaseWebcamera	EQU     KHwBaseRegisters + 0x0f000
KHwNVMemoryDevice			EQU		KHwBaseRegisters + 0x10000
; NTT Docomo - Defect 1291 fix - E32test t_tock.exe failed to load Logical Device - start

KHwBaseTimer2		EQU	KHwBaseRegisters + 0x11000
KHwBasePlatform		EQU     KHwBaseRegisters + 0x12000
; NTT Docomo - Defect 1291 fix - E32test t_tock.exe failed to load Logical Device - end

;-----------------------------------------------------------------------------
; Module linear bases
;-----------------------------------------------------------------------------

; Register group linear separation: 4K
KHwLinSeparation	EQU	0x1000

KHwLinBaseRegisters	EQU	KPrimaryIOBase

KHwLinBaseSic		EQU	KHwLinBaseRegisters + 0x00*KHwLinSeparation
KHwLinBaseRtc		EQU	KHwLinBaseRegisters + 0x01*KHwLinSeparation
KHwLinBaseTimer		EQU	KHwLinBaseRegisters + 0x02*KHwLinSeparation
KHwLinBaseKmiKeyboard	EQU	KHwLinBaseRegisters + 0x03*KHwLinSeparation   
KHwLinBaseKmiPointer    EQU	KHwLinBaseRegisters + 0x04*KHwLinSeparation
KHwLinBaseClcd		EQU	KHwLinBaseRegisters + 0x05*KHwLinSeparation
KHwLinBaseUart0		EQU	KHwLinBaseRegisters + 0x06*KHwLinSeparation
KHwLinBaseUart1		EQU	KHwLinBaseRegisters + 0x07*KHwLinSeparation
KHwLinBaseUart2		EQU	KHwLinBaseRegisters + 0x08*KHwLinSeparation
KHwLinBaseUart3		EQU	KHwLinBaseRegisters + 0x09*KHwLinSeparation
KHwLinBaseHostFs	EQU	KHwLinBaseRegisters + 0x0a*KHwLinSeparation
KHwLinBaseSnap		EQU	KHwLinBaseRegisters + 0x0b*KHwLinSeparation
KHwLinBaseNet		EQU	KHwLinBaseRegisters + 0x0c*KHwLinSeparation
KHwLinBaseNand		EQU	KHwLinBaseRegisters + 0x0d*KHwLinSeparation
KHwLinBaseAudio		EQU	KHwLinBaseRegisters + 0x0e*KHwLinSeparation
KHwLinBaseWebcameraDevice	EQU	KHwLinBaseRegisters + 0x0f*KHwLinSeparation
KHwLinBaseNVMemory	EQU	KHwLinBaseRegisters + 0x10*KHwLinSeparation
; NTT Docomo - Defect 1291 fix - E32test t_tock.exe failed to load Logical Device - start

KHwLinBaseTimer2		EQU	KHwLinBaseRegisters + 0x11*KHwLinSeparation
KHwLinBasePlatform	EQU	KHwLinBaseRegisters + 0x12*KHwLinSeparation
; NTT Docomo - Defect 1291 fix - E32test t_tock.exe failed to load Logical Device - end
	
	END
