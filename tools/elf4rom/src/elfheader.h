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

#ifndef ELFHEADER_H_
#define ELFHEADER_H_

#include "elfdefs.h"
#include "filefragment.h"
//#include "outputfile.h"

class Elf32Header : public FileFragmentOwner {
public:
	Elf32Header(){};
	Elf32Header(Elf32_Ehdr & aHeader);
	
	void SetEntryPoint(Elf32_Addr addr) { iHdr.e_entry = addr; }
	void SetProgramHdrOffset(Elf32_Off aOff) { iHdr.e_phoff = aOff; };
	void SetSectionHdrOffset(Elf32_Off aOff) { iHdr.e_shoff = aOff; };
	void AddProgramHdr() { iHdr.e_phnum++; };
	void AddSectionHdr() { iHdr.e_shnum++; };
	void SetSectionStringNdx(Elf32_Half ndx) { iHdr.e_shstrndx = ndx; };

	// FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData );
	virtual size_t Size() { return sizeof(iHdr); };
	virtual void DeleteFileFragmentData() {};

private:
	Elf32_Ehdr iHdr;
};

#endif /*ELFHEADER_H_*/
