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

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <cassert>

#include "elfromerror.h"
#include "inputfile.h"

InputFile & InputFile::operator=(const InputFile & aInputFile) {
	// Don't copy fd. Force a new to be got.
	iOpened = false;
	// Force size to be re-computed
	iSizeValid = false;
	iOffset = aInputFile.iOffset;
	iPathName = aInputFile.iPathName;
	return *this;
}

InputFile::InputFile(const InputFile & aInputFile){
	*this = aInputFile;
}

InputFile::~InputFile(){
	Close();
}

void InputFile::Open(){
	if (iOpened) return;
	if ((iFd = open(iPathName.c_str(), O_RDONLY|O_BINARY, 0)) < 0)
		errx(EX_NOINPUT, "open \"%s\" failed\n", iPathName.c_str());
	else
		iOpened = true;
}

void InputFile::Close(){
	if (!iOpened) return;
	close(iFd);
	iOpened = false;
	iSizeValid = false;
}

size_t InputFile::Size(){
	if (iSizeValid) return iSize - iOffset;
	SetSize();
	return iSize - iOffset;
}

void InputFile::SetSize(){
	Open();
	if ((iSize = lseek(iFd, (size_t)0, SEEK_END)) == (size_t)-1)
		errx(EX_NOINPUT, "failed to get size of \"%s\"", iPathName.c_str());
	if (lseek(iFd, (size_t)0, SEEK_SET) != 0)
		errx(EX_NOINPUT, "failed to get size of \"%s\"", iPathName.c_str());
	Close();
	iSizeValid = true;
}

char * InputFile::GetData(){
	size_t nbytes = Size();
	return GetData(nbytes);
}

char * InputFile::GetData(size_t nbytes) {
	char * data = new char[nbytes];
	GetData(data, nbytes);
	return data;
}

void InputFile::GetData(void * data, size_t nbytes){
	char * d = (char *)data;
	size_t done = 0;
	ssize_t n;
	
	Open();
	
	if (iOffset > 0)
		if ((size_t)lseek(iFd, iOffset, SEEK_SET) != iOffset) {
			Close();
			errx(EX_NOINPUT, "failed to read from \"%s\"", iPathName.c_str());
		}
	
	while (nbytes){
		n = read(iFd, d+done,nbytes);
		// n must be greater than 0
		if (n <= 0){
			Close();
			errx(EX_NOINPUT, "failed to read from \"%s\"", iPathName.c_str());
		}
		done += n;
		nbytes -= n;
	}
	
	Close();
}
