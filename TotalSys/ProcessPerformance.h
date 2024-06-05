#pragma once

#include <deque>

#include "Interfaces.h"

template<typename T>
class ProcessPerformance : public IDataProvider<T> {
public:
	ProcessPerformance() {
		m_Data.reserve(m_Count);
	}

	void SetLimit(uint32_t count) {
		assert(count >= 10);
		m_Count = count;
		m_Data.reserve(m_Count);
	}

	uint32_t GetLimit() const {
		return m_Count;
	}

	bool IsEmpty() const {
		return m_Data.empty();
	}

	void Add(T item) {
		m_Data.push_back(std::move(item));
		if (m_Data.size() > m_Count) {
			m_Data.erase(m_Data.begin());
		}
	}

	T const& Get(uint32_t index) const override {
		static const T empty;
		if (index >= m_Data.size())
			return empty;
		return m_Data[index];
	}

	T const* GetData() const {
		return m_Data.data();
	}

	uint32_t GetSize() const {
		return (uint32_t)m_Data.size();
	}

	T const& Peek() const {
		return m_Data.front();
	}


private:
	uint32_t m_Count{ 300 };
	std::vector<T> m_Data;
};

