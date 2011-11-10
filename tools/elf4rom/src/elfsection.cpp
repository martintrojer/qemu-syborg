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

#include "outputfile.h"
#include "inputfile.h"
#include "filefragment.h"
#include "elfsection.h"


ElfSection & ElfSection::operator=(const ElfSection & aSection) {
	iSectionName = aSection.iSectionName;
	iSectionHdr = aSection.iSectionHdr;
	iSectionData = aSection.iSectionData->Clone();
	iIndex = aSection.iIndex;
	return *this;
}

ElfSection::ElfSection(const ElfSection & aSection){
	*this = aSection;
}

ElfSectionRomData::ElfSectionRomData(const ElfSectionRomData & aData) {
	iOffset = aData.iOffset;
	iSize = aData.iSize;
}

ElfSectionRomData * ElfSectionRomData::Clone(){
	return new ElfSectionRomData(*this);
}

ElfSectionElfData::ElfSectionElfData(const ElfSectionElfData & aData) :
	iSource(aData.iSource)
{
	iOffset = aData.iOffset;
}

ElfSectionElfData * ElfSectionElfData::Clone(){
	return new ElfSectionElfData(*this);
}

void ElfSectionElfData::AddData(OutputFile & aOutputFile){
	iSource.AddData(aOutputFile);
}

void ElfSectionElfData::GetFileFragmentData(FileFragmentData & aFileFragmentData ){
	iSource.GetFileFragmentData(aFileFragmentData);
}

void ElfSectionElfData::DeleteFileFragmentData(){
	iSource.DeleteFileFragmentData();	
}

size_t ElfSectionElfData::Size(){
	return iSource.Size();
}

size_t ElfSectionElfData::GetOffset(){
	return iSource.GetOffset();
}

ElfSectionFileData::ElfSectionFileData(const ElfSectionFileData & aData) {
	iOffset = aData.iOffset;
	iInputFile = aData.iInputFile;
	iData = aData.iData;
}

ElfSectionFileData * ElfSectionFileData::Clone(){
	return new ElfSectionFileData(*this);
}

void ElfSectionFileData::GetFileFragmentData(FileFragmentData & aFileFragmentData ){
	iData = iInputFile->GetData();
	SetFileFragmentData(aFileFragmentData, iInputFile->Size(), (char *)iData);
}

void ElfSectionFileData::DeleteFileFragmentData(){
	if (iData) {
		char * d = iData;
		iData = NULL;
		delete [] d;
	}
}

size_t ElfSectionFileData::Size(){
	return iInputFile->Size();
}

ElfSectionNoData::ElfSectionNoData(const ElfSectionNoData & aData) {
	iOffset = aData.iOffset;
}

ElfSectionNoData * ElfSectionNoData::Clone(){
	return new ElfSectionNoData(*this);
}

void ElfSectionNoData::GetFileFragmentData(FileFragmentData & aFileFragmentData ){
	SetFileFragmentData(aFileFragmentData, 0u, reinterpret_cast<char *>(NULL));
}

ElfSection::~ElfSection(){
	delete iSectionData;
}

void ElfSection::AddData(OutputFile & aOutputFile){
	iSectionData->AddData(aOutputFile);
	SetSize(iSectionData->Size());
	SetOffset(iSectionData->GetOffset());
}
