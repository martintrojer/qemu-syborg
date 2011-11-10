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

#ifndef OUTPUTFILE_H_
#define OUTPUTFILE_H_

#include <vector>

#include "defs.h"
#include "filefragment.h"

// uncomment this is output file should be deleted on error
//#define DELETE_OUTPUT_ON_ERROR
// uncomment to use pwrite (this should be more efficient) but results in different
// file pointer behaviour. NB not defined by MINGW in unistd.h 
//#define USE_PWRITE

// TODO: NB currently this can only deal with 4gb files. Need to upgrade for larger files
class OutputFile
{
public:
	OutputFile(PathName aFileName);
	virtual ~OutputFile();

	virtual void Close();
	virtual void Flush();
	virtual void Dump();
	virtual const FileFragment & GetFileFragment(FileFragmentOwner * aOwner);
	virtual void Open();
	virtual void Write(size_t aOffset, size_t aSize, char * aData);
	virtual size_t Size(){ return iCurrentOffset; }

private:
	// Don't want one of these to be copied
	OutputFile(const OutputFile & aOutputFile);
	
	OutputFile & operator=(const OutputFile & aOutputFile);
	
private:
	typedef std::vector<FileFragment> FragmentList;
	
	PathName iFileName;
	FragmentList iFileFragments;
	bool iOpened;
	int iFd;
	size_t iCurrentOffset;

};

#endif /*OUTPUTFILE_H_*/
