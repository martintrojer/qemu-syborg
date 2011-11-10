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

#include "elfsectionmanager.h"

void ElfSectionManager::GetFileFragmentData(FileFragmentData & aFileFragmentData ){
	int nSections = iSections.size();
	if (nSections > 0){
		iData = new Elf32_Shdr[nSections];
		Elf32_Shdr * p = iData;
		SectionList::iterator aSection = iSections.begin();
		SectionList::iterator end = iSections.end();
		while (aSection != end) {
			*p = aSection->GetSectionHeader().iElf32Shdr;
			p++;
			aSection++;
		}
		SetFileFragmentData(aFileFragmentData, Size(), reinterpret_cast<char *>(iData));
	}
}

size_t ElfSectionManager::Size(){
	return SectionHeaderSize();
}

void ElfSectionManager::DeleteFileFragmentData(){
	if (iData) {
		Elf32_Shdr * d = iData;
		iData = NULL;
		delete[] d;
	}
}

void ElfSectionManager::AddData(){
	AddData(iOutputFile);
}

void ElfSectionManager::AddData(OutputFile & aOutputFile){
	if (iSections.size() > 0) {
		SectionList::iterator aSection = iSections.begin();
		SectionList::iterator end = iSections.end();
		while (aSection != end) {
			aSection->AddData(aOutputFile);
			aSection++;
		}
		AddSectionTable();
	}
}

void ElfSectionManager::EnsureSectionStringTableSectionAdded(){
	if (iSectionStringTableSectionAdded) return;
	
	// set iSectionStringTableSectionAdded true now so we don't do the next bit twice;
	iSectionStringTableSectionAdded = true;

	// first create the UNDEF section
	ElfSectionNoData * aUndefData = new ElfSectionNoData();
	
	Elf32_Shdr undef;
	undef.sh_name = 0; 
	undef.sh_type = SHT_NULL;
	undef.sh_flags = 0;
	undef.sh_addr = 0;
	undef.sh_offset = 0; 
	undef.sh_size = 0; 
	undef.sh_link = 0;
	undef.sh_info = 0;
	undef.sh_addralign = 0;
	undef.sh_entsize = 0;
	
	ElfSection aUndefSection(aUndefData, "", undef);
	AddSection(aUndefSection);
	
	ElfSectionElfData * aSectionStringTableSectionData = new ElfSectionElfData(iStringTable);
	Elf32_Shdr shdr;
	shdr.sh_name = 0; // for now.
	shdr.sh_type = SHT_STRTAB;
	shdr.sh_flags = 0;
	shdr.sh_addr = 0;
	shdr.sh_offset = 0; // for now
	shdr.sh_size = 0; // for now.
	shdr.sh_link = SHN_UNDEF;
	shdr.sh_info = 0;
	shdr.sh_addralign = 0;
	shdr.sh_entsize = 0;

	ElfSection aSectionStringTableSection(aSectionStringTableSectionData, ".shstrtab", shdr);
	AddSection(aSectionStringTableSection);
	iElf32Header.SetSectionStringNdx(iSections.size() - 1);
}
void ElfSectionManager::AddSection(ElfSection & aSection){
	EnsureSectionStringTableSectionAdded();
	if (aSection.GetName().size() > 0){
		// rename sections for GDB (yuk!)
		String sectionName(aSection.GetName());
		String ro("ER_RO");
		String text(".text");
		if (sectionName == ro){
			sectionName = text;
		} else {
			String rw("ER_RW");
			String data(".data");
			if (sectionName == rw){
				sectionName = data;
			} else {
				String zi("ER_ZI");
				String bss(".bss");
				if (sectionName == zi)
					sectionName = bss;
			}
		}
		size_t nameOffset = iStringTable.AddName(sectionName);
		aSection.SetNameOffset(nameOffset);
	} else {
		// use the initial Null String.
		size_t nameOffset = iStringTable.AllocateInitialNullString();
		aSection.SetNameOffset(nameOffset);
	}
	aSection.SetIndex(iSections.size());
	iSections.push_back(aSection);
	iElf32Header.AddSectionHdr();
}
	
size_t ElfSectionManager::SectionHeaderSize(){
	return iSections.size() * sizeof(Elf32_Shdr);
}

void ElfSectionManager::AddSectionStringTable(){
	// Assume the section header already setup and we've got hold of it. We need to say where the strings ended up.
	const FileFragment & aSectionTableFrag = iOutputFile.GetFileFragment(this);	
	SetOffset(aSectionTableFrag.GetOffset());
}

void ElfSectionManager::AddSectionTable(){
	const FileFragment & aSectionTableFrag = iOutputFile.GetFileFragment(this);	
	SetOffset(aSectionTableFrag.GetOffset());
	iElf32Header.SetSectionHdrOffset(GetOffset());
}
