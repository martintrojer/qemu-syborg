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

#ifdef __cplusplus
extern "C" {
#endif
		
char * strchr (const char *p, int ch)
	{
	char c;

	c = ch;
	for (;; ++p) 
		{
		if (*p == c)
			return ((char *)p);
		if (*p == '\0')
			return (char *)(0);
		}
	/* NOTREACHED */
	}

unsigned int strlen(const char *str)
	{
	const char *s;
	for (s = str; *s; ++s)	{	}

	return(s - str);
	}

int strcmp(const char *s1, const char *s2)
	{
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return (0);
	return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
	}

void * memchr(const void *s, int c, unsigned n)
	{
	if (n != 0) 
		{
		const unsigned char *p = s;
		do 
			{
			if (*p++ == (unsigned char)c)
				return ((void *)(p - 1));
			} while (--n != 0);
	
		}
	// Not found
	return (void *)0;
	}

int memcmp(const void *s1, const void *s2, unsigned n)
	{
	if (n != 0) 
		{
		const unsigned char *p1 = s1, *p2 = s2;

		do 
			{
			if (*p1++ != *p2++)
				return (*--p1 - *--p2);
			} while (--n != 0);
		}
	return (0);
	}

#ifdef __cplusplus
}
#endif
