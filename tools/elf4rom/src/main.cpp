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
#include <libelf.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include "romdetails.h"
#include "processoptions.h"

#include "elfromerror.h"
#include "elfrom.h"


void InitLibElf() {
	if (elf_version(EV_CURRENT) == EV_NONE) 
		errx(EX_SOFTWARE, "ELF library initialization failed: %s\n",
				elf_errmsg(-1));
}

RomDetails * Init(int argc, char * argv[]){
	RomDetails * details = ProcessOptions(argc, argv);
	return ProcessRomDetails(details);
}

int main(int argc, char * argv[]){
//TODO: print banner if trace on
	InitLibElf();
	RomDetails * romDetails = Init(argc, argv);
	ElfRom aElfRom(romDetails);
	aElfRom.SetupE32RomData();
	aElfRom.SetupELFRomData();
	aElfRom.AddData();
	
	aElfRom.Dump();
	
	return EXIT_SUCCESS;
}
