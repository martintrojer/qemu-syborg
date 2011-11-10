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

#include "elfheader.h"

Elf32Header::Elf32Header(Elf32_Ehdr & aHdr){
	iHdr = aHdr;
	iHdr.e_entry = 0x0;
	iHdr.e_phoff = iHdr.e_shoff = iHdr.e_shoff = 0;
	iHdr.e_phnum = iHdr.e_shnum = iHdr.e_shstrndx = 0;
	iHdr.e_ehsize = sizeof(Elf32_Ehdr);
	// Let's say this is an executable rather than a shared object.
	iHdr.e_type = ET_EXEC;
}

void Elf32Header::GetFileFragmentData(FileFragmentData & aFileFragmentData ){
	SetFileFragmentData(aFileFragmentData, Size(), reinterpret_cast<char *>(&iHdr));
}


