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

#ifndef FILEFRAGMENT_H_
#define FILEFRAGMENT_H_

#include <stddef.h>

class OutputFile;

class FileFragmentData
{

public:
	FileFragmentData() :
		iSize(0), iData(0)
		{};
	FileFragmentData(size_t aSize, char * aData) :
		iSize(aSize), iData(aData)
		{};
	virtual ~FileFragmentData();
	inline char* GetData() { return iData; };
	inline size_t GetSize() { return iSize; };
	inline void SetData(char * newVal) { iData = newVal; };
	inline void SetSize(size_t newVal) { iSize = newVal; };

private:
	size_t iSize;
	char* iData;
};

/**
 * Defines the interface for owners of FileFragments
 */
class FileFragmentOwner
{
public:
	FileFragmentOwner() :
		iOffset(0)
		{};
		
	FileFragmentOwner(size_t aOffset) :
		iOffset(aOffset)
		{};	
		
	virtual ~FileFragmentOwner();

	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData ) =0;
	virtual size_t Size() = 0;
	virtual void DeleteFileFragmentData() = 0;
	
	virtual void AddData(OutputFile & aFile);
	virtual size_t GetOffset() { return iOffset; }
	virtual void SetOffset(size_t aOffset) { iOffset = aOffset; }
	
//protected:
	void SetFileFragmentData(FileFragmentData & aFileFragmentData, size_t aSize, char * aData){
		aFileFragmentData.SetSize(aSize);
		aFileFragmentData.SetData(aData);
	}		
	
protected:
	size_t iOffset;
};


/**
 * Represents a fragment of file of a given size (in bytes) at a given offset (in
 * bytes).
 */
class FileFragment
{

public:
	FileFragment(size_t aOffset, size_t aSize, FileFragmentOwner * aOwner) :
		iOffset(aOffset), iSize(aSize), iOwner(aOwner)
		{};
	virtual ~FileFragment();
	
	inline size_t GetOffset() const { return iOffset; };
	inline size_t GetSize() const { return iSize; };
	inline void SetOffset(size_t newVal) { iOffset = newVal; };
	inline void SetSize(size_t newVal) { iSize = newVal; };
	void Write(OutputFile * aFile);

private:
	/**
	 * offset in bytes at which the fragment occurs in the file
	 */
	size_t iOffset;
	/**
	 * size in bytes of the file fragment
	 */
	size_t iSize;
	/**
	 * the owner of the file fragment
	 */
	FileFragmentOwner * iOwner;
};

#endif /*FILEFRAGMENT_H_*/
