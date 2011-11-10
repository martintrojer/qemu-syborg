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

const string DwarfFrameManager::iFrameSectionName(".debug_frame");

static inline size_t SizeOfEncodedValue (int encoding)
{
  switch (encoding & 0x7)
    {
    default:	/* ??? */
    case 0:	return ENCODED_POINTER_SIZE;
    case 2:	return 2;
    case 3:	return 4;
    case 4:	return 8;
    }
}

static inline bool ValueFitsSize(Dwarf_Word val, size_t size){
	switch (size){
	default:
	case 0: {
		cerr << "Error: size of " << size << " not allowed\n";
		exit(EXIT_FAILURE);
		}
	case 2:
		return val <= 0xffff;
	case 4:
		return val <= 0xffffffff;
	case 8:
		return true;
	}
}


void DwarfFrameManager::ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr aStart, Dwarf_Byte_Ptr aEnd){
	CiePtrEncodingMap ptrEncodingMap;
	CieAugmentationMap augmentationMap;
	size_t encoded_ptr_size = ENCODED_POINTER_SIZE;
	size_t bytes_read;
	Dwarf_Byte_Ptr start = aStart;
	Dwarf_Byte_Ptr section_start = start;
	Dwarf_Byte_Ptr end = aEnd;
	while (start < end) {
		unsigned char *augmentation_data = NULL;
		unsigned long augmentation_data_len = 0;
		size_t offset_size;
		size_t initial_length_size;

		Dwarf_Byte_Ptr saved_start = start;
		Dwarf_Word length = READ_UNALIGNED4(start); 
		start += 4;

		if (length == 0){ // ZERO terminator - shouldn't see this
			continue;
		}	

		if (length >= 0xfffffff0u) {
			cerr << "Error: 64 bit DWARF not supported\n";
			exit(EXIT_FAILURE);
		} else {	
			offset_size = 4;
			initial_length_size = 4;
		}

		Dwarf_Byte_Ptr block_end = saved_start + length + initial_length_size;
		if (block_end > end) {
			cerr << "Warning: Invalid length " << length << " in FDE at 0x" 
				<< (unsigned long)(saved_start - section_start) << " in file " << aPair.iXIPFileDetails.iElfFile << "\n";
			block_end = end;
		}	
		Dwarf_Word cie_id = READ_UNALIGNED4(start); 
		if (cie_id != (Dwarf_Word)DW_CIE_ID)
			WRITE_UNALIGNED4(start, cie_id + GetSectionOffset(aPair.iXIPFileDetails.iElfFile));
		start += offset_size;

		if (cie_id == (Dwarf_Word)DW_CIE_ID) {
			Dwarf_Ubyte version = *start++;

			char * augmentation = (char *) start;
			augmentation_data = NULL;
			start = (Dwarf_Byte_Ptr) strchr ((char *) start, '\0') + 1;

			if (augmentation[0] == 'z'){
				ULEB128(start, bytes_read);
				ULEB128(start, bytes_read);

				if (version == 1){
					// fc->ra = GET (1);
					start++;
				} else {
					// fc->ra = LEB ();
					ULEB128(start, bytes_read);
				}
				augmentation_data_len = ULEB128(start, bytes_read);
				augmentation_data = start;
				augmentationMap[saved_start] = augmentation_data_len;
				start += augmentation_data_len;
			} else if (strcmp (augmentation, "eh") == 0){
				//start += eh_addr_size;
				start += 4;
				// fc->code_factor = LEB ();
				// fc->data_factor = SLEB ();
				ULEB128(start, bytes_read);
				ULEB128(start, bytes_read);
				if (version == 1){
					//c->ra = GET (1);
					start++;
				} else {
					// fc->ra = LEB ();
					ULEB128(start, bytes_read);
				}
			} else {
				ULEB128(start, bytes_read);
				ULEB128(start, bytes_read);

				if (version == 1){
					// fc->ra = GET (1);
					start++;
				} else {
					// fc->ra = LEB ();
					ULEB128(start, bytes_read);
				}
			}

			if (augmentation_data_len){
				unsigned char *p, *q;
				p = (unsigned char *) augmentation + 1;
				q = augmentation_data;
				Dwarf_Ubyte encoding = 0;
				while (1){
					if (*p == 'L')
						q++;
					else if (*p == 'P')
						q += 1 + SizeOfEncodedValue(*q);
					else if (*p == 'R')
						encoding = *q++;
					else
						break;
					p++;
				}
				if (encoding)
					ptrEncodingMap[saved_start] = encoding;
			}
	  
		} else {
			Dwarf_Byte_Ptr look_for = section_start + cie_id;
			Dwarf_Ubyte encoding = 0;
			CiePtrEncodingMap::iterator iE = ptrEncodingMap.find(look_for);
			if (iE != ptrEncodingMap.end()){
				encoding = iE->second;
				encoded_ptr_size = SizeOfEncodedValue(encoding);
			}
			if ((encoding & 0x70) != DW_EH_PE_pcrel){
				// do the nasty
				LinearAddr addr = GetValue(start, encoded_ptr_size);
				LinearAddr relocatedAddr = aPair.iXIPFileDetails.Relocate(addr);
				if (ValueFitsSize(relocatedAddr, encoded_ptr_size)){
					WriteValue(start, relocatedAddr, encoded_ptr_size);
				} else {
					cerr << "Warning: relocated addresses in " << GetSectionName().c_str() 
					<< " section of " << aPair.iXIPFileDetails.iElfFile.c_str() 
					<< " too large for encoding. Backtraces may be misleading.\n";
					}
			}
			start += encoded_ptr_size;
			// skip the range size
			start += encoded_ptr_size;

			CieAugmentationMap::iterator iP =  augmentationMap.find(look_for);
			if (iP != augmentationMap.end()){
				ULEB128(start, bytes_read);
				start += bytes_read;
			}
		}
		
		Dwarf_Word tmp = 0;
		while (start < block_end){
			unsigned op, opa;
			op = *start++;
			opa = op & 0x3f;
			if (op & 0xc0)
				op &= 0xc0;
			switch (op){
			case DW_CFA_advance_loc:
				break;
			case DW_CFA_offset:
				ULEB128(start, bytes_read);
				break;
			case DW_CFA_restore:
				break;
			case DW_CFA_set_loc:
				start += encoded_ptr_size;
				break;
			case DW_CFA_advance_loc1:
				start += 1;
				break;
			case DW_CFA_advance_loc2:
				start += 2;
				break;
			case DW_CFA_advance_loc4:
				start += 4;
				break;
			case DW_CFA_offset_extended:
			case DW_CFA_val_offset:
				ULEB128(start, bytes_read);
				ULEB128(start, bytes_read);
				break;
			case DW_CFA_restore_extended:
			case DW_CFA_undefined:
			case DW_CFA_same_value:
				ULEB128(start, bytes_read);
				break;
			case DW_CFA_register:
			case DW_CFA_def_cfa:
				ULEB128(start, bytes_read);
				ULEB128(start, bytes_read);
				break;
			case DW_CFA_def_cfa_register:
			case DW_CFA_def_cfa_offset:
				ULEB128(start, bytes_read);
				break;
			case DW_CFA_def_cfa_expression:
				tmp = ULEB128(start, bytes_read);
				EditLocationExpression (start, encoded_ptr_size, tmp, aPair);
				start += tmp;
				break;
			case DW_CFA_expression: 
			case DW_CFA_val_expression: 
				ULEB128(start, bytes_read);
				tmp = ULEB128(start, bytes_read);
				EditLocationExpression (start, encoded_ptr_size, tmp, aPair);
				start += tmp;
				break;
#ifndef DW_CFA_offset_extended_sf
// seems to be type in dwarf.h
#define DW_CFA_offset_extended_sf 0x11
				//DW_CFA_cfa_offset_extended_sf
#endif
			case DW_CFA_offset_extended_sf:
			case DW_CFA_val_offset_sf:
			case DW_CFA_def_cfa_sf:
				ULEB128(start, bytes_read);
				ULEB128(start, bytes_read);
				break;
			case DW_CFA_def_cfa_offset_sf:
				ULEB128(start, bytes_read);
				break;
			case DW_CFA_MIPS_advance_loc8:
				start += 8;
				break;
			case DW_CFA_GNU_args_size:
				ULEB128(start, bytes_read);
				break;
			case DW_CFA_GNU_negative_offset_extended:
				ULEB128(start, bytes_read);
				ULEB128(start, bytes_read);
				break;
			default:
				break;
			}
		}
	}

}
