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

#include <iostream>
#include "dwarfmanager.h"
#include "inputfile.h"

const string DwarfRangesManager::iRangesSectionName(".debug_ranges");

void DwarfRangesManager::ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr aStart, Dwarf_Byte_Ptr aEnd){
	Dwarf_Byte_Ptr start = aStart;
	Dwarf_Byte_Ptr end = aEnd;
	size_t pointer_size = iDwarfInfoManager.GetPointerSize();
	while (start < end){
		// TODO: this doesn't deal with 64-bit Dwarf
		Dwarf_Word w1 = GetValue(start, pointer_size);
		start+= pointer_size;
		if (w1 == 0xffffffff){
			LinearAddr addr = GetValue(start, pointer_size);
			LinearAddr relocatedAddress = aPair.iXIPFileDetails.Relocate(addr);
			if (addr != relocatedAddress)
				WriteValue(start, relocatedAddress, pointer_size);
		} 
		start += pointer_size;
	}
}
