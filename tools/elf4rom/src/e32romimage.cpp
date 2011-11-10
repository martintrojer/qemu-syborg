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
#include <cstring>

#include "elfromerror.h"
#include "e32romimage.h"


void E32RomImage::SetupRomData(){
	// see if there's a header on the image.
	iRomFile.GetData(&iE32RomHeader, sizeof(iE32RomHeader));
	
	// In general the 'name' is of the form EPOC*ROM.
	// We are looking for ARM roms for the moment so check for EPOCARM and error on anything
	// else which matches EPOC*ROM
    bool isEPOCHeader = !strncmp((const char *)&iE32RomHeader.name[0], "EPOC", 4) && 
    				(strstr((const char *)&iE32RomHeader.name[0], "ROM") != NULL);
    bool isARMHeader = !strncmp((const char *)&iE32RomHeader.name[0], "EPOCARM", 7);

    if(isARMHeader)
    	iRomFile.SetOffset(sizeof(iE32RomHeader));
    else if (isEPOCHeader) 
    	errx(EX_NOINPUT, "unsupported rom type: %s\n", (const char *)&iE32RomHeader.name[0]);
}

void E32RomImage::GetFileFragmentData(FileFragmentData & aFileFragmentData ){
	assert(iRomFile.Size() > 0);
	if (!iData)
		iData = iRomFile.GetData();
	SetFileFragmentData(aFileFragmentData, Size(), reinterpret_cast<char *>(iData));
}

void E32RomImage::DeleteFileFragmentData(){ 
	if (iData) {
		char * d = iData;
		iData = NULL;
		delete [] d; 
	}
}
