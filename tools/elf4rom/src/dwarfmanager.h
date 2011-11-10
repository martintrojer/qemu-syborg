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

#ifndef DWARFMANAGER_H_
#define DWARFMANAGER_H_

#include <vector>
#include <map>
#include <string>
#include <libelf.h>

#include "dwarfdefs.h"
#include "dwarf.h"
#include "defs.h"
#include "filefragment.h"
#include "romdetails.h"
#include "elfsectionmanager.h"

using namespace std;

class FileShdrPair {
public:
	FileShdrPair(XIPFileDetails & aXIPFileDetails, Elf32_Shdr * aShdr):
		iXIPFileDetails(aXIPFileDetails),
		iShdr(*aShdr)
	{}
	FileShdrPair & FileShdrPair::operator=(const FileShdrPair & aFileShdrPair) {
		iXIPFileDetails = aFileShdrPair.iXIPFileDetails;
		iShdr = aFileShdrPair.iShdr;
		return *this;
	}

	FileShdrPair::FileShdrPair(const FileShdrPair &aFileShdrPair):
		iXIPFileDetails(aFileShdrPair.iXIPFileDetails),
		iShdr(aFileShdrPair.iShdr)
	{}
	
	XIPFileDetails & iXIPFileDetails;
	Elf32_Shdr iShdr;
};

typedef std::vector<FileShdrPair> FileShdrList;

void EditLocationExpression (Dwarf_Byte_Ptr data, 
							 unsigned int pointer_size, 
							 unsigned long length, 
							 FileShdrPair & aPair);
class DwarfManager;

class DwarfSectionManager : public FileFragmentOwner {
public:
	DwarfSectionManager(ElfSectionManager & aElfSectionManager, 
						DwarfManager & aDwarfManager, 
						const string & aName, 
						RomDetails * aRomDetails):
		iElfSectionManager(aElfSectionManager),
		iDwarfManager(aDwarfManager),
		iSectionName(aName),
		iSizeValid(false),
		iSize(0),
		iData(NULL),
		iRomDetails(aRomDetails)
	{}
	
	virtual ~DwarfSectionManager() {
		iFileShdrList.clear();
	}
	
	// The FileFragmentOwner protocol
	virtual void DeleteFileFragmentData(){
		if (iData) {
			Dwarf_Byte_Ptr d = iData;
			iData = NULL;
			delete [] d;
		}
	}

	virtual void AddSection(XIPFileDetails & aXIPFileDetails, Elf32_Shdr * aShdr);
	virtual void SetupSection();
	const string & GetSectionName() { return iSectionName; }
	virtual Dwarf_Byte_Ptr GetSectionData(FileShdrPair & aPair);
		

	
	// LEB decoding
	// Leb128 decoding used by all Dwarf section managers (potentially)
	// TODO could get rid of LEB macros and define as inline functions.
#define DECODE_ULEB128(v,p,n) \
		Dwarf_Word v = DwarfSectionManager::DecodeUnsignedLeb128(p, n);\
		p += n;
#define ULEB128(p,n) \
		DwarfSectionManager::DecodeUnsignedLeb128(p, n);\
		p += n;
	static Dwarf_Unsigned DecodeUnsignedLeb128(Dwarf_Byte_Ptr leb128, size_t & leb128_length);
#define DECODE_SLEB128(v,p,n) \
		Dwarf_Word v = DwarfSectionManager::DecodeSignedLeb128(p, n);\
		p += n;
#define SLEB128(p,n) \
		DwarfSectionManager::DecodeSignedLeb128(p, n);\
		p += n;
	static Dwarf_Signed DecodeSignedLeb128(Dwarf_Byte_Ptr leb128, size_t & leb128_length);
	
private:
	// Don't want one of these to be copied
	DwarfSectionManager(const DwarfSectionManager & aDwarfSectionManager);
	
	DwarfSectionManager & operator=(const DwarfSectionManager & aDwarfSectionManager);
protected:

	virtual Dwarf_Byte_Ptr GetData(){ return iData; }
	virtual void SetData(Dwarf_Byte_Ptr data) { iData = data; }
	
	size_t CheckNewOffset(size_t base, size_t offset){
		const Dwarf_Off limit = 0xfffffff0ul;
		Dwarf_Off newOffset = base + offset;
		if (newOffset >= limit) {
			cerr << "Error: cannot support transition from 32 to 64 bit offsets\n";
			exit(EXIT_FAILURE);
		}
		return (size_t)newOffset;
	}
	
protected:
	ElfSectionManager & iElfSectionManager;
	DwarfManager & iDwarfManager;
	FileShdrList iFileShdrList;
	const string iSectionName;
	bool iSizeValid;
	size_t iSize;
	Dwarf_Byte_Ptr iData;
	RomDetails * iRomDetails;
};

class DwarfConcatenatedSectionManager : public DwarfSectionManager {
public:
	DwarfConcatenatedSectionManager(ElfSectionManager & aElfSectionManager, 
									DwarfManager & aDwarfManager, 
									const string & aName, 
									RomDetails * aRomDetails):
		DwarfSectionManager(aElfSectionManager, 
							aDwarfManager, 
							aName, 
							aRomDetails)
	{}
	
	// The FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData );
	virtual size_t Size();
	
	virtual void ConcatenateData();
	virtual void ProcessSection(FileShdrPair & aPair);
	virtual void ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr start, Dwarf_Byte_Ptr end){};
	
	// Concatenated section protocol
	virtual void SetSectionOffset(PathName & aPathName, size_t aOffset);
	virtual void InitOffsetMap();
	virtual size_t GetSectionOffset(PathName & aPathName);
	virtual void SetSectionSize(PathName & aPathName, size_t aSize);
	virtual size_t GetSectionSize(PathName & aPathName);
	
	Dwarf_Byte_Ptr GetSection(PathName & aPathName);
	
protected:
	typedef std::map<PathName, size_t> PathNameSectionOffsetMap;
	PathNameSectionOffsetMap iPathNameSectionOffsetMap;
	PathNameSectionOffsetMap iPathNameSectionSizeMap;


};

class DwarfFragmentedSectionManager : public DwarfConcatenatedSectionManager {
public:
	DwarfFragmentedSectionManager(ElfSectionManager & aElfSectionManager, 
									DwarfManager & aDwarfManager, 
									const string & aName, 
									RomDetails * aRomDetails):
		DwarfConcatenatedSectionManager(aElfSectionManager, 
										aDwarfManager, 
										aName, 
										aRomDetails),
		iInitialTraceMessage(true)
	{}
	
	// Override the method of setting up the section
	virtual void SetupSection();

	// The FileFragmentOwner protocol
	virtual void GetFileFragmentData(FileFragmentData & aFileFragmentData );
	//virtual size_t Size();
	
	virtual void ConcatenateData();
	
	void ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr & aData);
	virtual void ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr start, Dwarf_Byte_Ptr end) = 0;
	
private:
	bool iInitialTraceMessage;

};

class DebugAbbrevAttrForm {
public:
	DebugAbbrevAttrForm():
			iAttr(0),
			iForm(0)
		{}
	DebugAbbrevAttrForm(Dwarf_Half a, Dwarf_Half f):
		iAttr(a),
		iForm(f)
	{}
	Dwarf_Half iAttr;
	Dwarf_Half iForm;
};

class DebugAbbrev {
public:
	DebugAbbrev():
		iCode(0),
		iTag(0),
		iCount(0),
		iRaw(NULL),
		iParsed(NULL)
		{}
	DebugAbbrev(Dwarf_Unsigned c, Dwarf_Unsigned t, size_t n, Dwarf_Byte_Ptr r, DebugAbbrevAttrForm * p):
			iCode(c),
			iTag(t),
			iCount(n),
			iRaw(r),
			iParsed(p)
			{}
	DebugAbbrev & operator=(const DebugAbbrev & aDebugAbbrev){
		iCode = aDebugAbbrev.iCode;
		iTag = aDebugAbbrev.iTag;
		iCount = aDebugAbbrev.iCount;
		iRaw = aDebugAbbrev.iRaw;
		iParsed = aDebugAbbrev.iParsed;
		return *this;
	}
	
	DebugAbbrev(const DebugAbbrev & aDebugAbbrev){
		*this = aDebugAbbrev;
	}
	
	void Destroy(){
		if (iParsed){
			DebugAbbrevAttrForm * d = iParsed;
			iParsed = NULL;
			delete [] d;
		}
	}
#if 0
	// can't have default dtor do anything until and unless we prevent iParsed getting deleted
	// whenever the class is copied in STL containers.
	~DebugAbbrev(){
		if (iParsed){
			DebugAbbrevAttrForm * d = iParsed;
			iParsed = NULL;
			delete [] d;
		}
	}
#endif
	Dwarf_Unsigned iCode;
	Dwarf_Unsigned iTag;
	size_t iCount;
	Dwarf_Byte_Ptr iRaw;
	DebugAbbrevAttrForm * iParsed;
};

class InfoContext {
private:
	typedef std::map<Dwarf_Word, DebugAbbrev> AbbrevMap;
	class AbbrevMapEntry {
	public:
		AbbrevMapEntry():
					iCursor(NULL),
					iMap(NULL)
				{}
		AbbrevMapEntry(Dwarf_Byte_Ptr c, AbbrevMap * m):
			iCursor(c),
			iMap(m)
		{}
		Dwarf_Byte_Ptr iCursor;
		AbbrevMap * iMap;
	};
	typedef std::map<Uint32, AbbrevMapEntry> AbbrevOffsetMap;

public:
	InfoContext():
		iContextValid(false),
		iSectionStart(NULL),
		iSectionEnd(NULL),
		iSectionOffset(0),
		// this is an invalid offset for 32 bit dwarf
		// and will trigger the right behaviour in SetAbbrevOffset
		// when called from Init
		iAbbrevOffset(0xffffffff), 
		
		iAbbrevMapEntry(NULL, NULL)
	{
		//ClearMap();
	}
	~InfoContext(){
		Reset();
	}
	void Init(Dwarf_Byte_Ptr s, Dwarf_Byte_Ptr e, size_t o){
		iSectionStart = s;
		iSectionEnd = e;
		iSectionOffset = o;
		//ClearMap();
		iContextValid = true;
		SetAbbrevOffset(0);
	}
	void Reset(){
		iContextValid = false;
		iSectionStart = iSectionEnd = NULL;
		iSectionOffset = 0;
		iAbbrevOffset = 0xffffffff;
		ClearMap();
	}
	void ClearMap() {
		for (AbbrevOffsetMap::iterator i = iMap.begin(); i != iMap.end(); i++){
			AbbrevMap::iterator e = i->second.iMap->end();
			for (AbbrevMap::iterator b = i->second.iMap->begin(); b != e; b++){
				b->second.Destroy();
			}
			i->second.iMap->clear();
		}
		iMap.clear();
	}
	size_t GetSectionOffset(){ return iSectionOffset; }
	
	void SetAbbrevOffset(size_t offset);
	
	DebugAbbrev & GetAbbrev(Dwarf_Word aCode);
	DebugAbbrev & FindAbbrev(Dwarf_Word aCode);

	bool iContextValid;
	Dwarf_Byte_Ptr iSectionStart;
	Dwarf_Byte_Ptr iSectionEnd;
	size_t iSectionOffset;
	size_t iAbbrevOffset;
	AbbrevMapEntry iAbbrevMapEntry;
	AbbrevOffsetMap iMap;
};

class DwarfAbbrevManager : public DwarfConcatenatedSectionManager {
public:
	DwarfAbbrevManager(ElfSectionManager & aElfSectionManager, 
					   DwarfManager & aDwarfManager, 
					   RomDetails * aRomDetails):
		DwarfConcatenatedSectionManager(aElfSectionManager, 
										aDwarfManager, 
										iAbbrevSectionName, 
										aRomDetails)
	{}
	
	// we might need to hang onto this after its been written to file
	virtual void DeleteFileFragmentData(){}

	void StartContext(PathName & aName);
	void EndContext();
	void SetContextAbbrevOffset(Uint32 offset);
	size_t GetContextSectionOffset();
	
	DebugAbbrev & GetAbbrev(Dwarf_Word aCode);
private:
	static const string iAbbrevSectionName;
	InfoContext iInfoContext;

};

class DwarfMacinfoManager : public DwarfFragmentedSectionManager {
public:
	DwarfMacinfoManager(ElfSectionManager & aElfSectionManager, 
						DwarfManager & aDwarfManager, 
						RomDetails * aRomDetails):
		DwarfFragmentedSectionManager(aElfSectionManager, 
									  aDwarfManager, 
									  iMacinfoSectionName, 
									  aRomDetails)
	{}
	virtual void ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr start, Dwarf_Byte_Ptr end){};

private:
	
	static const string iMacinfoSectionName;	
};

//class DwarfInfoManager : public DwarfConcatenatedSectionManager {
class DwarfInfoManager : public DwarfFragmentedSectionManager {
public:
	DwarfInfoManager(ElfSectionManager & aElfSectionManager, 
					 DwarfManager & aDwarfManager, 
					 DwarfAbbrevManager & aDwarfAbbrevManager, 
					 DwarfMacinfoManager & aDwarfMacinfoManager, 
					 RomDetails * aRomDetails):
		DwarfFragmentedSectionManager(aElfSectionManager, 
										aDwarfManager, 
										iInfoSectionName, 
										aRomDetails),
		iDwarfAbbrevManager(aDwarfAbbrevManager),
		iDwarfMacinfoManager(aDwarfMacinfoManager),
		iAddressSize(4),
		iLocalLength(0)
	{}
	
	virtual void ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr start, Dwarf_Byte_Ptr end);
	
	size_t GetPointerSize() { return iAddressSize; }
	
private:
	Dwarf_Byte_Ptr ProcessCU(FileShdrPair & aPair, Dwarf_Byte_Ptr s, Dwarf_Byte_Ptr e);
	Dwarf_Byte_Ptr ProcessDIE(FileShdrPair & aPair, Dwarf_Byte_Ptr s, Dwarf_Byte_Ptr e);
	
	typedef Dwarf_Byte_Ptr (*InfoEditFn)(DwarfInfoManager&, Dwarf_Byte_Ptr, Dwarf_Half,FileShdrPair & aPair);
	
#define DECLARE_INFO_EDIT_FN(name)static Dwarf_Byte_Ptr name(DwarfInfoManager& aManager, \
													Dwarf_Byte_Ptr aPtr, \
													Dwarf_Half aForm, \
													FileShdrPair & aPair)
	DECLARE_INFO_EDIT_FN(DefaultInfoEditFn);
	DECLARE_INFO_EDIT_FN(ErrorInfoEditFn);
	DECLARE_INFO_EDIT_FN(InfoEditAddress);
	DECLARE_INFO_EDIT_FN(InfoEditLinePtr);
	DECLARE_INFO_EDIT_FN(InfoEditLocListPtr);
	DECLARE_INFO_EDIT_FN(InfoEditLocExpr);
	DECLARE_INFO_EDIT_FN(InfoEditMacInfoPtr);
	DECLARE_INFO_EDIT_FN(InfoEditRangeListPtr);
	DECLARE_INFO_EDIT_FN(InfoEditString);
	DECLARE_INFO_EDIT_FN(InfoEditReference);
	DECLARE_INFO_EDIT_FN(InfoEditTrampoline);
	
	size_t SizeOfDieValue(Dwarf_Half aForm, Dwarf_Byte_Ptr aPtr);	

private:
	static const string iInfoSectionName;
	DwarfAbbrevManager & iDwarfAbbrevManager;
	DwarfMacinfoManager & iDwarfMacinfoManager;
	static InfoEditFn iInfoEditFn[];
	
	size_t iAddressSize;
	size_t iLocalLength;
};

class DwarfFrameManager : public DwarfFragmentedSectionManager {
public:
	DwarfFrameManager(ElfSectionManager & aElfSectionManager, 
					  DwarfManager & aDwarfManager, 
					  RomDetails * aRomDetails):
		DwarfFragmentedSectionManager(aElfSectionManager, 
									  aDwarfManager, 
									  iFrameSectionName, 
									  aRomDetails)
	{}

	virtual void ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr start, Dwarf_Byte_Ptr end);

private:
	typedef std::map<Dwarf_Byte_Ptr, Dwarf_Ubyte> CiePtrEncodingMap;
	typedef std::map<Dwarf_Byte_Ptr, size_t> CieAugmentationMap;

	static const string iFrameSectionName;	
};

class DwarfLineManager : public DwarfFragmentedSectionManager {
public:
	DwarfLineManager(ElfSectionManager & aElfSectionManager, 
					 DwarfManager & aDwarfManager, 
					 DwarfInfoManager & aDwarfInfoManager, 
					 RomDetails * aRomDetails):
		DwarfFragmentedSectionManager(aElfSectionManager, 
									  aDwarfManager, 
									  iLineSectionName, 
									  aRomDetails),
		iDwarfInfoManager(aDwarfInfoManager)
	{}
	
	virtual void ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr start, Dwarf_Byte_Ptr end);
	
private:
	DwarfInfoManager & iDwarfInfoManager;

	static const string iLineSectionName;	
};

class DwarfLocManager : public DwarfFragmentedSectionManager {
public:
	DwarfLocManager(ElfSectionManager & aElfSectionManager, 
					DwarfManager & aDwarfManager, 
					DwarfInfoManager & aDwarfInfoManager, 
					RomDetails * aRomDetails):
		DwarfFragmentedSectionManager(aElfSectionManager, 
									  aDwarfManager, 
									  iLocSectionName,
									  aRomDetails),
		iDwarfInfoManager(aDwarfInfoManager)
	{}
	
	virtual void ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr start, Dwarf_Byte_Ptr end);

private:
	DwarfInfoManager & iDwarfInfoManager;
	
	static const string iLocSectionName;
};

class DwarfNameManager : public DwarfFragmentedSectionManager {
public:
	DwarfNameManager(ElfSectionManager & aElfSectionManager, 
					 DwarfManager & aDwarfManager, 
					 const string & aName, 
					 DwarfInfoManager & aDwarfInfoManager, 
					 RomDetails * aRomDetails):
		DwarfFragmentedSectionManager(aElfSectionManager, 
									  aDwarfManager, 
									  aName, 
									  aRomDetails),
		iDwarfInfoManager(aDwarfInfoManager)
	{}
	
	virtual void ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr start, Dwarf_Byte_Ptr end);

protected:
	DwarfInfoManager & iDwarfInfoManager;
};

class DwarfPubnamesManager : public DwarfNameManager {
public:
	DwarfPubnamesManager(ElfSectionManager & aElfSectionManager, 
						 DwarfManager & aDwarfManager, 
						 DwarfInfoManager & aDwarfInfoManager, 
						 RomDetails * aRomDetails):
		DwarfNameManager(aElfSectionManager, 
						 aDwarfManager, 
						 iPubnamesSectionName, 
						 aDwarfInfoManager, 
						 aRomDetails)
	{}
	
private:
	static const string iPubnamesSectionName;	
};

class DwarfPubtypesManager : public DwarfNameManager {
public:
	DwarfPubtypesManager(ElfSectionManager & aElfSectionManager, 
						 DwarfManager & aDwarfManager, 
						 DwarfInfoManager & aDwarfInfoManager, 
						 RomDetails * aRomDetails):
		DwarfNameManager(aElfSectionManager, 
						 aDwarfManager, 
						 iPubtypesSectionName, 
						 aDwarfInfoManager, 
						 aRomDetails)
	{}
	
private:
	static const string iPubtypesSectionName;	
};

class DwarfArangesManager : public DwarfFragmentedSectionManager {
public:
	DwarfArangesManager(ElfSectionManager & aElfSectionManager, 
						DwarfManager & aDwarfManager, 
						DwarfInfoManager & aDwarfInfoManager, 
						RomDetails * aRomDetails):
		DwarfFragmentedSectionManager(aElfSectionManager, 
									  aDwarfManager, 
									  iArangesSectionName, 
									  aRomDetails),
		iDwarfInfoManager(aDwarfInfoManager)
	{}
	
	virtual void ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr start, Dwarf_Byte_Ptr end);

private:
	DwarfInfoManager & iDwarfInfoManager;

	static const string iArangesSectionName;	
};


class DwarfRangesManager : public DwarfFragmentedSectionManager {
public:
	DwarfRangesManager(ElfSectionManager & aElfSectionManager, 
					   DwarfManager & aDwarfManager, 
					   DwarfInfoManager & aDwarfInfoManager, 
					   RomDetails * aRomDetails):
		DwarfFragmentedSectionManager(aElfSectionManager, 
									  aDwarfManager, 
									  iRangesSectionName, 
									  aRomDetails),
		iDwarfInfoManager(aDwarfInfoManager)
	{}

	virtual void ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr start, Dwarf_Byte_Ptr end);

private:
	DwarfInfoManager & iDwarfInfoManager;

	static const string iRangesSectionName;	
};

class DwarfStrManager : public DwarfFragmentedSectionManager {
public:
	DwarfStrManager(ElfSectionManager & aElfSectionManager, 
					DwarfManager & aDwarfManager, 
					RomDetails * aRomDetails):
		DwarfFragmentedSectionManager(aElfSectionManager, 
									  aDwarfManager, 
									  iStrSectionName, 
									  aRomDetails)
	{}
	
	virtual void ProcessSection(FileShdrPair & aPair, Dwarf_Byte_Ptr start, Dwarf_Byte_Ptr end){};

private:
	static const string iStrSectionName;	
};

class DwarfManager {
public:
	DwarfManager(ElfSectionManager & aElfSectionManager, 
				RomDetails * aRomDetails, 
				OutputFile & aOutputFile):
		iDwarfAbbrevManager(aElfSectionManager, *this, aRomDetails),
		iDwarfFrameManager(aElfSectionManager, *this, aRomDetails),
		iDwarfMacinfoManager(aElfSectionManager, *this, aRomDetails),
		iDwarfInfoManager(aElfSectionManager, *this, iDwarfAbbrevManager, iDwarfMacinfoManager, aRomDetails),
		iDwarfLineManager(aElfSectionManager, *this, iDwarfInfoManager, aRomDetails),
		iDwarfLocManager(aElfSectionManager, *this, iDwarfInfoManager, aRomDetails),
		iDwarfPubnamesManager(aElfSectionManager, *this, iDwarfInfoManager, aRomDetails),
		iDwarfPubtypesManager(aElfSectionManager, *this, iDwarfInfoManager, aRomDetails),
		iDwarfArangesManager(aElfSectionManager, *this,iDwarfInfoManager, aRomDetails),
		iDwarfRangesManager(aElfSectionManager, *this,iDwarfInfoManager, aRomDetails),
		iDwarfStrManager(aElfSectionManager, *this, aRomDetails),
		iRomDetails(aRomDetails),
		iOutputFile(aOutputFile)
	{}
	
	size_t GetLineSectionOffset(PathName & pname){
		return iDwarfLineManager.GetSectionOffset(pname);
	}
	size_t GetLocListSectionOffset(PathName & pname){
		return iDwarfLocManager.GetSectionOffset(pname);
	}
	size_t GetMacInfoSectionOffset(PathName & pname){
		return iDwarfMacinfoManager.GetSectionOffset(pname);
	}
	size_t GetRangesSectionOffset(PathName & pname){
		return iDwarfRangesManager.GetSectionOffset(pname);
	}
	size_t GetStrSectionOffset(PathName & pname){
		return iDwarfStrManager.GetSectionOffset(pname);
	}

	OutputFile & GetOutputFile() { return iOutputFile; }
	
	void AddSection(XIPFileDetails & aXIPFileDetails, string aSectionName, Elf32_Shdr * aShdr);
	void SetupSections();
	
private:
	// Don't want one of these to be copied
	DwarfManager(const DwarfManager & aDwarfManager);
	
	DwarfManager & operator=(const DwarfManager & aDwarfManager);
	
private:
	DwarfAbbrevManager iDwarfAbbrevManager;
	DwarfFrameManager iDwarfFrameManager;
	DwarfMacinfoManager iDwarfMacinfoManager;
	DwarfInfoManager iDwarfInfoManager;
	DwarfLineManager iDwarfLineManager;
	DwarfLocManager iDwarfLocManager;
	DwarfPubnamesManager iDwarfPubnamesManager;
	DwarfPubtypesManager iDwarfPubtypesManager;
	DwarfArangesManager iDwarfArangesManager;
	DwarfRangesManager iDwarfRangesManager;
	DwarfStrManager iDwarfStrManager;
	RomDetails * iRomDetails;
	OutputFile & iOutputFile;
};

#endif /*DWARFMANAGER_H_*/
