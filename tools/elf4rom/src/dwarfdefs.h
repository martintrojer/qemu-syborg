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

#ifndef DWARFDEFS_H_
#define DWARFDEFS_H_

//#include <libdwarf.h>
/* to identify a cie */


#define DW_CIE_ID 		~(0x0)
#define DW_CIE_VERSION		1 /* DWARF2 */
#define DW_CIE_VERSION3		3 /* DWARF3 */
#define ABBREV_HASH_TABLE_SIZE	10

typedef int                 Dwarf_Bool;     /* boolean type */
//
// Copyright (c) 2008 Symbian Software Ltd. All rights reserved.
//

typedef unsigned long long  Dwarf_Off;      /* 8 byte file offset */
typedef unsigned long long  Dwarf_Unsigned; /* 8 byte unsigned value*/
typedef unsigned short      Dwarf_Half;     /* 2 byte unsigned value */
typedef unsigned char       Dwarf_Small;    /* 1 byte unsigned value */
typedef signed   long long  Dwarf_Signed;   /* 8 byte signed value */
typedef unsigned long long  Dwarf_Addr;     /* target memory address */

typedef void*		Dwarf_Ptr;          /* host machine pointer */

typedef unsigned long Dwarf_Word;
typedef signed long Dwarf_Sword;

typedef signed char Dwarf_Sbyte;
typedef unsigned char Dwarf_Ubyte;
typedef signed short Dwarf_Shalf;
typedef Dwarf_Ubyte *Dwarf_Byte_Ptr;

/* these 2 are fixed sizes which must not vary with the
** ILP32/LP64 model. Between these two, stay at 32 bit.
*/
typedef unsigned int Dwarf_ufixed;
typedef int Dwarf_sfixed;

/*
        In various places the code mistakenly associates
        forms 8 bytes long with Dwarf_Signed or Dwarf_Unsigned
	This is not a very portable assumption.
        The following should be used instead for 64 bit integers.
*/
typedef unsigned long long Dwarf_ufixed64;
typedef long long Dwarf_sfixed64;

char * GetDwarfTag(Dwarf_Unsigned aTag);
char * GetDwarfAttr(Dwarf_Half attr);
char * GetDwarfForm(Dwarf_Half form);

#define READ_UNALIGNED2(aPtr)ReadUnaligned2(aPtr)
static inline Dwarf_Word ReadUnaligned2(Dwarf_Byte_Ptr aPtr){
	Dwarf_Byte_Ptr p = aPtr;
	return p[0] | (p[1] << 8); 
}
#define READ_UNALIGNED4(aPtr)ReadUnaligned4(aPtr)
static inline Dwarf_Word ReadUnaligned4(Dwarf_Byte_Ptr aPtr){
	Dwarf_Byte_Ptr p = aPtr;
	return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); 
}
#define WRITE_UNALIGNED2(aPtr, val)WriteUnaligned2(aPtr, val)
static inline void WriteUnaligned2(Dwarf_Byte_Ptr aPtr, Dwarf_Word val){
	Dwarf_Byte_Ptr p = aPtr;
	p[0] = val & 0xff;
	p[1] = (val >> 8) & 0xff;
}

#define WRITE_UNALIGNED4(aPtr, val)WriteUnaligned4(aPtr, val)
static inline void WriteUnaligned4(Dwarf_Byte_Ptr aPtr, Dwarf_Word val){
	Dwarf_Byte_Ptr p = aPtr;
	p[0] = val & 0xff;
	p[1] = (val >> 8) & 0xff;
	p[2] = (val >> 16) & 0xff;
	p[3] = (val >> 24) & 0xff;
}

#include <iostream>
static inline Dwarf_Word GetValue(Dwarf_Byte_Ptr start, size_t size){
	switch (size){
	default:
	case 0: {
		std::cerr << "Error: size of " << size << " not allowed\n";
		exit(EXIT_FAILURE);
		}
	case 2:
		return READ_UNALIGNED2(start);
	case 4:
		return READ_UNALIGNED4(start);
	case 8: {
		std::cerr << "Error: 64 bit values not support yet\n";
		exit(EXIT_FAILURE);		
		}
	}	
}

static inline void WriteValue(Dwarf_Byte_Ptr start, Dwarf_Word val, size_t size){
	switch (size){
	default:
	case 0: {
		std::cerr << "Error: size of " << size << " not allowed\n";
		exit(EXIT_FAILURE);
		}
	case 2:
		WRITE_UNALIGNED2(start, val);
		break;
	case 4:
		WRITE_UNALIGNED4(start, val);
		break;
	case 8: {
		std::cerr << "Error: 64 bit values not support yet\n";
		exit(EXIT_FAILURE);		
		}
	}
	
}

// TODO: for the moment just say 4 - we are dealing with 32 bit ARM for the foreseeable future
// in the future we need to figure out where to get this from...
#define ENCODED_POINTER_SIZE 4

#endif /*DWARFDEFS_H_*/
