/*******************************************************************************
* Copyright (c) 2009 Accenture
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html
*
* Contributors:
* Accenture - Johnathan White

*******************************************************************************/

Submitter - Johnathan.White@accenture.com	

Purpose - Simple TextShell console application which reads image file from filesystem and outputs to display

Build from -  \sf\adaptation\qemu\applications\SymbianLogo_TextShell

Using command - sbs -b bld.inf -c armv5 -j 1

To include in textshell build modify \sf\os\kernelhwsrv\kernel\eka\rombuild\tshell.oby and add the line -

#include <rom\include\symbianlogo.iby>

Then Build textshell rom from \sf\os\kernelhwsrv\kernel\eka\rombuild\ using command - rom -v syborg -i armv5 -b udeb -noheader

Then run QEMU from \symbian-qemu-0.9.1\bin using command -

arm-none-symbianelf-qemu-system.exe -kernel \sf\os\kernelhwsrv\kernel\eka\rombuild\syborgarmv5d.img -M \sf\adaptation\qemu\baseport\syborg\syborg.dtb

