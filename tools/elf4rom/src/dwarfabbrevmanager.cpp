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

const string DwarfAbbrevManager::iAbbrevSectionName(".debug_abbrev");
void InfoContext::SetAbbrevOffset(size_t offset){
	if (!iContextValid){
		cerr << "Error: runtime error - SetAbbrevOffset called on invalid InfoContext\n";
		exit(EXIT_FAILURE);
	}
	
	if (iAbbrevOffset == offset)
			return;
	// Save current entry (in particular the cursor value
	if (iAbbrevOffset != 0xffffffff){
		AbbrevOffsetMap::iterator old = iMap.find(iAbbrevOffset);
		if (old != iMap.end())
			old->second.iCursor = iAbbrevMapEntry.iCursor;
		else
			iMap[iAbbrevOffset] = iAbbrevMapEntry;
	}
	
	AbbrevOffsetMap::iterator i = iMap.find(offset);
	if (i != iMap.end()){
		iAbbrevMapEntry = i->second;
	} else {
		AbbrevMap * newMap = new AbbrevMap;
		new (&iAbbrevMapEntry) AbbrevMapEntry(iSectionStart + offset, newMap);
		iMap[offset] = iAbbrevMapEntry;
	}
	iAbbrevOffset = offset;
}

DebugAbbrev & InfoContext::GetAbbrev(Dwarf_Word aCode){
	AbbrevMap::iterator i = iAbbrevMapEntry.iMap->find(aCode);
	if (i != iAbbrevMapEntry.iMap->end()){
			return i->second;
	} else {
		return FindAbbrev(aCode);
	}
}

DebugAbbrev & InfoContext::FindAbbrev(Dwarf_Word aCode){
	// retrieve cursor for where we've scanned so far.
	Dwarf_Byte_Ptr p = iAbbrevMapEntry.iCursor;
	// NB. error if we don't find the code before section end
	Dwarf_Byte_Ptr lim = iSectionEnd;
	bool error = false;
	bool found = false;
	size_t leb128_length;

	while (!found && (p < lim)){
		
#define CHECK_ABBREV_LIM(p,l,e) { if ((p)>(l)){ e = true; break;} }
#define CHECK_DECODE_ULEB128(v,p,n,l,e) \
			DECODE_ULEB128(v,p,n)\
			CHECK_ABBREV_LIM(p,l,e);
		
		size_t count = 0;
		//CHECK_DECODE_ULEB128(abbrev_code,p,leb128_length,lim, error);
		Dwarf_Word abbrev_code = DwarfSectionManager::DecodeUnsignedLeb128(p, leb128_length);
		p += leb128_length;
		if (p>lim){ 
			error = true; 
			break;
		}
		// Might as well error here since either we've found the NULL abbrev
	    if (abbrev_code == 0) {
	    	DebugAbbrev abbr(0, 0, 0, NULL, NULL);
	    	(*iAbbrevMapEntry.iMap)[0] = abbr;
	    	if (aCode == 0) // ??? why would it
	    		found = true;
	    }

		CHECK_DECODE_ULEB128(tag,p,leb128_length,lim, error);
		// don't care about 'has child'
	    p++;
	    CHECK_ABBREV_LIM(p,lim,error);
	    Dwarf_Byte_Ptr raw_attr_ptr = p;
	    Dwarf_Word attr;
	    Dwarf_Word attr_form;
	    do {
	    	CHECK_DECODE_ULEB128(a,p,leb128_length,lim, error);
	    	attr = a;
	    	CHECK_DECODE_ULEB128(f,p,leb128_length,lim, error);
	    	attr_form = f;

	    	if (attr != 0)
	    		count++;

	    } while (attr != 0 || attr_form != 0);
	    
		DebugAbbrevAttrForm * list = new DebugAbbrevAttrForm[count];
		Dwarf_Byte_Ptr q=raw_attr_ptr;
		for (size_t i=0 ; i < count ; i++){
			list[i].iAttr = ULEB128(q,leb128_length);
			list[i].iForm = ULEB128(q,leb128_length);
		}
	    DebugAbbrev abbr(abbrev_code, tag, count, raw_attr_ptr, list);
	    (*iAbbrevMapEntry.iMap)[abbrev_code] = abbr;
	    if (abbrev_code == aCode)
	    	found = true;
	}
	if (error){
		cerr << "Error: corrupt .debug_abbrev section\n";
		exit(EXIT_FAILURE);
	}
	if (!found){
		cerr << "Error: abbrev code not found in .debug_abbrev section\n";
		exit(EXIT_FAILURE);
	}
	// record where we scanned to
	iAbbrevMapEntry.iCursor = p;
	// get the 
	AbbrevMap::iterator i = iAbbrevMapEntry.iMap->find(aCode);
	if (i == iAbbrevMapEntry.iMap->end()){
		cerr << "Error: Runtime error processing .debug_abbrev section\n";
		exit(EXIT_FAILURE);
	} 

	return i->second;
}

void DwarfAbbrevManager::StartContext(PathName & aName){
	Dwarf_Byte_Ptr section = GetSection(aName);
	iInfoContext.Init(section, section + GetSectionSize(aName), GetSectionOffset(aName));
}

void DwarfAbbrevManager::EndContext(){
	iInfoContext.Reset();
}

void DwarfAbbrevManager::SetContextAbbrevOffset(Uint32 offset){
	iInfoContext.SetAbbrevOffset(offset);
}

size_t DwarfAbbrevManager::GetContextSectionOffset(){
	return iInfoContext.GetSectionOffset();
}

DebugAbbrev & DwarfAbbrevManager::GetAbbrev(Dwarf_Word aCode){
	return iInfoContext.GetAbbrev(aCode);
}
