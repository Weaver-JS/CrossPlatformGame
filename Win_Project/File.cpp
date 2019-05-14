#ifndef FILE_CPP
#define FILE_CPP

#include "Framework.h"

namespace File {

	FullFile ReadFullFile(const wchar_t* path)
	{
		FullFile result;

		HANDLE hFile;
		OVERLAPPED ol = {};

		hFile = CreateFileW(
			path,               // file to open
			GENERIC_READ,          // open for reading
			FILE_SHARE_READ,       // share for reading
			NULL,                  // default security
			OPEN_EXISTING,         // existing file only
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // normal file
			NULL);                 // no attr. template

		if (hFile != INVALID_HANDLE_VALUE)
		{
			DWORD fileSizeHigh;
			result.size = GetFileSize(hFile, &fileSizeHigh);
			FILETIME lastWriteTime;
			GetFileTime(hFile, NULL, NULL, &lastWriteTime);
			ULARGE_INTEGER li;
			li.HighPart = lastWriteTime.dwHighDateTime;
			li.LowPart = lastWriteTime.dwLowDateTime;
			result.lastWriteTime = li.QuadPart;

			result.data = new uint8_t[result.size + 1];

			if (ReadFileEx(hFile, result.data, (DWORD)result.size, &ol, nullptr))
			{
				result.data[result.size] = 0; // set the last byte to 0 to help strings
			}
			else
			{
				result.data = nullptr;
				result.size = 0;
			}
			CloseHandle(hFile);
		}
		else
		{
			result.data = nullptr;
			result.size = 0;
		}
		return result;
	}
}

#endif // !FILE_CPP


