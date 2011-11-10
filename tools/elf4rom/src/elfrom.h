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

#ifndef ELFROM_H_
#define ELFROM_H_

#include <vector>
//
// Copyright (c) 2008 Symbian Software Ltd. All rights reserved.
//

#include <libelf.h>

#include "romdetails.h"
#include "outputfile.h"
#include "elfheader.h"
#include "e32romimage.h"
#include "elfphdr.h"
#include "elfsectionmanager.h"
#include "elfsymboltablemanager.h"
#include "dwarfmanager.h"

class ElfRom {
public:
	ElfRom(RomDetails * aRomDetails) :
		iRomDetails(aRomDetails), iOutputFile(aRomDetails->iElfRomFile),
		iE32RomImage(aRomDetails->iRomFile),
		iElfSectionManager(iElf32Header, iE32RomImage, iOutputFile),
		iElfSymbolTableManager(iElf32Header, iE32RomImage, iOutputFile, iElfSectionManager),
		iDwarfManager(iElfSectionManager, aRomDetails, iOutputFile),
		iDwarfFound(false)
		{}
	void SetupE32RomData();
	void SetupELFRomData();
	void AddData();
	void Dump();

private:
	typedef std::vector< Elf32_Shdr * > Elf32_Shdr_List;
	
private:
	void SetupRomElf32_EHdr();
	void SetupRomImage();
	void SetupProgramHeaderTable();
	size_t AddBootStrapProgramHeader();
	size_t SetupProgramHeaders(XIPFileDetails & aXIPFileDetails, size_t offset, int addend);
	void SetupAuxilarySections();
	void SetupLogFile();
	void SetupSectionHeaderTable();
	void SetupSectionHeaders(XIPFileDetails & aXIPFileDetails);
	void SetupSectionHeaders(XIPFileDetails & aXIPFileDetails, Elf * e, size_t shstrndx);
	void SetUpSegmentInfo(XIPFileDetails & aXIPFileDetails, Elf * e);
	size_t AddRoSectionHeader(XIPFileDetails & aXIPFileDetails, Elf32_Shdr * aShdr, char * aName);
	size_t AddRwSectionHeader(XIPFileDetails & aXIPFileDetails, Elf32_Shdr * aShdr, char * aName);
	size_t AddBssSectionHeader(XIPFileDetails & aXIPFileDetails, Elf32_Shdr * aShdr, char * aName);
	size_t AddROMSectionHeader(XIPFileDetails & aXIPFileDetails, Elf32_Shdr * aShdr, char * aName, size_t aOffset);
	size_t AddFinalHeader(size_t offset);
	void SetupSymbolTable();
	void SetupDwarfSections();
	
private:
	RomDetails * iRomDetails;
	OutputFile iOutputFile;
	
	Elf32Header iElf32Header;
	E32RomImage iE32RomImage;

	ElfPHdr	iBootStrapPHdr;

	typedef std::vector<ElfPHdr> ElfPHdrList;
	ElfPHdrList iElfPHdrList;
	
	ElfSectionManager iElfSectionManager;
	ElfSymbolTableManager iElfSymbolTableManager;
	DwarfManager iDwarfManager;
	
	bool iDwarfFound;

};
#endif /*ELFROM_H_*/
