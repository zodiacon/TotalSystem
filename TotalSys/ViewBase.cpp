#include "pch.h"
#include "ViewBase.h"

bool ViewBase::IsOpen() const {
	return m_Open;
}

void ViewBase::Open(bool open) {
	m_Open = open;
}

bool* ViewBase::GetOpenAddress() {
	return &m_Open;
}
