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

#ifndef ELFSECTIONMANAGER_H_
#define ELFSECTIONMANAGER_H_

#include <vector>

#include "filefragment.h"
//
// Copyright (c) 2008 Symbian Software Ltd. All rights reserved.
//

#include "elfstringtable.h"
#include "outputfile.h"
#include "elfheader.h"
#include "e32romimage.h"
#include "elfsection.h"
#include "defs.h" // String


class ElfSectionHeaderStringTable : public ElfStringTable {
public:
	size_t AddName(String aName) {
		return AddString(aName);
	}

};

class ElfSectionManager : public FileFragmentOwner {
public:
	ElfSectionManager(Elf32Header & aElf32Header, E32RomImage & aRomImage, OutputFile & aOutputFile) :
		iElf32Header(aElf32Header), iRomImage(aRomImage), 
		iOutputFile(aOutputFile), iOffset(0), iData(NULL),
		iSectionStringTableSectionAdded(false)
		{}
	
	// The FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData );
	virtual size_t Size();
	virtual void DeleteFileFragmentData();
	virtual void AddData(OutputFile & aOutputFile);
	
	void AddData();
	void AddSection(ElfSection & aSection);
	
	int NumSections() { return iSections.size(); }
	
private:
	// Don't want one of these to be copied
	ElfSectionManager(const ElfSectionManager & aElfSectionManager);
	
	ElfSectionManager & operator=(const ElfSectionManager & aElfSectionManager);
	
private:
	size_t SectionHeaderSize();
	void AddSectionStringTable();
	void AddSectionTable();
	void EnsureSectionStringTableSectionAdded();
	
private:
	typedef std::vector<ElfSection> SectionList;
	Elf32Header & iElf32Header;
	E32RomImage & iRomImage;
	OutputFile & iOutputFile;
	SectionList iSections;
	size_t iOffset;
	ElfSectionHeaderStringTable iStringTable;
	Elf32_Shdr * iData;
	bool iSectionStringTableSectionAdded;
	
};

#endif /*ELFSECTIONMANAGER_H_*/
