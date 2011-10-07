#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>

#include <memory.h>
#include <math.h>

#include "CircFBuf.hpp"


int _tmain(int argc, _TCHAR* argv[])
{
	(void)argc;
	(void)argv;

	(void)_tprintf_s(_T("Circular Buffer Test\n"));



	CIRCULAR_FLOAT_BUFFER cfb = NULL;

	float farr[] = {1.0f,2.0f,3.0f,4.0f,5.0f,6.0f,7.0f,8.0f,9.0f,10.0f};

	cfb = CFB_create(5);

	if(cfb == NULL)
	{
		(void)_tprintf_s(_T("CFB failed creation\n"));
		return 0;
	}

	(void)_tprintf_s(_T("Created.\n"));

	(void)CFB_debugPrint(cfb);

	CFB_write(cfb, (float*)farr, 3);

	(void)_tprintf_s(_T("Wrote 3.\n"));

	(void)CFB_debugPrint(cfb);

	CFB_write(cfb, (float*)farr, 8);


	(void)_tprintf_s(_T("Wrote 8 more.\n"));

	(void)CFB_debugPrint(cfb);

	CFB_write(cfb, (float*)farr, 10);

	(void)_tprintf_s(_T("Wrote 10 more (overlap test).\n"));

	(void)CFB_debugPrint(cfb);


	(void)_tprintf_s(_T("Press enter to exit.\n"));

	(void)getc(stdin);


	(void)CFB_delete(cfb);

	return 0;
}