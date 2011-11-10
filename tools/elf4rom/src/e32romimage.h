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

#ifndef E32ROMIMAGE_H_
#define E32ROMIMAGE_H_

#include <cassert>

#include "defs.h"
#include "filefragment.h"
#include "inputfile.h"

class E32RomImage : public FileFragmentOwner {
public:
	typedef struct // this is the structure of an EPOC rom header
	{
		char name[16];
	    unsigned char versionStr[4];
	    unsigned char buildNumStr[4];
	    unsigned long romSize;
	    unsigned long wrapSize;
	} E32RomHeader;
public:
		
	E32RomImage(PathName aPath):
		iRomFile(aPath), iData(NULL)
		{};	
	
	void SetupRomData();

	// The FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData );
	virtual size_t Size() { 
		assert(iRomFile.Size() - iRomFile.GetOffset()); 
		return iRomFile.Size() - iRomFile.GetOffset(); 
	};
	virtual void DeleteFileFragmentData();
	
private:

	void Open();
	void Close();
	void SetSize();
	
private:	
	InputFile iRomFile;
	char * iData;
	E32RomHeader iE32RomHeader;

	
};

#endif /*E32ROMIMAGE_H_*/
