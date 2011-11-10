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
// SymbianLogo.cpp

#include <e32std.h>
#include <e32cons.h>
#include <f32file.h>
#include <hal.h>
#include <S32FILE.H> 

void SetupConsoleL();
void DisplayLogoL(CConsoleBase* aConsole);

CConsoleBase* console;

GLDEF_C TInt E32Main()									
    {
	CTrapCleanup* cleanup=CTrapCleanup::New();			
	TRAPD(error,SetupConsoleL());	
	if(error)
		RDebug::Printf("SymbianLogo SetupError %d", error);
	delete cleanup;										
	return 0;									
    }

void SetupConsoleL()									
    {
	console=Console::NewL(_L("SymbianLogo"),
	TSize(KConsFullScreen,KConsFullScreen));
	
	CleanupStack::PushL(console);
	TRAPD(error,DisplayLogoL(console));					
	if(error)
		RDebug::Printf("SymbianLogo DisplayLogo Error %d", error);
	CleanupStack::PopAndDestroy();						
    }


GLDEF_C void DisplayLogoL(CConsoleBase* aConsole)
    {
	TInt err =KErrNone;

	//Connect to FileServer
    RFs fs;
    err = fs.Connect();

	User::LeaveIfError(err);
    
	//Open file 
	
	/*
	File \sf\adaptation\qemu\baseport\syborg\syborg.dts contains board model description, the hostfs@0 block defines both the host path
	and target drive. The hostpath is by default set to \svphostfs\ and default drive number is 19 (S:\). If you would like to change this 
	edit the dts file and use arm-none-symbianelf-dtc.exe to create updated dtb file
	*/
    RFile file; 
    err = file.Open(fs, _L("S:\\symbian_logo.bmp"), EFileRead);
    
	User::LeaveIfError(err);

	//Use to read stream from file on HostFs, first 54 bytes are header so skip
	/*
	symbian_logo.bmp is a 480*480 bmp file
	*/
    RFileReadStream filestream(file, 54);
    
	
	//Obtain base address of framebuffer which will copy bitmap data into to display
    TInt iScreenAddress;
    HAL::Get(HAL::EDisplayMemoryAddress, iScreenAddress);
	TUint8* pointer = (TUint8*)iScreenAddress;
    



    pointer+=640*479*4; //bitmap is 480*480 where as display is 640 *640, start by incrementing display pointer to last line required
	for(TInt i=0;i<480;i++)
		{
		for(TInt j=0;j<1920;j++)
    		{
			*pointer = filestream.ReadUint8L(); //reads byte from file into correct offset in framebuffer
			pointer++;
    		}
		
		pointer-=1920; //decrement over offset between each line
		pointer-=640*4; //decrement to start of next line    
	}
    
    //Wait for User to press key then return
	aConsole->Getch();
	return;
	}
