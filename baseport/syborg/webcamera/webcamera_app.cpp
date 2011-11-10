/*
* Copyright (c) 2010 ISB.
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* ISB - initial contribution.
*
* Contributors:
*
* Description: USB driver for test
*
*/

#include <e32test.h>
#include <webcamera_driver.h>

#include <e32debug.h>
#define DP(format...) RDebug::Printf(format)

LOCAL_D RTest test(_L("WebcameraDevice_TEST"));

//Wins用ダミーなので実機では正式なものを使用する
_LIT(KWebcameraPddFileName, "webcamera.pdd");
_LIT(KWebcameraLddFileName, "ewebcamera.ldd");

GLDEF_C TInt E32Main()
 {
    test.Title();
    TInt r;
    
    test.Start(_L("Load Physical Device"));
    r=User::LoadPhysicalDevice(KWebcameraPddFileName);
    test(r==KErrNone || r==KErrAlreadyExists);

    test.Next(_L("Load Logical Device"));
    r=User::LoadLogicalDevice(KWebcameraLddFileName);
    test(r==KErrNone || r==KErrAlreadyExists);
//    __KHEAP_MARK;

//    test.Next(_L("Open Device"));
//    RDevice device;
//    r=device.Open(RWebcameraDevice::Name());
//    test(r==KErrNone);

    //test.Next(_L("Close Device"));
    //device.Close();

    test.Next(_L("Open Logical Channel"));
    RWebcameraDevice ldd;
    r=ldd.Open();
    test(r==KErrNone);

    test.Next(_L("Check access by wrong client"));
    RWebcameraDevice ldd2=ldd;
    r=ldd2.Duplicate(RThread(),EOwnerProcess);
    test(r==KErrAccessDenied);

    test.Next(_L("Check handle duplication"));
    ldd2=ldd;
    r=ldd2.Duplicate(RThread(),EOwnerThread);
    test(r==KErrNone);
    ldd2.Close();

    test.Next(_L("ReceiveData"));
    TRequestStatus status;
    HBufC8 * buffer = HBufC8::NewL(BUFSIZE);
    TPtr8	itempPtr(buffer->Des());
    itempPtr.SetLength(0);
    ldd.StartViewFinder(status,itempPtr);

    test.Next(_L("ReceiveDataCancel"));
    ldd.StopViewFinder();
    User::WaitForRequest(status);
    r=status.Int();
    test(r==KErrNone);
	
    itempPtr.SetLength(0);
    ldd.StartViewFinder(status,itempPtr);
    User::WaitForRequest(status);
    r=status.Int();
	test(r==KErrNone);
	
    test.Next(_L("CaptureData"));
    HBufC8 * buffer1 = buffer;
    TPtr8	itempPtr1(buffer1->Des());
    itempPtr1.SetLength(0);
    ldd.Capture(status,itempPtr1);
    User::WaitForRequest(status);
    r=status.Int();
    test(r==KErrNone);
    
    test.Next(_L("Close Logical Channel"));
    ldd.Close();

//    __KHEAP_MARKEND;

    test.Next(_L("Unload Logical Device"));
    r=User::FreeLogicalDevice(RWebcameraDevice::Name());
    test(r==KErrNone);

    test.Next(_L("Unload Physical Device"));
    TName pddName(RWebcameraDevice::Name());
    _LIT(KVariantExtension,".pdd");
    pddName.Append(KVariantExtension);
    r=User::FreePhysicalDevice(pddName);
    test(r==KErrNone);
	
    test.End();

    return(0);
 }
