#ifndef APP_VERSION_H
#define APP_VERSION_H

#include <System.SysUtils.hpp>

namespace AppVersion
{
	const int Major = 4;
	const int Minor = 3;
	const int Patch = 34;

	inline UnicodeString Text()
	{
		return IntToStr(Major) + L"." + IntToStr(Minor) + L"." + IntToStr(Patch);
	}

	inline UnicodeString TitleTemplate()
	{
		return L"SearchClient_asio v{version}";
	}

	inline UnicodeString AppTitle()
	{
		return StringReplace(
			TitleTemplate(),
			L"{version}",
			Text(),
			TReplaceFlags() << rfReplaceAll);
	}
}

#endif