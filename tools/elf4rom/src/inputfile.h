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

#ifndef INPUTFILE_H_
#define INPUTFILE_H_

#include <cassert>

#include "defs.h"

class InputFile {
public:
	InputFile(PathName & aPath) :
		iPathName(aPath), iFd(0), iOpened(false), iSize(0), iSizeValid(false), iOffset(0)
		{
			assert(iPathName.size() != 0);
		}
	~InputFile();
	void Open();
	void Close();
	size_t Size();
	char * GetData();
	char * GetData(size_t nbytes);
	void GetData(void * data, size_t nbytes);
	void SetOffset(size_t aOffset) { iOffset = aOffset; }
	size_t GetOffset() { return iOffset; }
	
	InputFile & operator=(const InputFile & aInputFile);
	InputFile(const InputFile & aInputFile);
	

private:
	
	void SetSize();
private:
	PathName iPathName;
	int iFd;
	bool iOpened;
	size_t iSize;
	bool iSizeValid;
	size_t iOffset;
};

#endif /*INPUTFILE_H_*/
