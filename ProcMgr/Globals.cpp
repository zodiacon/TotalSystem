#include "pch.h"
#include "Globals.h"
#include "ProcMgrSettings.h"

ProcMgrSettings& Globals::Settings() {
	static ProcMgrSettings settings;
	return settings;
}
