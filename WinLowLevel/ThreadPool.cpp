#include "pch.h"
#include "WinLowLevel.h"

namespace WinLL {
	bool ThreadPool::Create() {
		m_Tp = ::CreateThreadpool(nullptr);
		return *this;
	}

	ThreadPool::operator bool() const {
		return m_Tp != nullptr;
	}

	ThreadPool::operator PTP_POOL() const {
		return m_Tp;
	}

	void ThreadPool::SetMaxThreads(uint32_t threads) {
		::SetThreadpoolThreadMaximum(m_Tp, threads);
	}

	void ThreadPool::Close() {
		if (m_Tp) {
			::CloseThreadpool(m_Tp);
			m_Tp = nullptr;
		}
	}

	ThreadPool::~ThreadPool() {
		Close();
	}
}
