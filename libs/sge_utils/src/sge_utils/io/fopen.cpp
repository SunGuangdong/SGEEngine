#include "sge_utils/io/fopen.h"
#include "sge_utils/sge_utils.h"
#include <string>

int sge_fopen(FILE** ppFile, const char* filename, const char* mode)
{
	sgeAssert(ppFile != nullptr);
#ifdef _MSC_VER
	return fopen_s(ppFile, filename, mode);
#else
	*ppFile = fopen(filename, mode);
	if (*ppFile == nullptr) {
		// sgeLogError("fopen failed on '%s' with mode '%s'\n", filename, mode);
		return 1;
	}
	return 0;
#endif
}

void sge_fclose_safe(FILE* pFile)
{
	if (pFile) {
		fclose(pFile);
	}
}
