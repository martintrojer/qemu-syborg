/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
* 
* You should have received a copy of the GNU Lesser General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ROMDETAILS_H_
#define ROMDETAILS_H_

#include <vector>

#include "defs.h"

class XIPFileDetails {
public:
	XIPFileDetails():
		iE32File(""), 
		iElfFile(""), 
		iLoadAddr(0x0),
		iROAddr(0x0), 
		iROSize(0),
		iRWAddr(0x0),
		iRWSize(0),
		iROMDataAddr(0), 
		iBSSAddr(0),
		iBSSDataSize(0),
		iElfTextBase(0),
		iElfTextLimit(0),
		iElfDataBase(0),
		iRVCTProduced(false),
		iGCCProduced(false)
		{};
	XIPFileDetails(PathName & aE32File,PathName & aELFFile, LinearAddr load, LinearAddr text, size_t textSize,
			VirtualAddr data, size_t dataSize, LinearAddr aRomData, VirtualAddr bss,
			size_t bssDataSize):
		iE32File(aE32File), 
		iElfFile(aELFFile),
		iLoadAddr(load),
		iROAddr(text),
		iROSize(textSize),
		iRWAddr(data),
		iRWSize(dataSize),
		iROMDataAddr(aRomData), 
		iBSSAddr(bss),
		iBSSDataSize(bssDataSize),
		iElfTextBase(0),
		iElfTextLimit(0),
		iElfDataBase(0),
		iRVCTProduced(false),
		iGCCProduced(false)
		{};	
	XIPFileDetails(char * aE32File,char * aELFFile, LinearAddr load, LinearAddr text, size_t textSize,
			VirtualAddr data, size_t dataSize, LinearAddr aRomData, VirtualAddr bss, 
			size_t bssDataSize):
		iE32File(aE32File), 
		iElfFile(aELFFile), 
		iLoadAddr(load),
		iROAddr(text),
		iROSize(textSize),
		iRWAddr(data),
		iRWSize(dataSize),
		iROMDataAddr(aRomData), 
		iBSSAddr(bss),
		iBSSDataSize(bssDataSize),
		iElfTextBase(0),
		iElfTextLimit(0),
		iElfDataBase(0),
		iRVCTProduced(false),
		iGCCProduced(false)
		{};	
		
	LinearAddr Relocate(LinearAddr addr){
		if ((addr >= iElfTextBase) && (addr < iElfTextLimit)){
			size_t offset = addr - iElfTextBase;
			return iROAddr + offset;
		} else if ((addr >= iElfDataBase) && (addr < (iElfDataBase + iBSSDataSize))) {
			size_t offset = addr - iElfDataBase;
			return iRWAddr + offset;
		} else {
			return addr;
		}
	}
	PathName iE32File;
	PathName iElfFile;
	LinearAddr iLoadAddr;
	LinearAddr iROAddr;
	size_t iROSize;
	VirtualAddr iRWAddr;
	size_t iRWSize;
	LinearAddr iROMDataAddr;
	VirtualAddr iBSSAddr;
	size_t iBSSDataSize;
	VirtualAddr iElfTextBase;
	VirtualAddr iElfTextLimit;
	VirtualAddr iElfDataBase;
	// This is dodgy. We probably need a better way of coping with 'oddities'
	bool iRVCTProduced;
	bool iGCCProduced;
};

class RomDetails {
public:
	RomDetails():
		iBoardName(""),
		iRomFile(""),
		iElfRomFile(""),
		iLogFile(""),
		iDrive(""),
		iRomPhysAddr(0),
		iRomBaseLinearAddr(0),
		iKernelDataVirtualAddr(0),
		iNoDwarf(false),
		iSearch(false),
		iStrip(false),
		iTrace(false),
		iVerbosity(0)
		{}
		
	typedef std::vector<XIPFileDetails> XIPFileList;
	String iBoardName;
	PathName iRomFile;
	PathName iElfRomFile;
	PathName iLogFile;
	PathName iDrive;
	LinearAddr iRomPhysAddr;
	LinearAddr iRomBaseLinearAddr;
	VirtualAddr iKernelDataVirtualAddr;
	XIPFileDetails iPrimary;
	XIPFileList	iXIPFiles;
	bool iNoDwarf;
	bool iSearch;
	bool iStrip;
	bool iTrace;
	unsigned int iVerbosity;
	std::vector<std::string> iTargetFiles;
	std::vector<std::string> iExcludeFiles;

};
#endif /*ROMDETAILS_H_*/
