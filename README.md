# QEMU virtual board, Symbian Baseport (BSP) and tools

Copyright Martin Trojer <martin.trojer@gmail.com>

This is a dump of the Symbian Foundation repository of the "QEMU project".
The aim of the project was to create a viable emulated execution platform
for development and testing of symbian system and user applications.

The project was originally run by a small team (Martin Trojer, Andy Sizer)
from within Symbian Ltd and later transitioned to open-source domain
within the Symbian Foundation.

# Contents

* applications/
	Sample application 

* baseport/
	The baseport (BSP) needed to run Symbian OS in QEMU

* docs/
	Various documentation about the baseport, peripherals and how to use QEMU.
	The wiki folders contains a dump from Wiki documentation, thus the unfortunate .doc format.

* symbian-qemu-0.9.1-12/
	A version of QEMU with some extra features not yet available in the QEMU mainline.
	A corresponding set of Windows/Linux binaries is available from the Symbian Foundation wiki
        http://developer.symbian.org/wiki/index.php/SYBORG/QEMU

* tools/e32test-driver/
	A simple python script to run the E32test-suite and collect/summarize the results.

* tools/elf4rom/
	The ELF4ROM command line tool and libraries. This tool is used to convert Symbian ROM images
	into debuggable elf files. See docs/wiki/ELF4ROM.doc for more information.

# Notes

* The source layout for baseport now matches the Symbian Foundation layout, and 
  will compile correctly if this repository is unpacked to /sf/adaptation/qemu. Eventually
  the baseport could move into the kernelhwsrv or boardsupport packages, but that
  would imply changing AsspNKernIncludePath in baseport/syborg/variant.mmh
* Even though a version of QEMU is supplied, the TOT version of QEMU should be usable 
  (but you will have to build it yourself). The QEMU version is supplied here to 
  share some the extra features not yet available in mainline QEMU. Most noticeable 
  the support for Python peripherals (and binary separation of QEMU and its peripheral) 
  and the skinning support to supply a landing zone for phone specific buttons etc.
* Throughout the documentation the SVP is referenced. SVP stands for the 
  "Symbian Virtual Platform" and this was the proposed product name of the 
  QEMU based simulator. For the purpose of these sources and documents treat the SVP 
  as QEMU with the syborg board models.
