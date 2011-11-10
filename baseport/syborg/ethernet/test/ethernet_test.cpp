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

#include <e32test.h>
#include <e32debug.h>
#include <d32ethernet.h>

LOCAL_D RTest test(_L("DRIVER1_TEST"));

_LIT(KDriver1LddFileName,"enet");
_LIT(KDriver1PddFileName,"ethernet");

_LIT8(KTestSendData,"abcdefghijklmnopqrstuvwxyz");
_LIT8(KTestLargeSendData,"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");

GLDEF_C TInt E32Main()
    {
	test.Title();

	TInt r;
	RDebug::Printf(">>>>>>E32Main()");
	
	test.Start(_L("Load Physical Device"));
	r=User::LoadPhysicalDevice(KDriver1PddFileName);
	if (r != KErrNone)
		RDebug::Printf("LoadPhysicalDevice: value of error =%d", r);
		test(r==KErrNone || r==KErrAlreadyExists);

	test.Next(_L("Load Logical Device"));
	r=User::LoadLogicalDevice(KDriver1LddFileName);
	if (r != KErrNone)
			RDebug::Printf("LoadLogicalDevice: value of error =%d", r);
			test(r==KErrNone || r==KErrAlreadyExists);

	__KHEAP_MARK;

	test.Next(_L("Open Logical Channel"));
	RBusDevEthernet ldd;

	r=ldd.Open(0);
	RDebug::Printf("Value returned from ldd.Open()=%d", r);
	test(r==KErrNone);

	test.Next(_L("SendData"));
	TRequestStatus status;
	ldd.Write(status,KTestSendData);
	
	test.Next(_L("SendDataCancel"));
	ldd.WriteCancel();
	User::WaitForRequest(status);
	r=status.Int();
	//test(r==KErrCancel);

	test.Next(_L("SendData"));
	ldd.Write(status,KTestSendData);
	User::WaitForRequest(status);
	r=status.Int();
	//test(r==KErrNone);

	test.Next(_L("ReceiveData"));
	TBuf8<256> buffer;
	ldd.Read(status,buffer);

	test.Next(_L("ReceiveDataCancel"));
	ldd.ReadCancel();
	User::WaitForRequest(status);
	r=status.Int();
	test(r==KErrCancel);

	test.Next(_L("Close Logical Channel"));
	ldd.Close();

	__KHEAP_MARKEND;

	test.End();

	return(0);

  }


