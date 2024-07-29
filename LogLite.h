#pragma once
#include <cstdio>
#include <cstdarg>

FILE* file;
const bool EnableLog = false;

void Open()
{
	if constexpr (EnableLog)
	{
		fopen_s(&file, "SyringeEx.log", "w"); 
	}
}

void WriteLine(char const* const pFormat, ...) noexcept
{
	if constexpr (EnableLog)
	{
		if (file) {
			va_list args;
			va_start(args, pFormat);

			vfprintf(file, pFormat, args);
			fputs("\n", file);
			fflush(file);

			va_end(args);
		}
	}
	else
	{
		(void)pFormat;
	}
}

void Close()
{
	if constexpr (EnableLog)
	{
		if (file)fclose(file);
	}
}