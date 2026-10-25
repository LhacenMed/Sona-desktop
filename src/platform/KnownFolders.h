#pragma once

#include <string>
#include <windows.h>
#include <ShlObj.h>

// The path of a user folder such as FOLDERID_Videos or FOLDERID_Pictures.
std::wstring knownFolderPath(const KNOWNFOLDERID &folder);
