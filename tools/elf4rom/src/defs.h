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

#ifndef DEFS_H_
#define DEFS_H_

#include <string>
typedef std::string String;

#if 0
typedef char * PathName;
#endif

typedef String PathName;

typedef unsigned long LinearAddr;
typedef unsigned long VirtualAddr;

#define ALIGN4(x) (((x)+ 3) & ~3)

typedef unsigned long Uint32;
typedef long Int32;

typedef char Byte;
typedef char * Byteptr;

#endif /*DEFS_H_*/
