#pragma once

#include <cstdio>

int sge_fopen(FILE** ppFile, const char* filename, const char* mode);
void sge_fclose_safe(FILE* pFile);

