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

#ifndef ELFSYMBOLTABLEMANAGER_H_
#define ELFSYMBOLTABLEMANAGER_H_

#include <vector>
#include <cassert>

#include "filefragment.h"
#include "elfstringtable.h"
#include "outputfile.h"
#include "elfheader.h"
#include "e32romimage.h"
#include "elfsection.h"
#include "elfsectionmanager.h"
#include "defs.h" // String

class ElfSymbolTableManager;

class SectionNumberMapping {
public:
	SectionNumberMapping(size_t aOld, size_t aNew):
		iOld(aOld), iNew(aNew)
		{}
	
	SectionNumberMapping & operator=(const SectionNumberMapping & aSectionNumberMapping){
		iOld = aSectionNumberMapping.iOld;
		iNew = aSectionNumberMapping.iNew;
		return *this;
	}
	
	SectionNumberMapping(const SectionNumberMapping & aSectionNumberMapping){
		*this = aSectionNumberMapping;
	}

	size_t iOld;
	size_t iNew;
};

typedef std::vector< SectionNumberMapping > SectionNumberMap;

class SectionVaddrAddendMapping {
public:
	SectionVaddrAddendMapping(size_t aSectionNumber, signed int aAddend):
		iSectionNumber(aSectionNumber), iAddend(aAddend)
		{}
	
	SectionVaddrAddendMapping & operator=(const SectionVaddrAddendMapping & aSectionVaddrAddendMapping){
		iSectionNumber = aSectionVaddrAddendMapping.iSectionNumber;
		iAddend = aSectionVaddrAddendMapping.iAddend;
		return *this;
	}
	
	SectionVaddrAddendMapping(const SectionVaddrAddendMapping & SectionVaddrAddendMapping){
		*this = SectionVaddrAddendMapping;
	}

	size_t iSectionNumber;
	signed int iAddend;
};

typedef std::vector< SectionVaddrAddendMapping > SectionVaddrAddendMap;

class ElfFileSymbolFragments {
public:
	ElfFileSymbolFragments(ElfSymbolTableManager * aElfSymbolTableManager):
		iPath(""), 
		iSymbolTableOffset(0), iSymbolTableSize(0),
		iFirstGlobal(0),
		iStringTableOffset(0), iStringTableSize(0),
		iElfSymbolTableManager(aElfSymbolTableManager)
		{
		iSectionNumberMap.clear();
		iSectionVaddrAddendMap.clear();
		}
	
	ElfFileSymbolFragments & operator=(const ElfFileSymbolFragments & aElfFileSymbolFragments){
		iPath = aElfFileSymbolFragments.iPath;
		iSymbolTableOffset = aElfFileSymbolFragments.iSymbolTableOffset;
		iSymbolTableSize = aElfFileSymbolFragments.iSymbolTableSize;
		iFirstGlobal = aElfFileSymbolFragments.iFirstGlobal;
		iStringTableOffset = aElfFileSymbolFragments.iStringTableOffset;
		iStringTableSize = aElfFileSymbolFragments.iStringTableSize;
		iSectionNumberMap = aElfFileSymbolFragments.iSectionNumberMap;
		iSectionVaddrAddendMap = aElfFileSymbolFragments.iSectionVaddrAddendMap;
		iElfSymbolTableManager = aElfFileSymbolFragments.iElfSymbolTableManager;
		return *this;
	}
	
	ElfFileSymbolFragments(const ElfFileSymbolFragments & aElfFileSymbolFragments){
		*this = aElfFileSymbolFragments;
	}
	
	

	void AddSymbolTable(String & aPath, size_t aOffset, size_t aSize, size_t aFirstGlobal);
	
	void AddStringTable(String & aPath, size_t aOffset, size_t aSize);
	String & GetPath() { return iPath; }
	size_t GetSymbolTableOffset() { return iSymbolTableOffset; }
	size_t GetSymbolTableSize() { return iSymbolTableSize; }
	size_t GetFirstGlobal() { return iFirstGlobal; }
	size_t GetStringTableOffset() { return iStringTableOffset; }
	size_t GetStringTableSize() { return iStringTableSize; }
	
	void SetSectionNumberMap(SectionNumberMap & aSectionNumberMap){
		iSectionNumberMap = aSectionNumberMap;
	}
	
	size_t LookupSection(size_t ndx);
	
	void SetSectionVaddrAddendMap(SectionVaddrAddendMap & aSectionVaddrAddendMap){
		iSectionVaddrAddendMap = aSectionVaddrAddendMap;
	}
	
	int LookupVaddrAddend(size_t ndx);
	
	void Validate() {
		assert(iPath.size() != 0);
		assert(iSymbolTableOffset != 0);
		assert(iSymbolTableSize != 0);
		assert(iStringTableOffset != 0);
		assert(iStringTableSize != 0);
		assert(!iSectionNumberMap.empty());
		assert(!iSectionVaddrAddendMap.empty());
	}
	
	void Reset(){
		new (this) ElfFileSymbolFragments(iElfSymbolTableManager);
	}
	
private:
	String iPath;
	size_t iSymbolTableOffset;
	size_t iSymbolTableSize;
	size_t iFirstGlobal;
	size_t iStringTableOffset;
	size_t iStringTableSize;
	SectionNumberMap iSectionNumberMap;
	SectionVaddrAddendMap iSectionVaddrAddendMap;
	ElfSymbolTableManager * iElfSymbolTableManager;
	
};

class ElfSymTabStringTable : public ElfStringTable {
public:
	ElfSymTabStringTable(ElfSymbolTableManager & aElfSymbolTableManager):
		iElfSymbolTableManager(aElfSymbolTableManager)
		{}
	
	virtual size_t Size();
	
private:
	ElfSymbolTableManager & iElfSymbolTableManager;
};

class ElfSymbolTableManager : public FileFragmentOwner {
public:
	ElfSymbolTableManager(Elf32Header & aElf32Header, E32RomImage & aRomImage, 
					      OutputFile & aOutputFile, ElfSectionManager & aElfSectionManager) :
		iElf32Header(aElf32Header), iRomImage(aRomImage), 
		iOutputFile(aOutputFile), iElfSectionManager(aElfSectionManager),
		iData(NULL),
		iSymbolStringTableSizeValid(false),
		iSymbolStringTableSize(0),
		iCurrentFragment(this),
		iStringTable(*this)
		{}
	
	// The FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData );
	virtual size_t Size();
	virtual void DeleteFileFragmentData();
	virtual void AddData(OutputFile & aOutputFile);
	
	void AddSymbolTable(String & aPath, size_t aOffset, size_t aSize, size_t aFirstGlobal){
		iCurrentFragment.AddSymbolTable(aPath, aOffset, aSize, aFirstGlobal);
	}
	void AddStringTable(String & aPath, size_t aOffset, size_t aSize){
		iCurrentFragment.AddStringTable(aPath, aOffset, aSize);

	}
	virtual void AddSymbolFragment(){
		iSymbolFragments.push_back(iCurrentFragment);
	}
	virtual void AddSymbolTable();
	
	size_t GetSymTabStringsSectionSize();

	virtual void Finalize(SectionNumberMap & aSectionNumberMap, SectionVaddrAddendMap & aSectionVaddrAddendMap);
	
private:
	// Don't want one of these to be copied
	ElfSymbolTableManager(const ElfSymbolTableManager & aElfSymbolTableManager);
	
	ElfSymbolTableManager & operator=(const ElfSymbolTableManager & aElfSymbolTableManager);
	
	size_t GetSymTabSectionSize();
	size_t GetFirstNonLocalIndex();

private:
	typedef std::vector<ElfFileSymbolFragments> SymbolFragmentList;
private:
	Elf32Header & iElf32Header;
	E32RomImage & iRomImage;
	OutputFile & iOutputFile;
	ElfSectionManager & iElfSectionManager;
	
	char * iData;
	bool iSymbolStringTableSizeValid;
	size_t iSymbolStringTableSize;
	ElfFileSymbolFragments iCurrentFragment;
	SymbolFragmentList iSymbolFragments;
	ElfSymTabStringTable iStringTable;
};

#endif /*ELFSYMBOLTABLEMANAGER_H_*/
