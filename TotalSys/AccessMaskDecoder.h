#pragma once

class AccessMaskDecoder abstract final {
public:
	static std::string DecodeAccessMask(std::wstring const& typeName, ACCESS_MASK access);

private:
	struct AccessMaskPair {
		DWORD AccessMask;
		PCSTR Decoded;
		bool All{ false };
	};

	static std::unordered_map<std::wstring, std::vector<AccessMaskPair>> Tables;
};

