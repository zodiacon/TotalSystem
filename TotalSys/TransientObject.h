#pragma once

class TransientObject abstract {
public:
	virtual void New(uint32_t ms);
	virtual void Term(uint32_t ms);
	virtual bool Update();

	bool IsNew() const {
		return m_IsNew;
	}

	bool IsTerminated() const {
		return m_IsTerminated;
	}

private:
	DWORD64 m_ExpiryTime;
	bool m_IsNew : 1 { false }, m_IsTerminated : 1 { false };
};

