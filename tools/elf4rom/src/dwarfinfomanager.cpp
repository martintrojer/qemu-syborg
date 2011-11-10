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

#include <string.h>
#include <iostream>
#include <cassert>

#include "dwarfmanager.h"
#include "inputfile.h"

const string DwarfInfoManager::iInfoSectionName(".debug_info");

void DwarfInfoManager::ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr aStart, Dwarf_Byte_Ptr aEnd){
	iDwarfAbbrevManager.StartContext(aPair.iXIPFileDetails.iElfFile);
	Dwarf_Byte_Ptr p = aStart;
	Dwarf_Byte_Ptr e = aEnd;
	while (p < e) {
		p = ProcessCU(aPair, p, e);
	}
	iDwarfAbbrevManager.EndContext();
}

#define READ_DWARF_VAL(v,t,p) t v = *((t *)p); p += sizeof(t);
Dwarf_Byte_Ptr DwarfInfoManager::ProcessCU(FileShdrPair & aPair, Dwarf_Byte_Ptr s, Dwarf_Byte_Ptr e){
	Dwarf_Byte_Ptr p = s;
	//Read the CU header info
	// first check whether we're dealing with 32bit or 64 bit DWARF
	
	// TODO: must abstract over base types e.g uint32 etc.
	// TODO: this might not be 4 byte aligned and so could cause problems on some
	// architectures
	READ_DWARF_VAL(len, Uint32, p);
	if (len >= 0xfffffff0u){
		cerr << "Error: 64 bit DWARF not supported\n";
		exit(EXIT_FAILURE);
	}
	iLocalLength = len;
	Dwarf_Byte_Ptr end = p + len;
	// TODO: make sensitive to dwarf version number?
	READ_DWARF_VAL(version, Dwarf_Half, p);
	Uint32 abbrevOffset = *((Uint32 *)p);
	// update the offset into the abbrev table
	*((Uint32 *)p) = (Uint32)(abbrevOffset + iDwarfAbbrevManager.GetContextSectionOffset());
	p += sizeof(Uint32);
	READ_DWARF_VAL(address_size, Byte, p);
	
	// TODO: if this isn't 4 we're doomed aren't we?
	iAddressSize = address_size;
	
	iDwarfAbbrevManager.SetContextAbbrevOffset(abbrevOffset);
	
	// now process each DIE until end.
	while (p < end) {
		p = ProcessDIE(aPair, p, end);
	}

	return p;
}

void NoteProducer(FileShdrPair & aPair, Dwarf_Byte_Ptr aPtr, Dwarf_Unsigned aForm){

	switch (aForm){
	case DW_FORM_string:{
		const char * producer = (const char *)aPtr;
		const char * RvctProducer = "ARM/Thumb C/C++ Compiler, RVCT";
		const size_t RvctProducerLength = strlen(RvctProducer);
		const char * GccProducer = "GNU C++";
		const size_t GccProducerLength = strlen(GccProducer);
		if (!strncmp(producer, RvctProducer, RvctProducerLength))
			aPair.iXIPFileDetails.iRVCTProduced = true;
		if (!strncmp(producer, GccProducer, GccProducerLength))
			aPair.iXIPFileDetails.iGCCProduced = true;		
		return;
	}
    case DW_FORM_indirect: {
	    size_t indir_len = 0;
	    Dwarf_Unsigned form_indirect = DwarfSectionManager::DecodeUnsignedLeb128(aPtr, indir_len);
	    if (form_indirect == DW_FORM_indirect) {
			/* 	Eek, should never happen, will get warned later */
	    	return;
	    }
	    return NoteProducer(aPair, aPtr+indir_len, form_indirect);
		}
    case DW_FORM_strp:
    	// TODO - We need to get the string table for this -- 
    	return;
	}
}

Dwarf_Byte_Ptr DwarfInfoManager::ProcessDIE(FileShdrPair & aPair, Dwarf_Byte_Ptr s, Dwarf_Byte_Ptr e){
	Dwarf_Byte_Ptr info_ptr = s;
    size_t leb128_length;
    
    Dwarf_Word abbrev_code = DecodeUnsignedLeb128(info_ptr, leb128_length);
    info_ptr += leb128_length;
    if (abbrev_code == 0) {
    	return info_ptr;
    }

    DebugAbbrev & abbrev = iDwarfAbbrevManager.GetAbbrev(abbrev_code);
 
    //cout << "Tag = " << GetDwarfTag(abbrev.iTag) << " Code = " << abbrev_code <<  " num attrs = " << abbrev.iCount << endl;
   
    for (size_t i = 0; i < abbrev.iCount; i++){
    	size_t attr = abbrev.iParsed[i].iAttr;
    	Dwarf_Unsigned form = abbrev.iParsed[i].iForm;
    	//cout << "\tAttr " << GetDwarfAttr(attr) << " Form " << GetDwarfForm(form) << "\n";
    	
    	// record anything interesting about the producer here.
    	if (attr == DW_AT_producer)
    		NoteProducer(aPair, info_ptr, form);

    	if (attr > DW_AT_recursive)
    		info_ptr = DefaultInfoEditFn(*this, info_ptr, form, aPair);
    	else
    		info_ptr = iInfoEditFn[attr](*this, info_ptr, form, aPair);
    }
    
	return info_ptr;
}


size_t DwarfInfoManager::SizeOfDieValue(Dwarf_Half aForm, Dwarf_Byte_Ptr aPtr){
    Dwarf_Unsigned length = 0;
    size_t leb128_length = 0;
    size_t ret_value = 0;

    switch (aForm) {

    default:			/* Handles form = 0. */
    	return (aForm);

    case DW_FORM_addr:
    	return iAddressSize;

    case DW_FORM_ref_addr:
    	// TODO: sort this out
    	return 4;  // this is a 4 byte relocatable in 32 bit dwarf and 8-byte in 64 bit dwarf

    case DW_FORM_block1:
    	return (*aPtr) + 1;

    case DW_FORM_block2:
    	ret_value = READ_UNALIGNED2(aPtr) + 2;
		return ret_value;

    case DW_FORM_block4:
    	ret_value = READ_UNALIGNED4(aPtr) + 4;
    	return ret_value;

    case DW_FORM_data1:
	return 1;

    case DW_FORM_data2:
	return 2;

    case DW_FORM_data4:
	return 4;

    case DW_FORM_data8:
	return 8;

    case DW_FORM_string:
	return (strlen((char *) aPtr) + 1);

    case DW_FORM_block:
    	length = DecodeUnsignedLeb128(aPtr, leb128_length);
    	return length + leb128_length;

    case DW_FORM_flag:
	return 1;

    case DW_FORM_ref_udata:
    	DecodeUnsignedLeb128(aPtr, leb128_length);
	return leb128_length;

    case DW_FORM_indirect: {
	    size_t indir_len = 0;
	    Dwarf_Unsigned form_indirect = DecodeUnsignedLeb128(aPtr, indir_len);
	    if (form_indirect == DW_FORM_indirect) {
			/* 	Eek, should never happen */
	    	cerr << "Error: DW_FORM_indirect gone recursive! Can't happen\n";
	    }
	    return indir_len + SizeOfDieValue(form_indirect, aPtr+indir_len);
	}

    case DW_FORM_ref1:
    	return 1;

    case DW_FORM_ref2:
    	return 2;

    case DW_FORM_ref4:
    	return 4;

    case DW_FORM_ref8:
    	return 8;

    case DW_FORM_sdata:
    	DecodeSignedLeb128(aPtr, leb128_length);
    	return leb128_length;

    case DW_FORM_strp:
    	// TODO: sort this out
    	return 4;  // this is a 4 byte relocatable in 32 bit dwarf and 8-byte in 64 bit dwarf

    case DW_FORM_udata:
    	DecodeUnsignedLeb128(aPtr, leb128_length);
    	return leb128_length;
    }
}

Dwarf_Byte_Ptr DwarfInfoManager::DefaultInfoEditFn(DwarfInfoManager& aManager, Dwarf_Byte_Ptr aPtr, Dwarf_Half aForm, FileShdrPair & aPair){
	return aPtr + aManager.SizeOfDieValue(aForm, aPtr);
}

Dwarf_Byte_Ptr DwarfInfoManager::ErrorInfoEditFn(DwarfInfoManager& aManager, Dwarf_Byte_Ptr aPtr, Dwarf_Half aForm, FileShdrPair & aPair){
	cerr << "Error: Undefined DW_FORM value: " << aForm << "\n" ;
	exit(EXIT_FAILURE);
	return aPtr;
}

// TODO: implicitly only deals with 32-bit DWARF
// Called from other edit functions to deal with blocks that contain location expressions.
Dwarf_Byte_Ptr DwarfInfoManager::InfoEditLocExpr(DwarfInfoManager& aManager, Dwarf_Byte_Ptr aPtr, Dwarf_Half aForm, FileShdrPair & aPair){
	Dwarf_Unsigned length = 0;
	bool locExpr = false;
	Dwarf_Byte_Ptr block = aPtr;
	if (aForm == DW_FORM_block1) {
		locExpr = true;
		length = block[0];
		block++;
		aPtr += (length + 1); 
	} else if (aForm == DW_FORM_block2) {
		locExpr = true;
		length = READ_UNALIGNED2(block);
		block += 2;
		aPtr += (length + 2); 
	} else if (aForm == DW_FORM_block4) {
		locExpr = true;
		length = READ_UNALIGNED4(block);
		block += 4;
		aPtr += (length + 4); 
	} else if (aForm == DW_FORM_block) {
		locExpr = true;
		size_t leb_length = 0;
		length = DecodeUnsignedLeb128(block, leb_length);
		block += leb_length;
		aPtr += (length + leb_length); 
	}

	if (locExpr){		
		EditLocationExpression (block, aManager.iAddressSize, length, aPair);
		return aPtr;
	} else if (aForm == DW_FORM_indirect){
	    size_t indir_len = 0;
	    Dwarf_Half form_indirect = DecodeUnsignedLeb128(aPtr, indir_len);
	    if (form_indirect == DW_FORM_indirect) {
			/* 	Eek, should never happen */
	    	cerr << "Error: DW_FORM_indirect gone recursive! Can't happen\n";
	    }
	    return InfoEditLocExpr(aManager, aPtr+indir_len, form_indirect, aPair);
	
	} else
		return aPtr + aManager.SizeOfDieValue(aForm, aPtr);
}
// TODO: implicitly only deals with 32-bit DWARF
Dwarf_Byte_Ptr DwarfInfoManager::InfoEditAddress(DwarfInfoManager& aManager, Dwarf_Byte_Ptr aPtr, Dwarf_Half aForm, FileShdrPair & aPair){
	if (aForm == DW_FORM_addr){
		LinearAddr addr = READ_UNALIGNED4(aPtr);
		LinearAddr relocatedAddr = aPair.iXIPFileDetails.Relocate(addr);
		WRITE_UNALIGNED4(aPtr, relocatedAddr);
		return aPtr + 4;
	} else if (aForm == DW_FORM_indirect){
	    size_t indir_len = 0;
	    Dwarf_Unsigned form_indirect = DecodeUnsignedLeb128(aPtr, indir_len);
	    if (form_indirect == DW_FORM_indirect) {
			/* 	Eek, should never happen */
	    	cerr << "Error: DW_FORM_indirect gone recursive! Can't happen\n";
	    }
	    return InfoEditAddress(aManager, aPtr+indir_len, form_indirect, aPair);
	
	} else
		return aPtr + aManager.SizeOfDieValue(aForm, aPtr);
}

// TODO: implicitly only deals with 32-bit DWARF
Dwarf_Byte_Ptr DwarfInfoManager::InfoEditLinePtr(DwarfInfoManager& aManager, Dwarf_Byte_Ptr aPtr, Dwarf_Half aForm, FileShdrPair & aPair){
	if (aForm == DW_FORM_data4){
		size_t offset = READ_UNALIGNED4(aPtr);
		size_t newOffset = aManager.iDwarfManager.GetLineSectionOffset(aPair.iXIPFileDetails.iElfFile) + offset;
		if (offset != newOffset)
			WRITE_UNALIGNED4(aPtr, newOffset);
		return aPtr + 4;
	} else if (aForm == DW_FORM_indirect){
	    size_t indir_len = 0;
	    Dwarf_Half form_indirect = DecodeUnsignedLeb128(aPtr, indir_len);
	    if (form_indirect == DW_FORM_indirect) {
			/* 	Eek, should never happen */
	    	cerr << "Error: DW_FORM_indirect gone recursive! Can't happen\n";
	    }
	    return InfoEditLinePtr(aManager, aPtr+indir_len, form_indirect, aPair);
	
	} else
		return aPtr + aManager.SizeOfDieValue(aForm, aPtr);
}

// TODO: implicitly only deals with 32-bit DWARF
Dwarf_Byte_Ptr DwarfInfoManager::InfoEditLocListPtr(DwarfInfoManager& aManager, Dwarf_Byte_Ptr aPtr, Dwarf_Half aForm, FileShdrPair & aPair){
	if (aForm == DW_FORM_data4){
		size_t offset = READ_UNALIGNED4(aPtr);
		size_t newOffset = aManager.iDwarfManager.GetLocListSectionOffset(aPair.iXIPFileDetails.iElfFile) + offset;
		if (offset != newOffset)
			WRITE_UNALIGNED4(aPtr, newOffset);
		return aPtr + 4;
	} else if (aForm == DW_FORM_indirect){
	    size_t indir_len = 0;
	    Dwarf_Half form_indirect = DecodeUnsignedLeb128(aPtr, indir_len);
	    if (form_indirect == DW_FORM_indirect) {
			/* 	Eek, should never happen */
	    	cerr << "Error: DW_FORM_indirect gone recursive! Can't happen\n";
	    }
	    return InfoEditLocListPtr(aManager, aPtr+indir_len, form_indirect, aPair);
	
	} else
		//return aPtr + aManager.SizeOfDieValue(aForm, aPtr);
		return InfoEditLocExpr(aManager, aPtr, aForm, aPair);
}





// TODO: implicitly only deals with 32-bit DWARF
Dwarf_Byte_Ptr DwarfInfoManager::InfoEditMacInfoPtr(DwarfInfoManager& aManager, Dwarf_Byte_Ptr aPtr, Dwarf_Half aForm, FileShdrPair & aPair){
	if (aForm == DW_FORM_data4){
		size_t offset = READ_UNALIGNED4(aPtr);
		size_t newOffset = aManager.iDwarfManager.GetMacInfoSectionOffset(aPair.iXIPFileDetails.iElfFile) + offset;
		if (offset != newOffset)
			WRITE_UNALIGNED4(aPtr, newOffset);
		return aPtr + 4;
	} else if (aForm == DW_FORM_indirect){
	    size_t indir_len = 0;
	    Dwarf_Half form_indirect = DecodeUnsignedLeb128(aPtr, indir_len);
	    if (form_indirect == DW_FORM_indirect) {
			/* 	Eek, should never happen */
	    	cerr << "Error: DW_FORM_indirect gone recursive! Can't happen\n";
	    }
	    return InfoEditMacInfoPtr(aManager, aPtr+indir_len, form_indirect, aPair);
	
	} else
		return aPtr + aManager.SizeOfDieValue(aForm, aPtr);
}

// TODO: implicitly only deals with 32-bit DWARF
Dwarf_Byte_Ptr DwarfInfoManager::InfoEditRangeListPtr(DwarfInfoManager& aManager, Dwarf_Byte_Ptr aPtr, Dwarf_Half aForm, FileShdrPair & aPair){
	if (aForm == DW_FORM_data4){
		size_t offset = READ_UNALIGNED4(aPtr);
		size_t newOffset = aManager.iDwarfManager.GetRangesSectionOffset(aPair.iXIPFileDetails.iElfFile) + offset;
		if (offset != newOffset)
			WRITE_UNALIGNED4(aPtr, newOffset);
		return aPtr + 4;
	} else if (aForm == DW_FORM_indirect){
	    size_t indir_len = 0;
	    Dwarf_Half form_indirect = DecodeUnsignedLeb128(aPtr, indir_len);
	    if (form_indirect == DW_FORM_indirect) {
			/* 	Eek, should never happen */
	    	cerr << "Error: DW_FORM_indirect gone recursive! Can't happen\n";
	    }
	    return InfoEditRangeListPtr(aManager, aPtr+indir_len, form_indirect, aPair);
	
	} else
		return aPtr + aManager.SizeOfDieValue(aForm, aPtr);
}

// TODO: implicitly only deals with 32-bit DWARF
Dwarf_Byte_Ptr DwarfInfoManager::InfoEditString(DwarfInfoManager& aManager, Dwarf_Byte_Ptr aPtr, Dwarf_Half aForm, FileShdrPair & aPair){
	
	if (aForm == DW_FORM_strp){
		size_t offset = READ_UNALIGNED4(aPtr);
		size_t newOffset = aManager.iDwarfManager.GetStrSectionOffset(aPair.iXIPFileDetails.iElfFile) + offset;
		if (offset != newOffset)
			WRITE_UNALIGNED4(aPtr, newOffset);
		return aPtr + 4;
	} else if (aForm == DW_FORM_indirect){
	    size_t indir_len = 0;
	    Dwarf_Half form_indirect = DecodeUnsignedLeb128(aPtr, indir_len);
	    if (form_indirect == DW_FORM_indirect) {
			/* 	Eek, should never happen */
	    	cerr << "Error: DW_FORM_indirect gone recursive! Can't happen\n";
	    }
	    return InfoEditString(aManager, aPtr+indir_len, form_indirect, aPair);
	
	} else
		return aPtr + aManager.SizeOfDieValue(aForm, aPtr);
}

// TODO: implicitly only deals with 32-bit DWARF
Dwarf_Byte_Ptr DwarfInfoManager::InfoEditReference(DwarfInfoManager& aManager, Dwarf_Byte_Ptr aPtr, Dwarf_Half aForm, FileShdrPair & aPair){
	if (aForm == DW_FORM_ref_addr){
		size_t offset = READ_UNALIGNED4(aPtr);
		size_t newOffset = aManager.CheckNewOffset(aManager.GetSectionOffset(aPair.iXIPFileDetails.iElfFile), offset);
		if (offset != newOffset)
			WRITE_UNALIGNED4(aPtr, newOffset);
		return aPtr + 4;
	} else if (aForm == DW_FORM_indirect){
	    size_t indir_len = 0;
	    Dwarf_Half form_indirect = DecodeUnsignedLeb128(aPtr, indir_len);
	    if (form_indirect == DW_FORM_indirect) {
			/* 	Eek, should never happen */
	    	cerr << "Error: DW_FORM_indirect gone recursive! Can't happen\n";
	    }
	    return InfoEditReference(aManager, aPtr+indir_len, form_indirect, aPair);
	
	} else
		//return aPtr + aManager.SizeOfDieValue(aForm, aPtr);
		return InfoEditLocExpr(aManager, aPtr, aForm, aPair);
}

// TODO: implicitly only deals with 32-bit DWARF
// Explicitly check for *_address and *_strp then let the reference handler deal with the flag possiblity as s 'else'. 
Dwarf_Byte_Ptr DwarfInfoManager::InfoEditTrampoline(DwarfInfoManager& aManager, Dwarf_Byte_Ptr aPtr, Dwarf_Half aForm, FileShdrPair & aPair){
	if (aForm == DW_FORM_addr)
		return InfoEditAddress(aManager, aPtr, aForm, aPair);
	else if (aForm = DW_FORM_strp)
		return InfoEditString(aManager, aPtr, aForm, aPair);
	else if (aForm == DW_FORM_indirect){
		    size_t indir_len = 0;
		    Dwarf_Half form_indirect = DecodeUnsignedLeb128(aPtr, indir_len);
		    if (form_indirect == DW_FORM_indirect) {
				/* 	Eek, should never happen */
		    	cerr << "Error: DW_FORM_indirect gone recursive! Can't happen\n";
		    }
		    return InfoEditTrampoline(aManager, aPtr+indir_len, form_indirect, aPair);
		
		} 
	else
		return InfoEditReference(aManager, aPtr, aForm, aPair);
}


DwarfInfoManager::InfoEditFn DwarfInfoManager::iInfoEditFn [] = {
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  0 so this should never be called
	DwarfInfoManager::InfoEditReference, 	// DW_AT_sibling                           0x01
	DwarfInfoManager::InfoEditLocListPtr, 	// DW_AT_location                          0x02
	DwarfInfoManager::InfoEditString, 		// DW_AT_name                              0x03
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  4 so this should never be called
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  5 so this should never be called
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  6 so this should never be called
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  7 so this should never be called
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  8 so this should never be called
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_ordering                          0x09
    // TODO: This doesn't appear in the DWARF 3 spec of 2005/12/05 :-( so just defualt it for now
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_subscr_data                       0x0a
    DwarfInfoManager::InfoEditReference, 	// DW_AT_byte_size                         0x0b
    DwarfInfoManager::InfoEditReference, 	// DW_AT_bit_offset                        0x0c
    DwarfInfoManager::InfoEditReference, 	// DW_AT_bit_size                          0x0d
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  0x0e so this should never be called
    // TODO: This doesn't appear in the DWARF 3 spec of 2005/12/05 :-( so just defualt it for now
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_element_list                      0x0f
    DwarfInfoManager::InfoEditLinePtr, 		// DW_AT_stmt_list                         0x10
    DwarfInfoManager::InfoEditAddress, 		// DW_AT_low_pc                            0x11
    DwarfInfoManager::InfoEditAddress, 		// DW_AT_high_pc                           0x12
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_language                          0x13
    // TODO: This doesn't appear in the DWARF 3 spec of 2005/12/05 :-( so just defualt it for now
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_member                            0x14
    DwarfInfoManager::InfoEditReference, 	// DW_AT_discr                             0x15
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_discr_value                       0x16
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_visibility                        0x17
    DwarfInfoManager::InfoEditReference, 	// DW_AT_import                            0x18
    DwarfInfoManager::InfoEditLocListPtr, 	// DW_AT_string_length                     0x19
    DwarfInfoManager::InfoEditReference, 	// DW_AT_common_reference                  0x1a
    DwarfInfoManager::InfoEditString, 		// DW_AT_comp_dir                          0x1b
    DwarfInfoManager::InfoEditString, 		// DW_AT_const_value                       0x1c
    DwarfInfoManager::InfoEditReference, 	// DW_AT_containing_type                   0x1d
    DwarfInfoManager::InfoEditReference, 	// DW_AT_default_value                     0x1e
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  0x1f so this should never be called
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_inline                            0x20
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_is_optional                       0x21
    DwarfInfoManager::InfoEditReference, 	// DW_AT_lower_bound                       0x22
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  0x23 so this should never be called
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  0x24 so this should never be called
    DwarfInfoManager::InfoEditString, 		// DW_AT_producer                          0x25
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  0x26 so this should never be called
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_prototyped                        0x27
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  0x28 so this should never be called
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  0x29 so this should never be called
    DwarfInfoManager::InfoEditLocListPtr, 	// DW_AT_return_addr                       0x2a
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  0x2b so this should never be called
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_start_scope                       0x2c
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  0x2d so this should never be called
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_bit_stride                        0x2e /* DWARF3 name */
    DwarfInfoManager::InfoEditReference, 	// DW_AT_upper_bound                       0x2f
	DwarfInfoManager::ErrorInfoEditFn, 		// There is no DW_FORM  0x30 so this should never be called
    DwarfInfoManager::InfoEditReference, 	// DW_AT_abstract_origin                   0x31
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_accessibility                     0x32
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_address_class                     0x33
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_artificial                        0x34
    DwarfInfoManager::InfoEditReference, 	// DW_AT_base_types                        0x35
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_calling_convention                0x36
    DwarfInfoManager::InfoEditReference, 	// DW_AT_count                             0x37
    DwarfInfoManager::InfoEditLocListPtr, 	// DW_AT_data_member_location              0x38
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_decl_column                       0x39
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_decl_file                         0x3a
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_decl_line                         0x3b
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_declaration                       0x3c
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_discr_list                        0x3d
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_encoding                          0x3e
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_external                          0x3f
    DwarfInfoManager::InfoEditLocListPtr, 	// DW_AT_frame_base                        0x40
    DwarfInfoManager::InfoEditReference, 	// DW_AT_friend                            0x41
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_identifier_case                   0x42
    DwarfInfoManager::InfoEditMacInfoPtr, 	// DW_AT_macro_info                        0x43
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_namelist_item                     0x44
    DwarfInfoManager::InfoEditReference, 	// DW_AT_priority                          0x45
    DwarfInfoManager::InfoEditLocListPtr, 	// DW_AT_segment                           0x46
    DwarfInfoManager::InfoEditReference, 	// DW_AT_specification                     0x47
    DwarfInfoManager::InfoEditLocListPtr, 	// DW_AT_static_link                       0x48
    DwarfInfoManager::InfoEditReference, 	// DW_AT_type                              0x49
    DwarfInfoManager::InfoEditLocListPtr, 	// DW_AT_use_location                      0x4a
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_variable_parameter                0x4b
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_virtuality                        0x4c
    DwarfInfoManager::InfoEditLocListPtr, 	// DW_AT_vtable_elem_location              0x4d
    DwarfInfoManager::InfoEditReference, 	// DW_AT_allocated                         0x4e /* DWARF3 */
    DwarfInfoManager::InfoEditReference, 	// DW_AT_associated                        0x4f /* DWARF3 */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_data_location                     0x50 /* DWARF3 */
    DwarfInfoManager::InfoEditReference, 	// DW_AT_byte_stride                       0x51 /* DWARF3f */
    DwarfInfoManager::InfoEditAddress, 		// DW_AT_entry_pc                          0x52 /* DWARF3 */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_use_UTF8                          0x53 /* DWARF3 */
    DwarfInfoManager::InfoEditReference, 	// DW_AT_extension                         0x54 /* DWARF3 */
    DwarfInfoManager::InfoEditRangeListPtr, // DW_AT_ranges                            0x55 /* DWARF3 */
    DwarfInfoManager::InfoEditTrampoline, 	// DW_AT_trampoline                        0x56 /* DWARF3 */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_call_column                       0x57 /* DWARF3 */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_call_file                         0x58 /* DWARF3 */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_call_line                         0x59 /* DWARF3 */
    DwarfInfoManager::InfoEditString, 		// DW_AT_description                       0x5a /* DWARF3 */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_binary_scale                      0x5b /* DWARF3f */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_decimal_scale                     0x5c /* DWARF3f */
    DwarfInfoManager::InfoEditReference, 	// DW_AT_small                             0x5d /* DWARF3f */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_decimal_sign                      0x5e /* DWARF3f */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_digit_count                       0x5f /* DWARF3f */
    DwarfInfoManager::InfoEditString, 		// DW_AT_picture_string                    0x60 /* DWARF3f */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_mutable                           0x61 /* DWARF3f */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_threads_scaled                    0x62 /* DWARF3f */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_explicit                          0x63 /* DWARF3f */
    DwarfInfoManager::InfoEditReference, 	// DW_AT_object_pointer                    0x64 /* DWARF3f */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_endianity                         0x65 /* DWARF3f */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_elemental                         0x66 /* DWARF3f */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_pure                              0x67 /* DWARF3f */
    DwarfInfoManager::DefaultInfoEditFn, 	// DW_AT_recursive                         0x68 /* DWARF3f */
};
