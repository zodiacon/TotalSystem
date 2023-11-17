// WinLowLevel.cpp : Defines the functions for the static library.
//

#include "pch.h"
#include "WinLowLevel.h"

namespace WinLL {
	ObjectAttributes::ObjectAttributes(PUNICODE_STRING name, AttributesFlags attributes) {
		InitializeObjectAttributes(this, name, attributes, nullptr, nullptr);
	}
}
