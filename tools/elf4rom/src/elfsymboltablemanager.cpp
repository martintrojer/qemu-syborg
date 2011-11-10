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

#include <libelf.h>
#include <map>

#include "elfsymboltablemanager.h"

void ElfFileSymbolFragments::AddSymbolTable(String & aPath, size_t aOffset, size_t aSize, size_t aFirstGlobal){
	iPath = aPath;
	// drop the inital 'undefined' symbol
	iSymbolTableOffset = aOffset + sizeof(Elf32_Sym);
	iSymbolTableSize = aSize - sizeof(Elf32_Sym);
	iFirstGlobal = aFirstGlobal - 1;
}

void ElfFileSymbolFragments::AddStringTable(String & aPath, size_t aOffset, size_t aSize){
	iPath = aPath;
	// drop the inital "\0"
	iStringTableOffset = aOffset + 1;
	iStringTableSize = aSize - 1;
}

size_t ElfFileSymbolFragments::LookupSection(size_t ndx){
	SectionNumberMap::iterator aMapping = iSectionNumberMap.begin();
	SectionNumberMap::iterator end = iSectionNumberMap.end();
	while (aMapping != end) {
		if (aMapping->iOld == ndx) 
			return aMapping->iNew;
		aMapping++;
	}
	return ndx;
}

int ElfFileSymbolFragments::LookupVaddrAddend(size_t ndx){
	SectionVaddrAddendMap::iterator aMapping = iSectionVaddrAddendMap.begin();
	SectionVaddrAddendMap::iterator end = iSectionVaddrAddendMap.end();
	while (aMapping != end) {
		if (aMapping->iSectionNumber == ndx) 
			return aMapping->iAddend;
		aMapping++;
	}
	return 0;
}

size_t ElfSymTabStringTable::Size(){
	return iElfSymbolTableManager.GetSymTabStringsSectionSize();
}

void ElfSymbolTableManager::Finalize(SectionNumberMap & aSectionNumberMap, SectionVaddrAddendMap & aSectionVaddrAddendMap ){
	iCurrentFragment.SetSectionNumberMap(aSectionNumberMap);
	iCurrentFragment.SetSectionVaddrAddendMap(aSectionVaddrAddendMap);
	iCurrentFragment.Validate();
	iSymbolFragments.push_back(iCurrentFragment);
	iCurrentFragment.Reset();
}

// TODO: This could be done more efficiently and with out the use of the ElfStringTable object.
void ElfSymbolTableManager::GetFileFragmentData(FileFragmentData & aFileFragmentData ){
	size_t symTabSize = GetSymTabSectionSize(); 
	iData = new char[symTabSize];
	Elf32_Sym * syms = (Elf32_Sym *)iData;

	
	// set up UNDEF symbol
	syms[0].st_info = 0;
	syms[0].st_name = 0;
	syms[0].st_other = 0;
	syms[0].st_shndx = 0;
	syms[0].st_size = 0;
	syms[0].st_value = 0;

	// set up 'cursors' into final symbol table so we put locals first 
	// and globals at the end
	Elf32_Sym * lsym = &syms[1];
	size_t firstGlobal = GetFirstNonLocalIndex();
	Elf32_Sym * gsym = &syms[firstGlobal];
	Elf32_Sym * lsymLim = gsym;
	iStringTable.AllocateInitialNullString();
	
	SymbolFragmentList::iterator aFrag = iSymbolFragments.begin();
	SymbolFragmentList::iterator end = iSymbolFragments.end();
	while (aFrag != end) {
		//InputFile aFile((char *)(aFrag->GetPath().c_str()));
		InputFile aFile(aFrag->GetPath());
		aFile.SetOffset(aFrag->GetSymbolTableOffset());
		size_t symSize = aFrag->GetSymbolTableSize();
		size_t limit = symSize / sizeof(Elf32_Sym);
		char * symtabData = aFile.GetData(symSize);
		Elf32_Sym * symtab = (Elf32_Sym *)symtabData;
		aFile.SetOffset(aFrag->GetStringTableOffset());
		char * strtabx = aFile.GetData(aFrag->GetStringTableSize());
		// set strtab back one to 'add' "\0" back in so indexs works with addition.
		char * strtab = strtabx - 1;
		size_t firstNonLocal = aFrag->GetFirstGlobal();
		
		typedef std::map<size_t, size_t> SymbolNdxMap;
		SymbolNdxMap symNdxMap;
		
		for (size_t i = 0; i < limit; i++){
			size_t strndx = symtab[i].st_name;
			
			if (strndx != 0) {
				// see if we've already seen this index
				SymbolNdxMap::iterator res = symNdxMap.find(strndx);
				size_t newndx;
				if (res != symNdxMap.end()){
					newndx = res->second;
					symtab[i].st_name = newndx;	
				} else {
					char * name = &strtab[strndx];
					newndx = iStringTable.AddString(name);
					symNdxMap[strndx] = symtab[i].st_name = newndx;
				}
			}
			
			if (!(symtab[i].st_value || symtab[i].st_size)){
				symtab[i].st_shndx = SHN_UNDEF;
			} else {
				size_t oldNdx = symtab[i].st_shndx;
				
				// retrieve new section index
				size_t newscnndx = aFrag->LookupSection(oldNdx);
				// retrieve the vaddr adjustment to add to the symbol's value
				int addend = aFrag->LookupVaddrAddend(oldNdx);
				symtab[i].st_shndx = newscnndx;
				symtab[i].st_value += addend;
			}
			if (i < firstNonLocal){
				assert(lsym < lsymLim);
				*(lsym++) = symtab[i];
			} else {
				*(gsym++) = symtab[i];
			}
		}
		
		delete [] symtabData;
		delete strtabx;
		
		aFrag++;
	}
	SetFileFragmentData(aFileFragmentData, symTabSize, reinterpret_cast<char *>(iData));
}


size_t ElfSymbolTableManager::Size(){
	return GetSymTabSectionSize();
}

void ElfSymbolTableManager::DeleteFileFragmentData(){
	char * d = iData;
	iData = NULL;
	delete [] d;
}

void ElfSymbolTableManager::AddData(OutputFile & aOutputFile){
	const FileFragment & aSectionFrag = iOutputFile.GetFileFragment(this);	
	SetOffset(aSectionFrag.GetOffset());
}

void ElfSymbolTableManager::AddSymbolTable(){
	// The sym table section needs to record the index of its associated
	// string table in its link field and record the index of the first non-local
	// symbol in its info field
	int symTabSize = GetSymTabSectionSize(); 
	size_t firstNonLocal = GetFirstNonLocalIndex();
	size_t nextSectionIndex = iElfSectionManager.NumSections();
	
	ElfSectionElfData * aSymTabSectionData = new ElfSectionElfData(*this);
	Elf32_Shdr symTabShdr;
	symTabShdr.sh_name = 0; // for now.
	symTabShdr.sh_type = SHT_SYMTAB;
	symTabShdr.sh_flags = 0;
	symTabShdr.sh_addr = 0;
	symTabShdr.sh_offset = 0; // for now
	symTabShdr.sh_size = symTabSize; // for now.
	// symTabShdr will be @ index nextSectionIndex so the .strtab will 
	// be @ nextSectionIndex +1
	symTabShdr.sh_link = nextSectionIndex + 1;
	symTabShdr.sh_info = firstNonLocal;
	symTabShdr.sh_addralign = 4;
	symTabShdr.sh_entsize = sizeof(Elf32_Sym);
	
	ElfSection aSymTabSection(aSymTabSectionData, ".symtab", symTabShdr);
	iElfSectionManager.AddSection(aSymTabSection);


	ElfSectionElfData * aStringTableSectionData = new ElfSectionElfData(iStringTable);
	Elf32_Shdr shdr;
	shdr.sh_name = 0; // for now.
	shdr.sh_type = SHT_STRTAB;
	shdr.sh_flags = 0;
	shdr.sh_addr = 0;
	shdr.sh_offset = 0; // for now
	shdr.sh_size = GetSymTabStringsSectionSize();
	shdr.sh_link = 0;
	shdr.sh_info = 0;
	shdr.sh_addralign = 0;
	shdr.sh_entsize = 0;
	ElfSection aStringTableSection(aStringTableSectionData, ".strtab", shdr);
	iElfSectionManager.AddSection(aStringTableSection);

}

size_t ElfSymbolTableManager::GetSymTabSectionSize(){
	int symTabSize = sizeof(Elf32_Sym); // add the 'undefined' symbols
	
	SymbolFragmentList::iterator aFrag = iSymbolFragments.begin();
	SymbolFragmentList::iterator end = iSymbolFragments.end();
	while (aFrag != end) {
		symTabSize += aFrag->GetSymbolTableSize();
		aFrag++;
	}
	return symTabSize;
}

size_t ElfSymbolTableManager::GetSymTabStringsSectionSize(){
	
	if (iSymbolStringTableSizeValid)
		return iSymbolStringTableSize;
	
	int stringsSize = 1; // add the leading "\0"
	
	SymbolFragmentList::iterator aFrag = iSymbolFragments.begin();
	SymbolFragmentList::iterator end = iSymbolFragments.end();
	while (aFrag != end) {
		stringsSize += aFrag->GetStringTableSize();
		aFrag++;
	}
	iSymbolStringTableSizeValid = true;
	return iSymbolStringTableSize = stringsSize;
}

size_t ElfSymbolTableManager::GetFirstNonLocalIndex(){
	int ndx = 1; // add the 'undefined' symbols
	
	SymbolFragmentList::iterator aFrag = iSymbolFragments.begin();
	SymbolFragmentList::iterator end = iSymbolFragments.end();
	while (aFrag != end) {
		ndx += aFrag->GetFirstGlobal();
		aFrag++;
	}
	return ndx;
}
