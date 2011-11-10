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

#include <boost/program_options.hpp>
namespace po = boost::program_options;
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

#include <iostream>

#include <iterator>
#include <string>
#include <map>
#include <set>

#include <ctype.h>
#include <cstdlib>

using namespace std;

#include "defs.h"
#include "romdetails.h"
#include "inputfile.h"
#include "elfromerror.h"

static inline void downcase(std::string & s){
	for (std::string::iterator i = s.begin(); i != s.end(); i++)
		*i = tolower(*i);
}

static int required_option(const po::variables_map& vm, const char* option){
	if (vm.count(option) == 0 || vm[option].defaulted()) {
		cerr << "Error: option \'" << option << "\' required.\n";
		return 1;
	} 
	return 0;
}

static int either_or_required(const po::variables_map& vm, const char* option1, const char* option2){
	if ((vm.count(option1) == 0 || vm[option1].defaulted()) &&
		(vm.count(option2) == 0 || vm[option2].defaulted())) {
		cerr << "Error: either option \'" << option1 << "\' or option \'" << option2 <<"\' required.\n";
		return 1;
	} 
	return 0;
}

RomDetails * ProcessOptions(int ac, char* av[]) {
	RomDetails * details = new RomDetails;
	int errors = 0;
	
    try {

    	string phys_addr;
        po::options_description desc("  Command Line Only");
        desc.add_options()
            ("help,h", "produce help message")
            ("config-file,c", po::value<string>(), 	"pathname of config file "
            													  	"(overrides default of elf4rom.cfg and value "
            														"of ELF4ROM_CFG_FILE environment variable)")
        ;

        po::options_description config("  Command Line and Configuration File");
        config.add_options()
            ("board-name,b", po::value<string>(&details->iBoardName), 	"name of board targeted e.g. versatilepb")
            ("debug,d", po::value< vector<string> >(&details->iTargetFiles)->multitoken()->composing(), 
																		"collect ELF and DWARF data from the listed files")
			("drive,D", po::value<string>(&details->iDrive), 			"drive on which to find ELF files")
			("exclude,e", po::value< vector<string> >(&details->iExcludeFiles)->multitoken()->composing(), 
																		"exclude collection of ELF and DWARF data from the listed files")
            ("input,i", po::value<string>(&details->iRomFile), 			"pathname of ROM image")
            ("logfile,l", po::value<string>(&details->iLogFile), 		"pathname of ROMBUILD log file")
            ("no-dwarf,n", po::bool_switch(&details->iNoDwarf), 		"suppress generatation of DWARF in output"
																		" (prevents source level debugging but saves time and space)")
            ("output,o", po::value<string>(&details->iElfRomFile), 		"pathname of output file")
            // lexical_cast doesn't understand <LinearAddress> even though its just a typedef
            // for unsigned int
            ("physical-address,p", po::value<string>(&phys_addr), 		"physical address of ROM on device. Overrides board-name")
            ("search,s", po::bool_switch(&details->iSearch), 			"search for ELF files in build directory if .sym file not "
            															"found in release directory")
            ("strip,S", po::bool_switch(&details->iStrip), 				"suppress generation of symbol table and DWARF in output"
																		" (useful to produce a 'loadable' ELF image for e.g. a simulator)")
			("trace,t", po::bool_switch(&details->iTrace), 				"switch on trace")
			;
            
        po::options_description cmdline_options;
        cmdline_options.add(desc).add(config);

        po::options_description config_file_options;
        config_file_options.add(config);
        
        po::variables_map vm; 
        //po::store(po::parse_command_line(ac, av, desc), vm);
        po::store(po::command_line_parser(ac, av).options(cmdline_options).run(), vm);
        
        char * cfgFile = "elf4rom.cfg";
        if (vm.count("config-file")) {
        	char * xcfgFile = (char *)(vm["config-file"].as<string>().c_str());
        	fs::path cfgpath(xcfgFile);

        	if (fs::exists(cfgpath)) {
        		cfgFile = xcfgFile;
        	} else {
        		cerr << "Warning: specified config file " << xcfgFile << " not found: will not attempt to use default.\n";
        	}
        } else {
        	char * envCfgFile = getenv("ELF4ROM_CFG_FILE");
        	if (envCfgFile != NULL) 
        		cfgFile = envCfgFile;
        }
        
        ifstream ifs(cfgFile);
        store(parse_config_file(ifs, config_file_options), vm);

       	po::notify(vm); 
 
        if (vm.count("help")) {
        	cout << "elf4rom [option]";
            cout << cmdline_options << "\n";
            delete details;
            exit(EXIT_SUCCESS) ;
        }

        
        errors += required_option(vm, "input");
        errors += required_option(vm, "logfile");
        errors += required_option(vm, "output");
        
        errors += either_or_required(vm, "board-name", "physical-address");

        if (vm.count("physical-address")){
        	char *f;
        	const char * p = phys_addr.c_str();
        	details->iRomPhysAddr = strtoul(p ,&f , 16);
        	if (f == p){
        		cerr << "Error: invalid arg to --physical-address: " << phys_addr.c_str() << "\n";
        		exit(EXIT_FAILURE);
        	}
        } else if (vm.count("board-name")){
        	// TODO: figure out address frmo board name
        	cerr << "Error: --board-name option not implemented yet\n";
            exit(EXIT_FAILURE) ;
        }
        
        if (errors) {
        	cerr << "elf4rom [option]";
            cerr << cmdline_options << "\n";
            delete details;
            exit(EXIT_FAILURE) ;
        }
    }
    catch(exception& e) {
        cerr << "error: " << e.what() << "\n";
        exit(EXIT_FAILURE) ;
    }
    catch(...) {
        cerr << "Exception of unknown type!\n";
        exit(EXIT_FAILURE) ;
    }

    return details;
}


static bool VerifyLogFile(string::const_iterator & start, string::const_iterator & end, string::const_iterator & rest){
#if 0 
// Look for somthing like the following
ROMBUILD - Rom builder V2.08 (Build 593)
Copyright (c) 1996-2007 Symbian Software Ltd.
or
ROMBUILD - Rom builder V2.08 (Build 596)
Copyright (c) 1996-2009 Nokia Corporation.
#endif	

	const char * banner  = "ROMBUILD.*Rom builder.*Copyright.*";
	boost::regex e(banner);
	boost::match_results<string::const_iterator> what;
	if (boost::regex_search(start, end, what, e)){ 
		rest = what[0].second;
		return true;
	}
	return false;
}

static void OffenceWarning (string & offender){
	cerr << "Warning: The following section of the ROM Log appears corrupt:\n" << offender << "\n";	
}

static inline unsigned int ConvertToUnsignedLong(string & s, string & offender){
	char * endp;
	const char * ss = s.c_str();
	unsigned int res = strtoul(ss, &endp,16);
	if (endp == ss)
		OffenceWarning(offender);
	return res;
}

static bool ProcessXIPFile(string::const_iterator & start, string::const_iterator & end, 
		string::const_iterator & rest, RomDetails * details){
	const char * f  = 
		"Processing file (\\S+)\\s*" 		// 1
		"(\\[Primary\\]\\s*)?"				// 2
		"(\\[Secondary\\]\\s*)?"			// 3		
		"(ELF File:\\s*\\S+\\s*)?" 			// 4
		"(ELF MD5:\\s*\\S+\\s*)?"			// 5
		"Load Address:\\s+(\\S+)\\s*"		// 6
		"Size:\\s+\\S+\\s*"
		"Uids:\\s+\\S+\\s+\\S+\\s+\\S+\\s+\\S+\\s*"
		"Entry point:\\s+\\S+\\s*"
		"Code start addr:\\s+(\\S+)\\s*"	// 7
		"Data start addr:\\s+(\\S+)\\s*"	// 8
		"DataBssLinearBase:\\s+(\\S+)\\s*"	// 9
		"Text size:\\s+\\S+\\s*"
		"Code size:\\s+(\\S+)\\s*"			// 10
		"Data size:\\s+(\\S+)\\s*"			// 11
		"BssSize:\\s+(\\S+)\\s*"			// 12
		;
	boost::regex e(f);
	boost::match_results<string::const_iterator> what;
	if (boost::regex_search(start, end, what, e)){
		string filename(what[1].first, what[1].second);
		string offender(what[0].first, what[0].second);
		if (filename.size() == 0)
			OffenceWarning(offender);
		bool primary = what[2].matched;
		if (what[2].matched){
			primary = true;
		}
		
		const int kla = 6;
		string ls(what[kla].first, what[kla].second);
		LinearAddr load = ConvertToUnsignedLong(ls, offender);

		const int kta = kla + 1;
		string ts(what[kta].first, what[kta].second);
		LinearAddr text = ConvertToUnsignedLong(ts, offender);

		const int kda = kta + 1;
		string ds(what[kda].first, what[kda].second);
		LinearAddr data = ConvertToUnsignedLong(ds, offender);

		const int kva = kda + 1;
		string vds(what[kva].first, what[kva].second);
		VirtualAddr vdata = ConvertToUnsignedLong(vds, offender);

		const int kts = kva + 1;
		string tss(what[kts].first, what[kts].second);
		size_t textSize = ConvertToUnsignedLong(tss, offender);		

		const int kds = kts + 1;
		string dss(what[kds].first, what[kds].second);
		size_t fileDataSize = ConvertToUnsignedLong(dss, offender);

		const int kbs = kds + 1;
		string bsss(what[kbs].first, what[kbs].second);
		size_t bssSize = ConvertToUnsignedLong(bsss, offender);		
		VirtualAddr bss = vdata + fileDataSize;
		size_t memDataSize = fileDataSize + bssSize;
		
		string elffile("");
		details->iXIPFiles.push_back(XIPFileDetails(filename, 
													elffile, 
													load, 
													text, 
													textSize,
													vdata, 
													fileDataSize, 
													data, 
													bss, 
													memDataSize));
		if (primary)
			new(&details->iPrimary)XIPFileDetails(filename, 
												  elffile, 
												  load, 
												  text, 
												  textSize,
												  vdata, 
												  fileDataSize, 
												  data, 
												  bss, 
												  memDataSize);
		
		rest = what[0].second;
		return true;
	}
	rest = start;
	return false;

}

static void CheckXIPFiles(RomDetails * details, std::vector<string> & list){
	for (std::vector<string>::iterator i = list.begin(); i != list.end(); i++) {
		bool found = false;
		for (RomDetails::XIPFileList::iterator j = details->iXIPFiles.begin(); j != details->iXIPFiles.end(); j++){
			fs::path e32filePath(j->iE32File);
			String e32FileName(e32filePath.leaf());
			if ((*i) == e32FileName){
				found = true;
				break;
			}

		}
		if (!found){
			cerr << "WARNING: " << (*i) << " not found in ROM\n";
		}
	}
}

static void CheckDebugXIPFiles(RomDetails * details){
	CheckXIPFiles(details, details->iTargetFiles);
}

static void CheckExcludeXIPFile(RomDetails * details){
	CheckXIPFiles(details, details->iExcludeFiles);
}

static void ProcessXIPFiles(string::const_iterator & start, string::const_iterator & end, 
		string::const_iterator & rest, RomDetails * details){
	while (ProcessXIPFile(start,end,rest,details)){
		start = rest;
	}
	CheckDebugXIPFiles(details);
	CheckExcludeXIPFile(details);
}


static void ProcessRomDetails(string::const_iterator & start, string::const_iterator & end, 
								string::const_iterator & rest, RomDetails * details){
	const char * align  = "Linear base address:\\s*([\\S]+)$";
	boost::regex e(align);
	boost::match_results<string::const_iterator> what;
	if (boost::regex_search(start, end, what, e)){ 
		string offender(what[0].first, what[0].second);
		string lbas(what[1].first, what[1].second);
		details->iRomBaseLinearAddr = ConvertToUnsignedLong(lbas, offender);
	} else {
        cerr << "Error: " << details->iLogFile << " not a valid ROM log file. Could not find Linear base address." << "\n";
        exit(EXIT_FAILURE) ;		
	}
}

static fs::path FindBuildPath(RomDetails * details){
	const string epoc32("epoc32");
	const string builddir("build");
	const string epocRoot(getenv("EPOCROOT"));
	if (details->iDrive.size() > 0) {
		string drive(details->iDrive);
		if (drive.size() == 1){
			drive += ":";
		} else if (((drive.size() == 2) && (drive[drive.size()-1] != ':')) || (drive.size() > 2)){
	        cerr << "Error: Invalid drive specification: " << drive << "\n";
	        exit(EXIT_FAILURE) ;			
		}
		fs::path buildpath(drive);

		buildpath /= epocRoot;
		buildpath /= epoc32;
		buildpath /= builddir;
		return buildpath;
	}
	fs::path primary_path(details->iPrimary.iE32File);
	fs::path buildpath;
	for (fs::path::iterator i = primary_path.begin(); i != primary_path.end(); i++){
		string item(*i);
		downcase(item);
		buildpath /= item;
		if (item == epoc32) {
			buildpath /= builddir;
			break;
		}
	}
	return buildpath;
}

class PathCache {
public:
	typedef std::map<string, string> PathMap;

	PathCache(fs::path & apath):
		iBuildPath(apath)
		{
			if (fs::exists(apath)){
				fs::recursive_directory_iterator i(apath);
				iCurrent = i;
			}
		}
	bool FindPath(string & pattern, string & result);
	fs::path & GetBuildPath() { return iBuildPath; }
	
private:
	bool GetNextPath(string & path);
	bool Filtered(fs::path & path);

private:
	
	fs::path iBuildPath;
	fs::recursive_directory_iterator iCurrent;
	fs::recursive_directory_iterator iEnd;
	PathMap iPathMap;
	static const std::string filters;
};



bool PathCache::Filtered(fs::path & apath){
	std::string ext(fs::extension(apath));
	downcase(ext);
	if (ext.size() > 0){
		if (filters.find(ext) != std::string::npos)
			return true;
	} 
	return false;
}

bool PathCache::GetNextPath(string & path){
	 for (; iCurrent != iEnd; ++iCurrent ){
		 if (fs::is_directory(iCurrent->status()))
			 continue;
		 fs::path candidate(iCurrent->path());
		 if (!Filtered(candidate)) {
			 path = candidate.string();
			 ++iCurrent;
			 return true;
		 }
	 }
	 return false;
}

static void GetTarget(string & source, string & target){
	fs::path t1(source);
	fs::path::iterator s = t1.begin();
	fs::path::iterator start = t1.end();
	int n = 0;
	for (; n < 3 && s != start; n++, start--){}
	if (n < 3) {
		warnx(0, "%s does not have 3 elements\n", source.c_str());
	}

	fs::path q;
	for (; start != t1.end(); start++){
		q /= fs::path(*start);
	}
	target = q.string();
	downcase(target);
}

bool PathCache::FindPath(string & source, string & result){
	string ss;
	if (source.size() == 0) return false;
	GetTarget(source, ss);
	
	// now check the cache
	PathMap::iterator res = iPathMap.find(ss);
	if (res != iPathMap.end()){
		result = res->second;
		return true;
	}
	
	// otherwise iterate until we find a match
	string candidate;
	while (GetNextPath(candidate)){
		downcase(candidate);
		size_t n = candidate.rfind(ss);
		if (n != string::npos){
			size_t csize = candidate.size();
			size_t sssize = ss.size();
			if ((csize - sssize) == n){
				// put it in cache anyway just in case its the primary
				iPathMap[ss] = candidate;
				result = candidate;
				return true;
			}
		}
		string x;
		GetTarget(candidate, x);
		iPathMap[x] = candidate;
	}
	return false;
}

const std::string PathCache::filters(
		".cpp"
		".h"
		".mk"
		".o"
		".in"
		".via"
		".mbg"
		".rsg"
		".bat"
		".make"
		".def"
		".armv5"
	);

static bool FindSymFile(XIPFileDetails & detail, string & rootname){
	fs::path buildpath(rootname);
	fs::path e32path(detail.iE32File);
	buildpath /= e32path;
	// check for pre-RAPTOR .sym file
	fs::path elfpath = fs::change_extension(buildpath, ".sym");
	if (fs::exists(elfpath)) {
		detail.iElfFile = elfpath.string();
		return true;
	}
	// check for RAPTOR .sym file
	fs::path symPath(buildpath.string() + ".sym");
	if (fs::exists(symPath)) {
		detail.iElfFile = symPath.string();
		return true;
	}	
	return false;
}

static void FindElfFile(XIPFileDetails & detail, PathCache & cache, bool search){
	// see if there's a .sym file
	string root(cache.GetBuildPath().root_name());
	if (FindSymFile(detail, root)) return;
	if (!search) {
		cerr << "Warning: could not find ELF file for " << detail.iE32File << ".\n";
		return;
	}
	string s(detail.iE32File);
	string res;
	if (s.size() == 0) return;
	if (cache.GetBuildPath().string().empty()) return;
	
	if (cache.FindPath(s, res)){
		detail.iElfFile = res;
	} else
		cerr << "Warning: could not find ELF file for " << detail.iE32File << ".\n";
}


static void FindElfFiles(RomDetails * details){
	fs::path buildroot = FindBuildPath(details);
	PathCache cache(buildroot);
	bool search = details->iSearch;
	FindElfFile(details->iPrimary, cache, search);
	for(RomDetails::XIPFileList::iterator i = details->iXIPFiles.begin(); i != details->iXIPFiles.end(); i++){
		fs::path e32filePath(i->iE32File);
		String e32FileName(e32filePath.leaf());
		if (details->iTargetFiles.empty()) {
			bool find = true;
			for (std::vector<String>::iterator ef = details->iExcludeFiles.begin(); ef != details->iExcludeFiles.end(); ef++) {
				fs::path excludePath(*ef);
				String excludeFile(excludePath.leaf());
				if (e32FileName == excludeFile) {
					find = false;
					break;
				}
			}
			if (find)
				FindElfFile(*i, cache, search);
		} else {
			for (std::vector<String>::iterator tf = details->iTargetFiles.begin(); tf != details->iTargetFiles.end(); tf++) {
				fs::path targetPath(*tf);
				String targetFile(targetPath.leaf());				
				if (e32FileName == targetFile) {
					FindElfFile(*i, cache, search);
					break;
				}
			}
		}
	}
	
}

RomDetails * ProcessRomDetails(RomDetails * details){
	InputFile in(details->iLogFile);
	char * data = in.GetData();
	string logfile(data);
	string::const_iterator  start = logfile.begin();
	string::const_iterator  end = logfile.end();
	string::const_iterator  rest;

	if (!VerifyLogFile(start, end , rest)) {
        cerr << "error: " << details->iLogFile << " not a valid ROM log file." << "\n";
        exit(EXIT_FAILURE) ;
	}
	
	ProcessXIPFiles(start,end,rest,details);
	
	ProcessRomDetails(rest, end, rest, details);
	
	FindElfFiles(details);
	
	// TODO: make sure functions that return data allocated with new [] 
	// have an appropriate return value so that delete [] can be used
	delete [] data;
	
	return details;
}
