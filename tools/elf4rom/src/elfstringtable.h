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

#ifndef ELFSTRINGTABLE_H_
#define ELFSTRINGTABLE_H_

//
// Copyright (c) 2008 Symbian Software Ltd. All rights reserved.
//

#include <vector>

#include "defs.h"
#include "filefragment.h"
#include "outputfile.h"

class ElfStringTable : public FileFragmentOwner {
public:
	ElfStringTable() :
		iSize(0), iData(NULL)
		{}
	
	~ElfStringTable(){
		iStrings.clear();
		if (iData) {
			char * d = iData;
			iData = NULL;
			delete d;
		}
	}
	
	// The FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData );
	virtual size_t Size();
	virtual void SetSize(size_t aSize) 
		{ iSize = aSize; }
	virtual void DeleteFileFragmentData();
	
	size_t AddString(String aName);
	size_t AllocateInitialNullString() {
		if (iSize == 0) {
			StringTableEntry sn("", 0);
			iStrings.push_back(sn);
			iSize = 1;
		}
		// The initial string is always at offset 0 in the section.
		return 0;
	}
	
private:
	class StringTableEntry {
	public:
		StringTableEntry(String aName, size_t aOffset):
			iName(aName), iOffset(aOffset)
			{}

		String iName;
		size_t iOffset;
	};

	typedef std::vector<StringTableEntry> StringList;
	
	StringList iStrings;
	size_t iSize;
	char * iData;
};

#endif /*ELFSTRINGTABLE_H_*/
