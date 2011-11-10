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

#ifndef ELFPHDR_H_
#define ELFPHDR_H_
#include <libelf.h>

#include "romdetails.h"
#include "elfheader.h"
#include "e32romimage.h"
#include "filefragment.h"
#include "outputfile.h"

class ElfPHdr : public FileFragmentOwner {
public:
	ElfPHdr(){}
	
	ElfPHdr(const ElfPHdr & aPHdr);
	
	ElfPHdr & operator=(const ElfPHdr & aPHdr);
	
	Elf32_Word GetPhdrFilesz(){ return iElf32Phdr.p_filesz; }
	Elf32_Word GetPhdrOffset(){ return iElf32Phdr.p_offset; }
	Elf32_Addr GetPhdrPaddr(){ return iElf32Phdr.p_paddr; }

	void SetPhdrType(Elf32_Word x) { iElf32Phdr.p_type = x; }
	void SetPhdrOffset(Elf32_Off x) { iElf32Phdr.p_offset = x; }
	void SetPhdrVaddr(Elf32_Addr x) { iElf32Phdr.p_vaddr = x; }
	void SetPhdrPaddr(Elf32_Addr x) { iElf32Phdr.p_paddr = x; }
	void SetPhdrFilesz(Elf32_Word x) { iElf32Phdr.p_filesz = x; }
	void SetPhdrMemsz(Elf32_Word x) { iElf32Phdr.p_memsz = x; }
	void SetPhdrFlags(Elf32_Word x) { iElf32Phdr.p_flags = x; }
	void SetPhdrAlign(Elf32_Word x) { iElf32Phdr.p_align = x; }
	
	void InitRom(RomDetails * aRomDetails, size_t aOffset);
	
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData );
	virtual size_t Size() { return sizeof(Elf32_Phdr); };
	virtual void DeleteFileFragmentData() {};

private:

#if 0
	typedef struct {
	    Elf32_Word		p_type;
	    Elf32_Off		p_offset;
	    Elf32_Addr		p_vaddr;
	    Elf32_Addr		p_paddr;
	    Elf32_Word		p_filesz;
	    Elf32_Word		p_memsz;
	    Elf32_Word		p_flags;
	    Elf32_Word		p_align;
	} Elf32_Phdr;
#endif
	Elf32_Phdr	iElf32Phdr;
};

#endif /*ELFPHDR_H_*/
