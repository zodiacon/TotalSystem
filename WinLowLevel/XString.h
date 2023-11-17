#pragma once

#include <string>

template<typename T>
class XBasicString : public std::basic_string<T> {
public:
	using bs = std::basic_string<T>;
	using bs::basic_string;
	XBasicString Left(size_t count) const {
		return bs::substr(0, count);
	}

	XBasicString Right(size_t count) const {
		return substr(bs::length() - count, count);
	}
};

using XString = XBasicString<char>;
using XWString = XBasicString<wchar_t>;
