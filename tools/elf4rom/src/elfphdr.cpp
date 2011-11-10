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

#include "elfphdr.h"

ElfPHdr & ElfPHdr::operator=(const ElfPHdr & aPHdr){
	iOffset = aPHdr.iOffset;
	iElf32Phdr = aPHdr.iElf32Phdr;
	return *this;
}

ElfPHdr::ElfPHdr(const ElfPHdr & aPHdr){
	*this = aPHdr;
}

#if 0
void ElfPHdr::InitRom(RomDetails * aRomDetails, size_t aOffset){
	iElf32Phdr.p_type = PT_LOAD;
	iElf32Phdr.p_offset = aOffset;
	iElf32Phdr.p_vaddr = aRomDetails->iRomBaseLinearAddr;
	iElf32Phdr.p_paddr = aRomDetails->iRomPhysAddr;
	iElf32Phdr.p_align = 4;
	iElf32Phdr.p_flags = PF_X + PF_R;
	iElf32Phdr.p_filesz = iElf32Phdr.p_memsz = iRomImage.Size();
}
#endif

void ElfPHdr::GetFileFragmentData(FileFragmentData & aFileFragmentData ){
	SetFileFragmentData(aFileFragmentData, Size(), reinterpret_cast<char *>(&iElf32Phdr));
}

