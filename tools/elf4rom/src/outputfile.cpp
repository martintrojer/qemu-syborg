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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cassert>

#include "outputfile.h"

static const size_t NumReservedFileFragments = 100;

OutputFile::OutputFile(PathName aFileName) :
	iFileName(aFileName), iOpened(false), iFd(-1), iCurrentOffset(0)
{
	assert(iFileName.size() != 0);
	// reserve space for file fragments
	iFileFragments.reserve(NumReservedFileFragments);
}

OutputFile::~OutputFile(){
	iFileFragments.clear();
	if (iOpened)
		Close();
}


void OutputFile::Close(){
	if (iOpened) {
		close(iFd);
		iOpened = false;
	}
}

void OutputFile::Flush(){
	if (!iOpened)
		Open();
	if (iFileFragments.size() > 0) {
		FragmentList::iterator aFrag = iFileFragments.begin();
		FragmentList::iterator end = iFileFragments.end();
		while (aFrag != end) {
			aFrag->Write(this);
			aFrag++;
		}
		iFileFragments.clear();
	}	
}

void OutputFile::Dump(){
	Flush();
	Close();
}

const FileFragment & OutputFile::GetFileFragment(FileFragmentOwner * aOwner){
	size_t aSize = aOwner->Size();
	FileFragment aFragment(iCurrentOffset, aSize, aOwner);
	iFileFragments.push_back(aFragment);
	iCurrentOffset += ALIGN4(aSize);
	return  iFileFragments.back();
}

void OutputFile::Open(){
	if (!iOpened) {
		if ((iFd = open(iFileName.c_str(), O_WRONLY | O_CREAT | O_BINARY | O_TRUNC, S_IRUSR | S_IWUSR)) == -1) {
			char msg[1000];
			sprintf(msg, "Output file %s", iFileName.c_str());
			perror(msg);
			exit(EXIT_FAILURE);
		}
		iOpened = true;
	}
}

void OutputFile::Write(size_t aOffset, size_t aSize, char * aData){
	assert(((aSize==0) && (aData==NULL)) || ((aSize>0) && (aData!=NULL)));
	if (aSize == 0) return;
	char msg[1000];
#ifndef USE_PWRITE
	if (lseek(iFd, aOffset, SEEK_SET) != (int)aOffset) {
		sprintf(msg, "Failed to seek to offset %d in output file %s", aOffset, iFileName.c_str());
		goto error;
	}
	for (size_t n = 0; n < aSize;) {
		if ((int)(n += write(iFd, aData+n, aSize - n)) == -1) {
			sprintf(msg, "Failed to write to output file %s", iFileName.c_str());
			goto error;			
		}
	}
#else
	for (size_t n = 0; n < aSize;) {
		if ((n += pwrite(iFd, aData+n, aSize - n, aOffset + n)) == -1) {
			sprintf(msg, "Failed to write to output file %s", iFileName.c_str());
			goto error;			
		}
	}	
#endif
	return;
	
error:
		perror(msg);
// should we close the file and delete it??		
#ifdef DELETE_OUTPUT_ON_ERROR
		Close();
		unlink(iFileName);
#endif
		exit(EXIT_FAILURE);
}
