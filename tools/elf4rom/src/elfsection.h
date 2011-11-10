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

#ifndef ELFSECTION_H_
#define ELFSECTION_H_

#include <libelf.h>

#include "defs.h" // for String

#include "filefragment.h"
#include "inputfile.h"

class ElfSectionHeader {
public:
	ElfSectionHeader()
		{}
	
	ElfSectionHeader(Elf32_Shdr & aShdr) :
		iElf32Shdr(aShdr)
		{}
	
	Elf32_Shdr iElf32Shdr;
};

class ElfSectionData  : public FileFragmentOwner {
public:
	ElfSectionData()
		{}
	ElfSectionData(size_t aOffset):
		FileFragmentOwner(aOffset)
		{}
	virtual ElfSectionData * Clone() = 0;
	
	// The FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData ) = 0;
	virtual size_t Size() = 0;
	virtual void DeleteFileFragmentData() = 0;


};

class ElfSectionRomData : public ElfSectionData {
public:
	ElfSectionRomData(size_t aOffset, size_t aSize) :
		ElfSectionData(aOffset), 
		iSize(aSize)
		{}
	
	ElfSectionRomData(const ElfSectionRomData & aData);

	// ElfSection protocol
	virtual ElfSectionRomData * Clone();

	// The FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData ){}
	virtual size_t Size(){ return iSize; }
	// Nothing to delete
	virtual void DeleteFileFragmentData(){};
	// Dont add data;
	virtual void AddData(OutputFile & aOutputFile) {};
	
private:
	size_t iSize;
};

class ElfSectionElfData : public ElfSectionData {
public:
	ElfSectionElfData(FileFragmentOwner & aSource) :
		iSource(aSource)
		{}
	
	ElfSectionElfData(const ElfSectionElfData & aData);
	
	// ElfSection protocol
	virtual ElfSectionElfData * Clone();

	// The FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData );
	virtual size_t Size();
	virtual void DeleteFileFragmentData();
	virtual void AddData(OutputFile & aOutputFile);
	virtual size_t GetOffset();
	
private:
	FileFragmentOwner & iSource;
};

class ElfSectionFileData : public ElfSectionData {
public:
	ElfSectionFileData(InputFile * aInputFile) :
		iInputFile(aInputFile), iData(NULL)
		{}
	ElfSectionFileData(const ElfSectionFileData & aData);
	
	// ElfSection protocol
	virtual ElfSectionFileData * Clone();

	// The FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData );
	virtual size_t Size();
	virtual void DeleteFileFragmentData();

private:
	InputFile * iInputFile;
	char * iData;
};

class ElfSectionNoData : public ElfSectionData {
public:
	ElfSectionNoData(){}
	
	ElfSectionNoData(const ElfSectionNoData & aData);
	
	// ElfSection protocol
	virtual ElfSectionNoData * Clone();

	// The FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData );
	virtual size_t Size() { return 0; }
	// Nothing to delete
	virtual void DeleteFileFragmentData(){};
	// Dont add data;
	virtual void AddData(OutputFile & aOutputFile) {};



private:
	InputFile * iInputFile;
};

class ElfSection {
public:
	ElfSection(ElfSectionData *  aData) :
		iSectionName(""), iSectionData(aData), iIndex(0)
		{}
	
	ElfSection(ElfSectionData *  aData, String aName, Elf32_Shdr & aShdr) :
		iSectionName(aName), iSectionHdr(aShdr), iSectionData(aData)
		{}
	
	ElfSection(const ElfSection & aData);
	
	ElfSection & operator=(const ElfSection & aSection);
	
	virtual ~ElfSection();
	
	String & GetName() { return iSectionName ; }
	void SetNameOffset(size_t nameOffset) { iSectionHdr.iElf32Shdr.sh_name = nameOffset; }
	
	ElfSectionHeader & GetSectionHeader() { return iSectionHdr; }
	
	void SetSize(size_t aSize) { iSectionHdr.iElf32Shdr.sh_size = aSize; }
	void SetOffset(size_t aOffset) { iSectionHdr.iElf32Shdr.sh_offset = aOffset; }
	
	virtual void AddData(OutputFile & aOutputFile);
	unsigned int GetIndex() { return iIndex; };
	void SetIndex(unsigned int aIndex) { iIndex = aIndex; };
	
private:
	String iSectionName;
	ElfSectionHeader iSectionHdr;
	ElfSectionData * iSectionData;
	unsigned int	iIndex;
};

#endif /*ELFSECTION_H_*/
