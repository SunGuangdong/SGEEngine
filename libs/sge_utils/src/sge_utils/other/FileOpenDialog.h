#pragma once

#include "sge_utils/sge_utils.h"
#include <string>

namespace sge {

void DialongOk(const char* caption, const char* message);
bool DialogYesNo(const char* caption, const char* message);

std::string FileOpenDialog(const std::string& prompt, bool fileMustExists, const char* fileFilter, const char* initialDir);
std::string FileSaveDialog(const std::string& prompt, const char* fileFilter, const char* defaultExtension, const char* initialDir);
std::string FolderOpenDialog(const char* const prompt, const std::string& initialPath);

} // namespace sge
