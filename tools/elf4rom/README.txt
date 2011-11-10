The ELF4ROM command line tool and libraries. This tool is used to convert Symbian ROM images
into debuggable elf files. See docs/wiki/ELF4ROM.doc for more information.

ELF4ROM depends on the following libraries;

* boost/program_options
* boost/regex
* boost/filesystem
* libelf
* libdwarf

Supported libelf and libdwarf can be found in the libs/ folder
Boost can be downloaded and built from http://www.boost.org/

On windows please use mingw or cygwin to build.
