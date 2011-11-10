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

const string DwarfLineManager::iLineSectionName(".debug_line");

inline static size_t process_extended_line_op (FileShdrPair & aPair, Dwarf_Byte_Ptr data)
{
	size_t bytes_read;
	size_t len = (size_t)ULEB128(data, bytes_read);

	if (len == 0){
		cerr << "badly formed extended line op encountered!\n";
		// a length of 0 indicates a badly formed op and will force everything to be ignored until 'end_of_sequence'.
		return 0;
	}

	len += bytes_read;
	unsigned char op_code = *data++;
  	switch (op_code){
    case DW_LNE_end_sequence:
      	break;
    case DW_LNE_set_address: {
    	size_t size = len - bytes_read - 1;
		LinearAddr addr = GetValue(data, size);
		LinearAddr relocatedAddress = aPair.iXIPFileDetails.Relocate(addr);
		if (addr != relocatedAddress)
			WriteValue(data, relocatedAddress, size);
		break;
    	}
    case DW_LNE_define_file:
      	ULEB128(data, bytes_read);
      	ULEB128(data, bytes_read);
      	ULEB128(data, bytes_read);
      	break;     
    default:

    	break;
    }

  return len;
}

void DwarfLineManager::ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr aStart, Dwarf_Byte_Ptr aEnd){
	Dwarf_Byte_Ptr start = aStart;
	Dwarf_Byte_Ptr end = aEnd;
	size_t bytes_read = 0;
	while (start < end){
		Dwarf_Byte_Ptr hdrptr = start;		
		size_t offset_size, initial_length_size;
		
		Dwarf_Word length = READ_UNALIGNED4(hdrptr); 
		hdrptr += 4;

		if (length >= 0xfffffff0u) {
			cerr << "Error: 64 bit DWARF not supported\n";
			exit(EXIT_FAILURE);
		} else {	
			offset_size = 4;
			initial_length_size = 4;
		}
		
		Dwarf_Byte_Ptr end_of_sequence = start + length + initial_length_size;

		Dwarf_Half version = READ_UNALIGNED2(hdrptr);
		hdrptr += 2;

		if (version != 2 && version != 3){
			static bool warned = false;
			if (!warned){
				cerr << "Only DWARF 2 and 3 aranges are currently supported\n";
				warned = true;
			}
			return;
		}
		
		hdrptr += offset_size;
#if 0
		// Don't need the next four fields 
		Dwarf_Ubyte min_insn_length = *hdrptr++;   
		Dwarf_Ubyte default_is_stmt = *hdrptr++;
		Dwarf_Ubyte line_base = *hdrptr++;
		Dwarf_Ubyte line_range = *hdrptr++;
#endif
		hdrptr +=4;
		Dwarf_Ubyte opcode_base = *hdrptr++;

	    /* Skip the contents of the Opcodes table.  */
		Dwarf_Byte_Ptr standard_opcodes = hdrptr;
		start = standard_opcodes + opcode_base - 1;

		/* skip the contents of the Directory table.  */
		while (*start != 0){
			start += strlen ((char *) start) + 1;
		}
		/* Skip the NUL at the end of the table.  */
		start++;

		/* skip the contents of the File Name table.  */
		while (*start != 0){
			start += strlen ((char *) start) + 1;

			ULEB128(start, bytes_read);
			ULEB128(start, bytes_read);
			ULEB128(start, bytes_read);
		}
		/* Skip the NUL at the end of the table.  */
		start++;
	   
		while (start < end_of_sequence){
			unsigned char op_code = *start++;

			if (op_code >= opcode_base){
				continue;
			} else {
				switch (op_code){
// missing from dwarf.h - first byte of extended op codes is '0x0'
#define DW_LNS_extended_op 0x0
				case DW_LNS_extended_op:
					size_t n = process_extended_line_op (aPair, start);
					// if we don't understand the extended op skip to the end of the sequence :-(
					if (n == 0)
						start = end_of_sequence;
					else 
						start += n;
					break;

				case DW_LNS_copy:
					break;

				case DW_LNS_advance_pc:
				case DW_LNS_advance_line:
				case DW_LNS_set_file:
				case DW_LNS_set_column:
					ULEB128(start, bytes_read);
					break;

				case DW_LNS_negate_stmt:
				case DW_LNS_set_basic_block:
				case DW_LNS_const_add_pc:
					break;

				case DW_LNS_fixed_advance_pc:
					start += 2;
					break;

				case DW_LNS_set_prologue_end:
				case DW_LNS_set_epilogue_begin:
					break;

				case DW_LNS_set_isa:
					ULEB128(start, bytes_read);
					break;

				default:
					for (int i = standard_opcodes[op_code - 1]; i > 0 ; --i){
						ULEB128(start, bytes_read);
					}
					break;
				}
			}
		}
		// !! eek ARM seems to require header word aligned - at least for Dwarf 2
		if (aPair.iXIPFileDetails.iRVCTProduced && (version == 2))
			start = (Dwarf_Byte_Ptr)(((unsigned long)start + 3) & ~3);
	}
}
