#include "pch.h"
#include "TransientObject.h"

void TransientObject::New(uint32_t ms) {
	m_IsNew = true;
	m_ExpiryTime = ::GetTickCount64() + ms;
}

void TransientObject::Term(uint32_t ms) {
	m_IsNew = false;
	m_IsTerminated = true;
	m_ExpiryTime = ::GetTickCount64() + ms;
}

bool TransientObject::Update() {
	if (!m_IsNew && !m_IsTerminated)
		return false;

	bool term = m_IsTerminated;
	if (::GetTickCount64() > m_ExpiryTime) {
		m_IsTerminated = m_IsNew = false;
		return term;
	}
	return false;
}
