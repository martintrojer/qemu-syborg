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

const string DwarfPubnamesManager::iPubnamesSectionName(".debug_pubnames");
const string DwarfPubtypesManager::iPubtypesSectionName(".debug_pubtypes");

void DwarfNameManager::ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr aStart, Dwarf_Byte_Ptr aEnd){
	Dwarf_Byte_Ptr start = aStart;
	Dwarf_Byte_Ptr end = aEnd;
	while (start < end){
		Dwarf_Byte_Ptr data = start;
		size_t offset_size, initial_length_size;

		Dwarf_Word length = READ_UNALIGNED4(data); 
		data += 4;

		if (length >= 0xfffffff0u) {
			cerr << "Error: 64 bit DWARF not supported\n";
			exit(EXIT_FAILURE);
		} else {	
			offset_size = 4;
			initial_length_size = 4;
		}

		Dwarf_Half version = READ_UNALIGNED2(data);
		data += 2;

		Dwarf_Word offset = GetValue(data, offset_size); 
		Dwarf_Word newOffset = CheckNewOffset(iDwarfInfoManager.GetSectionOffset(aPair.iXIPFileDetails.iElfFile), offset);
		if (offset != newOffset)
			WriteValue(data, offset, offset_size);
		data += offset_size;


		//Dwarf_Word size = GetValue(data, offset_size);
		// Don't need the length field
		data += offset_size;

		start += length + initial_length_size;

		if (version != 2 && version != 3){
			static bool warned = false;
			if (!warned){
				cerr << "Only DWARF 2 and 3 pubnames are currently supported\n";
				warned = true;
			}

			continue;
		}

		do {
			offset = GetValue(data, offset_size);

			if (offset != 0)
			{
				data += offset_size;
				data += strlen ((char *) data) + 1;
			}
		}
		while (offset != 0);
	}
}
