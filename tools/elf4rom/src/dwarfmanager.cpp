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
#include <iomanip>

#include "dwarfmanager.h"
#include "inputfile.h"
#include "outputfile.h"
#include "filefragment.h"

void DwarfSectionManager::AddSection(XIPFileDetails & aXIPFileDetails, Elf32_Shdr * aShdr){
	if (iRomDetails->iTrace){
		cout << "    " << GetSectionName() << " - DWARF\n";
	}

	FileShdrPair aFileShdrPair(aXIPFileDetails, aShdr);
	iFileShdrList.push_back(aFileShdrPair);
}

void DwarfSectionManager::SetupSection(){
	if (!iFileShdrList.empty()){
		ElfSectionElfData * aDwarfSectionData = new ElfSectionElfData(*this);
		Elf32_Shdr aDwarfShdr;
		aDwarfShdr.sh_name = 0; // for now.
		aDwarfShdr.sh_type = SHT_PROGBITS;
		aDwarfShdr.sh_flags = 0;
		aDwarfShdr.sh_addr = 0;
		aDwarfShdr.sh_offset = 0; // for now
		aDwarfShdr.sh_size = 0; // for now.
		aDwarfShdr.sh_link = 0;
		aDwarfShdr.sh_info = 0;
		aDwarfShdr.sh_addralign = 4;
		aDwarfShdr.sh_entsize = sizeof(Elf32_Sym);
		
		ElfSection aDwarfSection(aDwarfSectionData, GetSectionName().c_str(), aDwarfShdr);
		iElfSectionManager.AddSection(aDwarfSection);
	}
}

Dwarf_Unsigned DwarfSectionManager::DecodeUnsignedLeb128(Dwarf_Byte_Ptr leb128, size_t & leb128_length){
	Dwarf_Ubyte byte;
    Dwarf_Word word_number;
    Dwarf_Unsigned number;
    Dwarf_Sword shift;
    Dwarf_Sword byte_length;

    /* The following unrolls-the-loop for the first few bytes and
       unpacks into 32 bits to make this as fast as possible.
       word_number is assumed big enough that the shift has a defined
       result. */
    if ((*leb128 & 0x80) == 0) {
    	leb128_length = 1;
    	return *leb128;
    } else if ((*(leb128 + 1) & 0x80) == 0) {
    	leb128_length = 2;

    	word_number = *leb128 & 0x7f;
    	word_number |= (*(leb128 + 1) & 0x7f) << 7;
    	return word_number;
    } else if ((*(leb128 + 2) & 0x80) == 0) {
    	leb128_length = 3;

    	word_number = *leb128 & 0x7f;
    	word_number |= (*(leb128 + 1) & 0x7f) << 7;
    	word_number |= (*(leb128 + 2) & 0x7f) << 14;
    	return word_number;
    } else if ((*(leb128 + 3) & 0x80) == 0) {
    	leb128_length = 4;

    	word_number = *leb128 & 0x7f;
    	word_number |= (*(leb128 + 1) & 0x7f) << 7;
    	word_number |= (*(leb128 + 2) & 0x7f) << 14;
    	word_number |= (*(leb128 + 3) & 0x7f) << 21;
    	return word_number;
    }

    /* The rest handles long numbers Because the 'number' may be larger 
       than the default int/unsigned, we must cast the 'byte' before
       the shift for the shift to have a defined result. */
    number = 0;
    shift = 0;
    byte_length = 1;
    byte = *(leb128);
    for (;;) {
    	number |= ((Dwarf_Unsigned) (byte & 0x7f)) << shift;

    	if ((byte & 0x80) == 0) {
    		leb128_length = byte_length;
    		return number;
    	}
    	shift += 7;

    	byte_length++;
    	++leb128;
    	byte = *leb128;
    }
}

#define BITSINBYTE 8
Dwarf_Signed DwarfSectionManager::DecodeSignedLeb128(Dwarf_Byte_Ptr leb128, size_t & leb128_length){
    Dwarf_Signed number = 0;
    Dwarf_Bool sign = 0;
    Dwarf_Word shift = 0;
    Dwarf_Ubyte byte = *leb128;
    Dwarf_Word byte_length = 1;

    /* byte_length being the number of bytes of data absorbed so far in 
       turning the leb into a Dwarf_Signed. */

    for (;;) {
    	sign = byte & 0x40;
    	number |= ((Dwarf_Signed) ((byte & 0x7f))) << shift;
    	shift += 7;

    	if ((byte & 0x80) == 0) {
    		break;
    	}
    	++leb128;
    	byte = *leb128;
    	byte_length++;
    }

    if ((shift < sizeof(Dwarf_Signed) * BITSINBYTE) && sign) {
    	number |= -((Dwarf_Signed) 1 << shift);
    }

    leb128_length = byte_length;
    return number;
}

Dwarf_Byte_Ptr DwarfSectionManager::GetSectionData(FileShdrPair & aPair){

	InputFile in(aPair.iXIPFileDetails.iElfFile);
	in.SetOffset(aPair.iShdr.sh_offset);
	return (Dwarf_Byte_Ptr)in.GetData(aPair.iShdr.sh_size);
}

void DwarfConcatenatedSectionManager::GetFileFragmentData(FileFragmentData & aFileFragmentData ){
	ConcatenateData();
	if (iRomDetails->iTrace){
		cout << "\nGenerating DWARF section " << GetSectionName() << " size = "
		<< dec << Size() << " bytes\n";
	} 
	FileShdrList::iterator e = iFileShdrList.end();
	for (FileShdrList::iterator i = iFileShdrList.begin(); i < e; i++){
		if (iRomDetails->iTrace){
			cout << "    " << i->iXIPFileDetails.iElfFile << " " << dec << i->iShdr.sh_size << " bytes\n" << flush;
		}
		ProcessSection(*i);
	}	
	SetFileFragmentData(aFileFragmentData, iSize, reinterpret_cast<char *>(iData));
}

void DwarfConcatenatedSectionManager::ConcatenateData(){
	if (iData == NULL) {
		size_t sectionSize = Size();
		iData = new Dwarf_Ubyte[sectionSize];
		Dwarf_Byte_Ptr p = iData;
		FileShdrList::iterator e = iFileShdrList.end();
		for (FileShdrList::iterator i = iFileShdrList.begin(); i < e; i++){
			size_t off = p - iData;
			size_t soff = GetSectionOffset(i->iXIPFileDetails.iElfFile);
			if (off >= sectionSize)
				assert(off < sectionSize);
			if (off != soff)
				assert(off == soff);
			size_t n = i->iShdr.sh_size;
			Dwarf_Byte_Ptr contrib = GetSectionData(*i);
			memcpy(p, contrib, n);
			p += n;
			delete [] contrib;
		}
	}
}

size_t DwarfConcatenatedSectionManager::Size(){
	if (iSizeValid)
		return iSize;
	//cerr << "Size for " << GetSectionName() << "\n";
	size_t offset = 0;
	FileShdrList::iterator e = iFileShdrList.end();
	for (FileShdrList::iterator i = iFileShdrList.begin(); i != e; i++){
		//cerr << "offset = 0x" << offset << "\t";
		SetSectionOffset(i->iXIPFileDetails.iElfFile, offset);
		size_t size = i->iShdr.sh_size;
		//cerr << "section size for " << i->iXIPFileDetails.iElfFile << " 0x" << size << "\n";
		SetSectionSize(i->iXIPFileDetails.iElfFile, size);
		size_t newOffset = offset + size;
		if (newOffset < offset){
			cerr << "Error: The combined section " << GetSectionName() 
				<< " requires translation from 32 to 64 bit Dwarf which is not currently supported.\n"
				<< "Exclude the following files (or their equivalent in terms of their contribution to Dwarf):\n";
			for (; i != e; i++){
				cerr << i->iXIPFileDetails.iE32File << "\n";
			}
			exit(EXIT_FAILURE);
		}
		offset = newOffset;
	}
	//cerr << "Size = 0x" << offset << "\n";
	iSizeValid = true;
	return iSize = offset;
}

void DwarfConcatenatedSectionManager::ProcessSection(FileShdrPair & aPair){
	Dwarf_Byte_Ptr start = GetSection(aPair.iXIPFileDetails.iElfFile);
	Dwarf_Byte_Ptr end = start + aPair.iShdr.sh_size;
	ProcessSection(aPair, start, end);
}

void DwarfConcatenatedSectionManager::SetSectionOffset(PathName & aPathName, size_t aOffset) {
	iPathNameSectionOffsetMap[aPathName] = aOffset;
}

void DwarfConcatenatedSectionManager::InitOffsetMap(){ 
	Size();  // forces the map to be set up if it hasn't been already
}

size_t DwarfConcatenatedSectionManager::GetSectionOffset(PathName & aPathName){
	if (!iSizeValid) // if the size is valid then so must be the offsets.
		InitOffsetMap(); 
	return iPathNameSectionOffsetMap[aPathName];
}

void DwarfConcatenatedSectionManager::SetSectionSize(PathName & aPathName, size_t aSize) {
		iPathNameSectionSizeMap[aPathName] = aSize;
}

size_t DwarfConcatenatedSectionManager::GetSectionSize(PathName & aPathName){
	if (!iSizeValid) // if the size is valid then so must be the offsets.
		InitOffsetMap(); 
	return iPathNameSectionSizeMap[aPathName];
}

Dwarf_Byte_Ptr DwarfConcatenatedSectionManager::GetSection(PathName & aPathName){
	ConcatenateData();
	size_t offset = GetSectionOffset(aPathName);
	return iData + offset;
}

class ElfSectionFragmentedDwarfData : public ElfSectionElfData {
public:
	ElfSectionFragmentedDwarfData(FileFragmentOwner & aSource) :
		ElfSectionElfData(aSource)
	{}
	
	ElfSectionFragmentedDwarfData(const ElfSectionElfData & aData) :
		ElfSectionElfData(aData)
	{}	
	
	// ElfSection protocol
	virtual ElfSectionFragmentedDwarfData * Clone(){
		return new ElfSectionFragmentedDwarfData(*this);
	}
	
	virtual void AddData(OutputFile & aOutputFile){
		return;
	}
};

class DwarfSectionFragment : public FileFragmentOwner {
public:
	DwarfSectionFragment(DwarfFragmentedSectionManager & aSource,
						 FileShdrPair & aPair):
		iSource(aSource),
		iPair(aPair),
		iData(NULL)
		{}
	
	// Bitwise copy is OK so don't need to write our own copy ctor etc.
	
	// The FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData );
	virtual size_t Size();
	virtual void DeleteFileFragmentData();
	
private:
	DwarfSectionFragment();
	
private:
	DwarfFragmentedSectionManager & iSource;
	FileShdrPair & iPair;
	Dwarf_Byte_Ptr iData;
};

void DwarfSectionFragment::GetFileFragmentData(FileFragmentData & aFileFragmentData ){
	iSource.ProcessSection(iPair, iData);	
	SetFileFragmentData(aFileFragmentData, Size(), reinterpret_cast<char *>(iData));	
}

size_t DwarfSectionFragment::Size(){
	return iPair.iShdr.sh_size;
}

void DwarfSectionFragment::DeleteFileFragmentData(){
	delete [] iData;
	// 
	delete this;
}

void DwarfFragmentedSectionManager::SetupSection(){
	if (!iFileShdrList.empty()){
		ElfSectionFragmentedDwarfData * aDwarfSectionData = new ElfSectionFragmentedDwarfData(*this);
		Elf32_Shdr aDwarfShdr;
		aDwarfShdr.sh_name = 0; // for now.
		aDwarfShdr.sh_type = SHT_PROGBITS;
		aDwarfShdr.sh_flags = 0;
		aDwarfShdr.sh_addr = 0;
		aDwarfShdr.sh_offset = 0; // for now
		aDwarfShdr.sh_size = 0; // for now.
		aDwarfShdr.sh_link = 0;
		aDwarfShdr.sh_info = 0;
		aDwarfShdr.sh_addralign = 4;
		aDwarfShdr.sh_entsize = sizeof(Elf32_Sym);
		
		// aDwarfSectionData will ask the secton manager (i.e. its source) for the offset of the section.
		// So we better record it here. We assume that it is the current size of the output file.
		// As long as we are single threaded and all the fragments get added consecutively as below
		// this is a safe assumption.
		SetOffset(iDwarfManager.GetOutputFile().Size());
		
		ElfSection aDwarfSection(aDwarfSectionData, GetSectionName().c_str(), aDwarfShdr);
		iElfSectionManager.AddSection(aDwarfSection);
		
		for (FileShdrList::iterator i = iFileShdrList.begin(); i < iFileShdrList.end(); i++ ){
			DwarfSectionFragment * aFrag = new DwarfSectionFragment(*this, *i);
			aFrag->AddData(iDwarfManager.GetOutputFile());
		}
	}
}

// NB the section itself doesn't write any data
// The FileFragmentOwner protocol

void DwarfFragmentedSectionManager::GetFileFragmentData(FileFragmentData & aFileFragmentData ){	
	SetFileFragmentData(aFileFragmentData, 0, reinterpret_cast<char *>(NULL));
}

// This should never get called
void DwarfFragmentedSectionManager::ConcatenateData(){
	assert(1 == 0);
	return;
}

void DwarfFragmentedSectionManager::ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr & aData){
	if (iInitialTraceMessage & iRomDetails->iTrace){
		cout << "\nGenerating DWARF section " << GetSectionName() << " size = "
			<< dec << Size() << " bytes\n";
		iInitialTraceMessage = false;
	}
	if (iRomDetails->iTrace){
		cout << "    " << aPair.iXIPFileDetails.iElfFile << " size = " 
		<< dec << aPair.iShdr.sh_size << "\n" << flush; 
	}
	Dwarf_Byte_Ptr start = GetSectionData(aPair);
	aData = start;
	Dwarf_Byte_Ptr end = start + aPair.iShdr.sh_size;
	ProcessSection(aPair, start, end);
}

#if 0
// This should never get called
void DwarfFragmentedSectionManager::ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr start, Dwarf_Byte_Ptr end){
	assert(1 == 0);
	return;
}
#endif

const string DwarfMacinfoManager::iMacinfoSectionName(".debug_macinfo");
const string DwarfStrManager::iStrSectionName(".debug_str");


void DwarfManager::AddSection(XIPFileDetails & aXIPFileDetails, string aSectionName, Elf32_Shdr * aShdr){
	if (iDwarfAbbrevManager.GetSectionName() == aSectionName)
		iDwarfAbbrevManager.AddSection(aXIPFileDetails, aShdr);
	else if (iDwarfArangesManager.GetSectionName() == aSectionName)
		iDwarfArangesManager.AddSection(aXIPFileDetails, aShdr);
	else if (iDwarfFrameManager.GetSectionName() == aSectionName)
		iDwarfFrameManager.AddSection(aXIPFileDetails, aShdr);
	else if (iDwarfInfoManager.GetSectionName() == aSectionName)
		iDwarfInfoManager.AddSection(aXIPFileDetails, aShdr);
	else if (iDwarfLineManager.GetSectionName() == aSectionName)
		iDwarfLineManager.AddSection(aXIPFileDetails, aShdr);
	else if (iDwarfLocManager.GetSectionName() == aSectionName)
		iDwarfLocManager.AddSection(aXIPFileDetails, aShdr);
	else if (iDwarfMacinfoManager.GetSectionName() == aSectionName)
		iDwarfMacinfoManager.AddSection(aXIPFileDetails, aShdr);
	else if (iDwarfPubnamesManager.GetSectionName() == aSectionName)
		iDwarfPubnamesManager.AddSection(aXIPFileDetails, aShdr);
	else if (iDwarfPubtypesManager.GetSectionName() == aSectionName)
		iDwarfPubtypesManager.AddSection(aXIPFileDetails, aShdr);
	else if (iDwarfRangesManager.GetSectionName() == aSectionName)
		iDwarfRangesManager.AddSection(aXIPFileDetails, aShdr);
	else if (iDwarfStrManager.GetSectionName() == aSectionName)
		iDwarfStrManager.AddSection(aXIPFileDetails, aShdr);
#if 0
	else
		cerr << "Warning: unrecognised debug section name " << aSectionName << " ignored\n";
#endif
}

void DwarfManager::SetupSections(){
	// The order here is important for fix up
	// first the purely concatenated 'leaf' sections
	// See the diagram on p.182 of the Dwarf 3 spec
	// to understand the 'dependenices'
	iDwarfAbbrevManager.SetupSection();
	iDwarfFrameManager.SetupSection();
	iDwarfPubnamesManager.SetupSection();
	iDwarfPubtypesManager.SetupSection();
	iDwarfArangesManager.SetupSection();	
	iDwarfMacinfoManager.SetupSection();
	
	iDwarfInfoManager.SetupSection();
	iDwarfLineManager.SetupSection();
	iDwarfLocManager.SetupSection();

	iDwarfRangesManager.SetupSection();
	iDwarfStrManager.SetupSection();
}
