/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/

#include <e32std.h>
#include <f32fsys.h>

_LIT(KEStartPanicCatagory, "SVP_HOSTFS_SYSSTART");

enum TEStartPanic
    {
    ELitNoDM = 1,                           // No Domain Manager
    ELitDMInitFail,                         // Domain Manager init fail
    ELitHALFail,                            // HAL init fail
    ELitConnectFsFail1,                     // Connect fs 1 fail
    ELitInitLocalPwStoreFail,               // Init PwStore fail
    ELitLocaleInitialisationFail,           // Initialisation of locale properties failed
    ELitFSInitDriveInfoFail,                // FS Init DriveInfo fail
    ELitCreateTrapHandlerFail,              // Create trap handler fail
    ELitLoadSysLddsFail,                    // Load sys ldds fail
    ELitLocalDriveMappingFail,              // Local drive mapping fail
    ELitDriveMappingFileFail,               // Drive mapping file not found
    ELitSwapMappingFailArrayInconsistent,   // Swap mappings fail - array inconsistent
    ELitFsSwapMappingFail,                  // Swap mappings fail - Fs request failed
    EPropertyError,                         // RProperty return error
    ECompMountFsFail,                       // Failed comp fs mount
    EFsNameFail,                            // File system name on Z: failed
    ELitNoWS,                               // No WSERV
    EStartupModeFail,                       // Get startup mode failed
    ESysAgentFail,                          // Fail to launch system agent
    ESetSystemDriveFail                     // Fail to set System Drive
    };

inline void Panic(TEStartPanic aPanic, TInt aReason)
    {
    TBuf<10> panic(KEStartPanicCatagory);
    panic.AppendFormat(_L("_%d"), aPanic);
    User::Panic(panic, aReason);
    }

_LIT(KServerPathSysBin, "0:\\Sys\\Bin\\");
_LIT(KWindowServerRootName1,"EWSRV.EXE");	
_LIT(KHostFSMounter, "z:\\sys\\bin\\SVPHOSTFS.EXE");

TBool CreateServer(const TDriveList& aDrives, const TDesC& aRootName)
	{
	RProcess ws;
	TInt r=ws.Create(aRootName, KNullDesC);
	if (r!=KErrNone)
		{
		TFileName name;
		name = KServerPathSysBin();
		name+=aRootName;
		TInt i=EDriveZ;
		FOREVER
			{
			i= (i==0) ? EDriveZ : i-1;
			if (aDrives[i]!=KDriveAbsent) // Got a valid drive
				{
				name[0]=(TUint32)('A'+i); // Set the drive letter
				r=ws.Create(name,KNullDesC);
				if (r==KErrNone)
					break;
				}
			if (i==EDriveZ)
				return EFalse;
			}
		}
	ws.Resume();
	ws.Close();
	return ETrue;
	}

GLDEF_C TInt E32Main()
        {
	RProcess ws;
	TInt r=ws.Create(KHostFSMounter, KNullDesC);
	if (r == KErrNone)
	        {
		TRequestStatus stat;
		ws.Rendezvous(stat);
		ws.Resume();
		User::WaitForRequest(stat);		// wait for start or death
		ws.Close();
		}
	// Start the window server 
	RFs aFs;
	r = aFs.Connect();

	if (r != KErrNone)
	        Panic(ELitConnectFsFail1, r);
	TDriveList list;
	aFs.DriveList(list);
	aFs.Close();

	if (!CreateServer(list,KWindowServerRootName1))
	        Panic(ELitNoWS,KErrNotFound);

	return(KErrNone);
	}	
