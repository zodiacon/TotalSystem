#pragma once

#include "dia2.h"
#include "DiaSymbol.h"
#include <vector>
#include <string>
#include <wil\com.h>

class DiaSession {
public:
	bool OpenImage(PCWSTR path);
	bool OpenPdb(PCWSTR path);
	void Close();
	std::wstring LastError() const;
	operator bool() const;

	DiaSymbol GlobalScope() const;
	std::vector<DiaSymbol> FindChildren(DiaSymbol const& parent, PCWSTR name = nullptr, SymbolTag tag = SymbolTag::Null, CompareOptions options = CompareOptions::None) const;
	// global scope
	std::vector<DiaSymbol> FindChildren(PCWSTR name = nullptr, SymbolTag tag = SymbolTag::Null, CompareOptions options = CompareOptions::None) const;

	DiaSymbol GetSymbolByVA(ULONGLONG va, SymbolTag tag = SymbolTag::Null, long* disp = nullptr) const;

private:
	bool OpenCommon(PCWSTR path, bool image);

private:
	wil::com_ptr<IDiaSession> m_spSession;
	wil::com_ptr<IDiaDataSource> m_spSource;
};

