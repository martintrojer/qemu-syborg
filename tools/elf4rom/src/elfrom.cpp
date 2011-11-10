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

#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

#include "elfromerror.h"
#include "elfrom.h"
#include "inputfile.h"

#define NO_GAPS

void ElfRom::SetupE32RomData() {
	SetupRomElf32_EHdr();
	SetupRomImage();
}

// TODO: don't use primary file - fill header in by hand.
void ElfRom::SetupRomElf32_EHdr() {
	//create ELF header

	unsigned char c[EI_NIDENT] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS32, ELFDATA2LSB, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
	Elf32_Ehdr elf32_ehdr;

	for (int i=0; i <EI_NIDENT;i++)
		elf32_ehdr.e_ident[i] = c[i];

	elf32_ehdr.e_type		= ET_EXEC;
	elf32_ehdr.e_machine	= EM_ARM;
	elf32_ehdr.e_version	= EV_CURRENT;
	elf32_ehdr.e_entry		= iRomDetails->iRomPhysAddr;
	elf32_ehdr.e_shoff		= sizeof(Elf32_Ehdr);
	
// ARM specific flags 
// e_entry contains a program-loader entry point
#define EF_ARM_HASENTRY 0x02
// Each subsection of the symbol table is sorted by symbol value
#define EF_ARM_SYMSARESORTED 0x04
// Symbols in dynamic symbol tables that are defined in sections
// included in program segment n have st_shndx = n+ 1. 
#define EF_ARM_DYNSYMSUSESEGIDX 0x8
// Mapping symbols precede other local symbols in the symbol table
#define EF_ARM_MAPSYMSFIRST 0x10
// This masks an 8-bit version number, the version of the ARM EABI to
// which this ELF file conforms. This EABI is version 2. A value of 0
// denotes unknown conformance. (current version is 0x02000000)
#define EF_ARM_EABIMASK 0xFF000000

#define EF_ARM_EABI_VERSION 0x02000000
#define EF_ARM_BPABI_VERSION 0x04000000

	elf32_ehdr.e_flags		= EF_ARM_BPABI_VERSION | EF_ARM_HASENTRY;
	elf32_ehdr.e_ehsize		= sizeof(Elf32_Ehdr);
	elf32_ehdr.e_phentsize 	= sizeof(Elf32_Phdr);
	elf32_ehdr.e_shentsize 	= sizeof(Elf32_Shdr);
	elf32_ehdr.e_shnum		= 0;
	elf32_ehdr.e_shstrndx	= 0;
	elf32_ehdr.e_phnum		= 02;
	new (&iElf32Header) Elf32Header(elf32_ehdr);
#if 0
	iElf32Header.SetEntryPoint(iRomDetails->iRomPhysAddr);
	elf_end(e);
	close(fd);
#endif
	iElf32Header.SetEntryPoint(iRomDetails->iRomPhysAddr);
	iElf32Header.AddData(iOutputFile);
	assert(iElf32Header.GetOffset() == 0);
	if (iRomDetails->iTrace){
		cout << "\nElf header added.\n";
	}

}


void ElfRom::SetupRomImage(){
	// ensure image size is known and we know how to get hold of it
	iE32RomImage.SetupRomData();
	iE32RomImage.AddData(iOutputFile);
	assert(iE32RomImage.GetOffset() == iElf32Header.Size());
	if (iRomDetails->iTrace){
		size_t offset = iE32RomImage.GetOffset();
		size_t size = iE32RomImage.Size();
		cout << "\nAdded ROM image " << iRomDetails->iRomFile << "\n";
		cout.fill('0');
		cout << hex << "      offset 0x" << setw(8)
			<< offset << " size = 0x" << setw(8) << size << "\n" ;
	}
}

void ElfRom::SetupProgramHeaderTable(){
	size_t offsetx = AddBootStrapProgramHeader();
	RomDetails::XIPFileList::iterator aXIPFile = iRomDetails->iXIPFiles.begin();
	RomDetails::XIPFileList::iterator end = iRomDetails->iXIPFiles.end();
	unsigned int p = iRomDetails->iRomPhysAddr;
	unsigned int v = iRomDetails->iRomBaseLinearAddr;
	int addend = v > p ? -(v - p) : p - v;
	
	if (iRomDetails->iTrace){
		cout << "\nAdding program headers for e32images\n";
	}
	
	while (aXIPFile != end) {
		offsetx = SetupProgramHeaders(*aXIPFile, offsetx, addend);
		aXIPFile++;
	}
	AddFinalHeader(offsetx);

#ifdef NO_GAPS
	// check there are no gaps or overlaps in the phdrs
	Elf32_Word bsz = iBootStrapPHdr.GetPhdrFilesz();
	Elf32_Word coffset = iBootStrapPHdr.GetPhdrOffset() + bsz;
	Elf32_Addr phys_addr = iBootStrapPHdr.GetPhdrPaddr() + bsz;
	ElfPHdrList::iterator aCheckHdr = iElfPHdrList.begin();
	ElfPHdrList::iterator endCheckHdr = iElfPHdrList.end();
	while(aCheckHdr != endCheckHdr) {
		Elf32_Word o = aCheckHdr->GetPhdrOffset();
		if (coffset != o){
			cerr << "Error: Phdr table broken - offsets incorrect\n";
			assert(coffset == o);
		}
		Elf32_Addr addr = aCheckHdr->GetPhdrPaddr();
		if (phys_addr > addr){
			cerr << "Error: Phdr table broken - physical addresses incorrect\n";
			assert(phys_addr <= addr);
		}
		size_t sz = aCheckHdr->GetPhdrFilesz();
		coffset = o + sz;
		phys_addr = addr + sz;
		aCheckHdr++;
	}
#endif
	
	ElfPHdrList::iterator aHdr = iElfPHdrList.begin();
	ElfPHdrList::iterator endHdr = iElfPHdrList.end();
	while (aHdr != endHdr) {
		aHdr->AddData(iOutputFile);
		aHdr++;
	}
}

size_t ElfRom::AddBootStrapProgramHeader(){
	iBootStrapPHdr.SetPhdrType(PT_LOAD);
	iBootStrapPHdr.SetPhdrOffset(iE32RomImage.GetOffset());
	iBootStrapPHdr.SetPhdrVaddr(iRomDetails->iRomPhysAddr);
	iBootStrapPHdr.SetPhdrPaddr(iRomDetails->iRomPhysAddr);
	iBootStrapPHdr.SetPhdrAlign(4);
	iBootStrapPHdr.SetPhdrFlags(PF_X + PF_R);
	size_t bootstrapsize = iRomDetails->iXIPFiles[0].iLoadAddr - iRomDetails->iRomBaseLinearAddr;
	iBootStrapPHdr.SetPhdrFilesz(bootstrapsize);
	iBootStrapPHdr.SetPhdrMemsz(bootstrapsize);
		
	iBootStrapPHdr.AddData(iOutputFile);
	assert((iElf32Header.Size() + iE32RomImage.Size()) == iBootStrapPHdr.GetOffset());
	iElf32Header.AddProgramHdr();
	iElf32Header.SetProgramHdrOffset(iBootStrapPHdr.GetOffset());
	if (iRomDetails->iTrace){
		size_t offset = iBootStrapPHdr.GetOffset();
		cout << "\nAdded PHdr for bootstrap\n";
		cout.fill('0');
		cout << hex << "      offset 0x" << setw(8) << offset 
			 << " size = 0x" << setw(8) << bootstrapsize << "\n";
	}

	return iE32RomImage.GetOffset() + bootstrapsize;
}

static inline size_t InitE32HdrPHdr(ElfPHdr & hdr, size_t offset, Elf32_Word vaddr, Elf32_Word paddr, Elf32_Word size){
	hdr.SetPhdrType(PT_LOAD);
	hdr.SetPhdrOffset(offset);
	hdr.SetPhdrVaddr(vaddr);
	hdr.SetPhdrPaddr(paddr);
	hdr.SetPhdrAlign(0);
	hdr.SetPhdrFlags(PF_R);
	hdr.SetPhdrFilesz(size);
	hdr.SetPhdrMemsz(size);
		
	return offset + size;	
	
}
static inline size_t InitROPHdr(ElfPHdr & hdr, size_t offset, Elf32_Word vaddr, Elf32_Word paddr, Elf32_Word size){
	hdr.SetPhdrType(PT_LOAD);
	hdr.SetPhdrOffset(offset);
	hdr.SetPhdrVaddr(vaddr);
	hdr.SetPhdrPaddr(paddr);
	hdr.SetPhdrAlign(4);
	hdr.SetPhdrFlags(PF_X + PF_R);
	hdr.SetPhdrFilesz(size);
	hdr.SetPhdrMemsz(size);
		
	return offset + size;	
	
}

static inline size_t InitRWPHdr(ElfPHdr & hdr, size_t offset, Elf32_Word vaddr, Elf32_Word paddr, Elf32_Word fsize, Elf32_Word msize){
	hdr.SetPhdrType(PT_LOAD);
	hdr.SetPhdrOffset(offset);
	hdr.SetPhdrVaddr(vaddr);
	hdr.SetPhdrPaddr(paddr);
	hdr.SetPhdrAlign(4);
	hdr.SetPhdrFlags(PF_X + PF_R);
	hdr.SetPhdrFilesz(fsize);
	hdr.SetPhdrMemsz(msize);
		
	return offset + fsize;	
	
}

size_t ElfRom::SetupProgramHeaders(XIPFileDetails & aXIPFileDetails, size_t offset, int addend){
	ElfPHdr e32hdr;
	Elf32_Word e32hdrVaddr = aXIPFileDetails.iLoadAddr;
	Elf32_Word e32hdrPaddr = aXIPFileDetails.iLoadAddr + addend;
	Elf32_Word e32hdrSize = aXIPFileDetails.iROAddr - aXIPFileDetails.iLoadAddr ;
	size_t e32hdrOffsetInRom = aXIPFileDetails.iLoadAddr - iRomDetails->iRomBaseLinearAddr;
	size_t e32hdrOffsetInElf = iE32RomImage.GetOffset() + e32hdrOffsetInRom;
#ifdef NO_GAPS
	// But actually the offset we'll use is the one we were given as an argument.
	// This means we need to adjust the size, vaddr and paddr by the difference
	size_t diff = e32hdrOffsetInElf - offset;
	e32hdrSize += diff;
	e32hdrVaddr -= diff;
	e32hdrPaddr -= diff;
	offset = InitE32HdrPHdr(e32hdr, offset, e32hdrVaddr, e32hdrPaddr, e32hdrSize);
#else
	offset = InitE32HdrPHdr(e32hdr, e32hdrOffsetInElf, e32hdrVaddr, e32hdrPaddr, e32hdrSize);
#endif
	iElf32Header.AddProgramHdr();
	iElfPHdrList.push_back(e32hdr);
	ElfPHdr thdr;
	Elf32_Word textVaddr = aXIPFileDetails.iROAddr;
	Elf32_Word textPaddr = aXIPFileDetails.iROAddr + addend;
	Elf32_Word textSize = aXIPFileDetails.iROSize;
	size_t textOffsetInRom = aXIPFileDetails.iROAddr - iRomDetails->iRomBaseLinearAddr;
	size_t textOffsetInElf = iE32RomImage.GetOffset() + textOffsetInRom;
	offset = InitROPHdr(thdr, textOffsetInElf, textVaddr, textPaddr, textSize);
	iElf32Header.AddProgramHdr();
	iElfPHdrList.push_back(thdr);

	if (iRomDetails->iTrace){
		cout << "  " << aXIPFileDetails.iE32File << "\n";
		cout.fill(' ');
		cout << left << "    .text\n";
		cout.fill('0');
		cout << "      paddr = 0x" << right << setw(8) << textPaddr << " vaddr = 0x" << right << setw(8)<< textVaddr
		     << " offset = 0x" << right << setw(8) << textOffsetInElf 
			 << " size = 0x" << setw(8) << textSize << "\n" << flush;
	}
	
	if (aXIPFileDetails.iRWSize > 0){
		ElfPHdr dhdr;
		Elf32_Word dataVaddr = aXIPFileDetails.iRWAddr;
		Elf32_Word dataPaddr = aXIPFileDetails.iROMDataAddr + addend;
		Elf32_Word fsize = aXIPFileDetails.iRWSize;
		Elf32_Word msize = aXIPFileDetails.iBSSDataSize;
		size_t dataOffsetInRom = aXIPFileDetails.iROMDataAddr - iRomDetails->iRomBaseLinearAddr;
		size_t dataOffsetInElf = iE32RomImage.GetOffset() + dataOffsetInRom;
		offset = InitRWPHdr(dhdr, dataOffsetInElf, dataVaddr, dataPaddr, fsize, msize);
		iElf32Header.AddProgramHdr();
		iElfPHdrList.push_back(dhdr);
		
		if (iRomDetails->iTrace){
			cout.fill(' ');
			cout << left << "    .data\n";
			cout.fill('0');
			cout << "      paddr = 0x" << right << setw(8) << dataPaddr << " vaddr = 0x" << right << setw(8)<< dataVaddr  
				 << " offset = 0x" << right << setw(8)<< dataOffsetInElf
				 << " size = 0x" << right << setw(8) << fsize << "\n" << flush;
		}
		
	}
	return offset;
}

// This adds a header for the remainder of the ROM image after the last XIP file
size_t ElfRom::AddFinalHeader(size_t offset){
	ElfPHdr & lastHdr = iElfPHdrList.back();
	Elf32_Word startAddr = lastHdr.GetPhdrPaddr() + lastHdr.GetPhdrFilesz();
	ElfPHdr e32hdr;
	Elf32_Word e32hdrVaddr = startAddr;
	Elf32_Word e32hdrPaddr = startAddr;
	Elf32_Word e32hdrSize = iE32RomImage.Size() - offset + iE32RomImage.GetOffset();

	offset = InitE32HdrPHdr(e32hdr, offset, e32hdrVaddr, e32hdrPaddr, e32hdrSize);

	iElf32Header.AddProgramHdr();
	iElfPHdrList.push_back(e32hdr);	
	
	if (iRomDetails->iTrace){
		cout << "\nAdded final PHdr\n" << "      offset 0x" << hex << right << setw(8) << offset 
			<< " size = 0x" << setw(8) << e32hdrSize << "\n";
	}
	return offset;
}

void ElfRom::SetupAuxilarySections(){
	SetupLogFile();
}

void ElfRom::SetupLogFile(){
	if (!iRomDetails->iLogFile.size()) return;
	InputFile * aLogFile = new InputFile(iRomDetails->iLogFile);
	ElfSectionFileData * aLogFileData = new ElfSectionFileData(aLogFile);
	Elf32_Shdr shdr;
	shdr.sh_name = 0; // for now.
	shdr.sh_offset = 0; // for now
	shdr.sh_info = 0;
	shdr.sh_link = SHN_UNDEF;
	shdr.sh_addr = 0;
	shdr.sh_addralign = 0;
	shdr.sh_type = SHT_PROGBITS;
	shdr.sh_size = 0; // for now.
	shdr.sh_flags = 0;
	shdr.sh_entsize = 0;

	ElfSection aLogFileSection(aLogFileData, "ROMLogFile", shdr);
	iElfSectionManager.AddSection(aLogFileSection);

}

void ElfRom::SetupELFRomData() {
	SetupProgramHeaderTable();
	SetupAuxilarySections();
	if (!iRomDetails->iStrip){
		SetupSectionHeaderTable();
		SetupSymbolTable();
		if (!iRomDetails->iNoDwarf){
			SetupDwarfSections();
		}
	}
}

void ElfRom::SetupSectionHeaderTable(){
	RomDetails::XIPFileList::iterator aXIPFile = iRomDetails->iXIPFiles.begin();
	RomDetails::XIPFileList::iterator end = iRomDetails->iXIPFiles.end();

	if (iRomDetails->iTrace && aXIPFile != end){
		cout << "\nAdding Section headers from associated ELF files\n";
	}

	while (aXIPFile != end) {
		SetupSectionHeaders(*aXIPFile);
		aXIPFile++;
	}
}

void ElfRom::SetupSectionHeaders(XIPFileDetails & aXIPFileDetails) {
	// Open ELF file
	PathName aPath = aXIPFileDetails.iElfFile;
	
	if (aPath.size() == 0) return;
	
	int fd;
	Elf * e;
	size_t shstrndx;
	bool hasSectionStringTable = true;


	if ((fd = open(aPath.c_str(), O_RDONLY|O_BINARY, 0)) < 0){
		warnx(EX_NOINPUT, "open \"%s\" failed\n", aPath.c_str());
		goto finish;
	}
	if ((e = elf_begin(fd, ELF_C_READ , NULL)) == NULL)
		errx(EX_SOFTWARE, "elf_begin() failed: %s.\n", elf_errmsg(-1));
	if (elf_kind(e) != ELF_K_ELF)
		errx(EX_SOFTWARE, "file not of kind ELF_K_ELF: %s.\n", aPath.c_str());
	if (elf_getshstrndx(e, &shstrndx) == 0) {
		hasSectionStringTable = false;
		warnx(EX_SOFTWARE, "getshstrndx() failed for \"%s\"\n", aPath.c_str());
	}
	if (hasSectionStringTable){
		SetUpSegmentInfo(aXIPFileDetails, e);
		SetupSectionHeaders(aXIPFileDetails, e, shstrndx);
	}

	elf_end(e);
	close(fd);	
finish:
	return;
}

void ElfRom::SetupSectionHeaders(XIPFileDetails & aXIPFileDetails, Elf * e, size_t shstrndx){
	// Iterate through sections looking for the ones we're after. Namely:
	// text, data, bss/zi, symtab, strtable,  and .debug*
	Elf_Scn * scn = NULL; 
	Elf32_Shdr * shdr;
	SectionNumberMap aSectionNumberMap;
	SectionVaddrAddendMap aSectionVaddrAddendMap;

	String aPath(aXIPFileDetails.iElfFile);
	
	const char * debugName = ".debug";
	const size_t debugNameLength = strlen(debugName);
	const char * staticStrTab = ".strtab";
	
	if (iRomDetails->iTrace){
		cout << "  " << aXIPFileDetails.iElfFile << "\n";
	}
	
	while ((scn = elf_nextscn(e, scn)) != NULL) {

    	if ((shdr = elf32_getshdr(scn)) == NULL)
    		errx(EX_SOFTWARE, "getshdr() failed: %s.\n", elf_errmsg(-1));
    	
		size_t aOldNdx = elf_ndxscn(scn);
    	char * name = elf_strptr(e, shstrndx, shdr->sh_name);
    	VirtualAddr sectionAddr = shdr->sh_addr;

    	switch (shdr->sh_type) {
    	case SHT_NOBITS:
    		// Check for BSS or ZI
    		if ((shdr->sh_flags & SHF_WRITE) && (shdr->sh_flags & SHF_ALLOC)) {
    			// set up section number mapping
    			size_t aNew = AddBssSectionHeader(aXIPFileDetails, shdr, name);
    			aSectionNumberMap.push_back(SectionNumberMapping(aOldNdx, aNew));
    			// set up address adjustment for relocation of e.g. symbols
    			int addend = aXIPFileDetails.iBSSAddr - sectionAddr;
    			aSectionVaddrAddendMap.push_back(SectionVaddrAddendMapping(aOldNdx, addend));
    		}
    		break;
#define ARM_EXIDX	(SHT_LOPROC + 1)
    	case ARM_EXIDX:    		
    	case SHT_PROGBITS:
    		// text/ro or data/rw will have SHF_ALLOC set
    		if (shdr->sh_flags & SHF_ALLOC) {
    			size_t aNew = 0;
    		    int addend = 0;
    		    if (shdr->sh_flags & SHF_WRITE) {
    				aNew = AddRwSectionHeader(aXIPFileDetails, shdr, name);
    				addend = aXIPFileDetails.iRWAddr - sectionAddr;
    		    } else {
    				aNew = AddRoSectionHeader(aXIPFileDetails, shdr, name);
    				addend = aXIPFileDetails.iROAddr - sectionAddr;
    		    }
    			// set up section number mapping
    		    aSectionNumberMap.push_back(SectionNumberMapping(aOldNdx, aNew)); 
    			// set up address adjustment for relocation of e.g. symbols
    		    aSectionVaddrAddendMap.push_back(SectionVaddrAddendMapping(aOldNdx, addend));     	
    		} else if (!iRomDetails->iNoDwarf && !strncmp(debugName, name, debugNameLength)) {
    			iDwarfFound = true;
    			iDwarfManager.AddSection(aXIPFileDetails, name, shdr);
    		}
    		break;
    	case SHT_SYMTAB:
    		iElfSymbolTableManager.AddSymbolTable(aPath, shdr->sh_offset, shdr->sh_size, shdr->sh_info);
    		break;
    	case SHT_STRTAB:
    		if (!strcmp(staticStrTab, name))
    			iElfSymbolTableManager.AddStringTable(aPath, shdr->sh_offset, shdr->sh_size);
    		break;
    	}
    }
	iElfSymbolTableManager.Finalize(aSectionNumberMap, aSectionVaddrAddendMap);
}

size_t ElfRom::AddRoSectionHeader(XIPFileDetails & aXIPFileDetails, Elf32_Shdr * aShdr, char * aName){
	size_t delta = aShdr->sh_addr - aXIPFileDetails.iElfTextBase;
	VirtualAddr vaddr = aXIPFileDetails.iROAddr + delta;
	size_t offsetInRom = vaddr - iRomDetails->iRomBaseLinearAddr;
	size_t offsetInElf = iE32RomImage.GetOffset() + offsetInRom;
	aShdr->sh_addr = vaddr;
	return AddROMSectionHeader(aXIPFileDetails, aShdr, aName, offsetInElf );
}

size_t ElfRom::AddRwSectionHeader(XIPFileDetails & aXIPFileDetails, Elf32_Shdr * aShdr, char * aName){		
	size_t delta = aShdr->sh_addr - aXIPFileDetails.iElfDataBase;
	VirtualAddr vaddr = aXIPFileDetails.iROMDataAddr + delta;
	size_t offsetInRom = vaddr - iRomDetails->iRomBaseLinearAddr;
	size_t offsetInElf = iE32RomImage.GetOffset() + offsetInRom;	
	aShdr->sh_addr = aXIPFileDetails.iRWAddr;
	return AddROMSectionHeader(aXIPFileDetails, aShdr, aName, offsetInElf);
}

size_t ElfRom::AddBssSectionHeader(XIPFileDetails & aXIPFileDetails, Elf32_Shdr * aShdr, char * aName){	
	size_t delta = aShdr->sh_addr - aXIPFileDetails.iElfDataBase;
	VirtualAddr vaddr = aXIPFileDetails.iROMDataAddr + delta;
	size_t offsetInRom = vaddr - iRomDetails->iRomBaseLinearAddr;
	size_t offsetInElf = iE32RomImage.GetOffset() + offsetInRom;	
	aShdr->sh_addr = aXIPFileDetails.iBSSAddr;
	return AddROMSectionHeader(aXIPFileDetails, aShdr, aName, offsetInElf);
}

size_t ElfRom::AddROMSectionHeader(XIPFileDetails & aXIPFileDetails, Elf32_Shdr * aShdr, char * aName, size_t aOffset){	
	ElfSectionData * romData = aOffset ? 
								(ElfSectionData *)new ElfSectionRomData(aOffset, aShdr->sh_size) 
								: (ElfSectionData *)new ElfSectionNoData();
	ElfSection aSection(romData, aName, *aShdr);
	iElfSectionManager.AddSection(aSection);
	
	if (iRomDetails->iTrace){
		cout.fill(' ');
		cout << "    " << left << setw(22) << aName << "\n";
		cout.fill('0');
		cout << "      vaddr = 0x" << right << hex << setw(8) << aShdr->sh_addr << " offset = 0x" 
			 << right << hex << setw(8) << aOffset 
			 << " size = 0x" << right << hex << setw(8) << aShdr->sh_size << "\n";
	}
	return aSection.GetIndex();
}

void ElfRom::SetUpSegmentInfo(XIPFileDetails & aXIPFileDetails, Elf * e){
	Elf32_Ehdr * ehdr = elf32_getehdr(e);
	if (ehdr == NULL)
		errx(EX_SOFTWARE, "elf32_getehdr() failed: %s.", elf_errmsg(-1));
	size_t n = ehdr->e_phnum;
	Elf32_Phdr * phdr = elf32_getphdr(e);
	if (phdr == NULL)
		errx(EX_SOFTWARE, "elf32_getphdr() failed: %s.", elf_errmsg(-1));

	for (size_t i = 0; i < n; i++) {
		if (phdr[i].p_flags & PF_X){
			VirtualAddr segmentAddr = phdr[i].p_vaddr;
	    	aXIPFileDetails.iElfTextBase = segmentAddr;
	    	aXIPFileDetails.iElfTextLimit = segmentAddr + phdr[i].p_memsz;
			
		} else if (phdr[i].p_flags & PF_W){
	    	aXIPFileDetails.iElfDataBase = phdr[i].p_vaddr;
		}
	}	
}

void ElfRom::SetupSymbolTable(){
	iElfSymbolTableManager.AddSymbolTable();
	if (iRomDetails->iTrace){
		cout << "\nAdded section headers for combined symbol table and symbol string table\n";
	}
}

void ElfRom::SetupDwarfSections(){
	if (iRomDetails->iTrace && iDwarfFound){
		cout << "\nSetting up Dwarf Sections\n";
	} else if (iRomDetails->iTrace && !iDwarfFound && !iRomDetails->iNoDwarf){
		cout << "\nWarning: No Dwarf information found\n";
		return;
	}
	iDwarfManager.SetupSections();
}

void ElfRom::AddData() {
	iElfSectionManager.AddData();
	iElf32Header.SetSectionHdrOffset(iElfSectionManager.GetOffset());
}

void ElfRom::Dump(){
	iOutputFile.Dump();
	if (iRomDetails->iTrace){
		cout << "\nWrote " << iRomDetails->iElfRomFile << " " << dec << iOutputFile.Size() << " bytes\n";
	}	
}
