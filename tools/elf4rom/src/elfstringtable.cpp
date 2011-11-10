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

#include "elfstringtable.h"

void ElfStringTable::GetFileFragmentData(FileFragmentData & aFileFragmentData ){
	size_t nbytes = Size();
	iData = new char[nbytes];
	memset(iData, 0, nbytes);
	if (iStrings.size() > 0) {
		StringList::iterator aString = iStrings.begin();
		StringList::iterator end = iStrings.end();
		while (aString != end) {
			memcpy(iData + aString->iOffset, aString->iName.data(), aString->iName.size()); 
			aString++;
		}
	}
	SetFileFragmentData(aFileFragmentData, nbytes, reinterpret_cast<char *>(iData));
}

size_t ElfStringTable::Size() {
	return iSize;
}

void ElfStringTable::DeleteFileFragmentData(){
	if (iData) {
		char * d = iData;
		iData = NULL;
		delete [] d;
	}
}

size_t ElfStringTable::AddString(String aString){
	size_t offset = iSize;
	StringTableEntry sn(aString, offset);
	iStrings.push_back(sn);
	size_t ss = aString.size();
	iSize += (ss + 1);
	return offset;
}
