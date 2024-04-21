#pragma once

#include <vector>
#include <algorithm>
#include <functional>

template<typename T>
class SortedFilteredVector {
public:
	SortedFilteredVector(size_t capacity = 16) {
		reserve(capacity);
	}

	SortedFilteredVector& operator=(std::vector<T> const& other) {
		m_Items = other;
		size_t count;
		m_Indices.resize(count = other.size());
		for (size_t i = 0; i < count; i++)
			m_Indices[i] = i;
		return *this;
	}

	void reserve(size_t capacity) {
		m_Items.reserve(capacity);
		m_Indices.reserve(capacity);
	}

	void clear() {
		m_Items.clear();
		m_Indices.clear();
	}

	bool empty() const {
		return m_Items.empty();
	}

	void push_back(const T& value) {
		m_Items.push_back(value);
		if (m_Filter == nullptr || m_Filter(value, m_Items.size() - 1))
			m_Indices.push_back(m_Indices.size());
	}

	void push_back(T&& value) {
		if (m_Filter == nullptr || m_Filter(value, m_Items.size() - 1))
			m_Indices.push_back(m_Indices.size());
		m_Items.push_back(std::move(value));
	}

	void shrink_to_fit() {
		m_Items.shrink_to_fit();
		m_Indices.shrink_to_fit();
	}

	void Remove(size_t index) {
		auto realIndex = m_Indices[index];
		m_Items.erase(m_Items.begin() + realIndex);
		m_Indices.erase(m_Indices.begin() + index);
		for (size_t i = 0; i < m_Indices.size(); i++) {
			if (m_Indices[i] >= realIndex)
				m_Indices[i]--;
		}
	}

	void ClearSort() {
		int c = 0;
		for (auto& i : m_Indices)
			i = c++;
	}

	typename std::vector<T>::const_iterator begin() const {
		return m_Items.begin();
	}

	typename std::vector<T>::const_iterator end() const {
		return m_Items.end();
	}

	template<typename Iterator>
	void append(Iterator begin, Iterator end) {
		for (auto it = begin; it != end; ++it)
			push_back(std::move(*it));
	}

	template<typename Iterator>
	void insert(size_t at, Iterator begin, Iterator end) {
		//
		// only call after ResetSort and no filter
		//
		size_t count = end - begin;
		m_Items.insert(m_Items.begin() + at, begin, end);
		std::vector<size_t> indices(count);
		for (size_t c = 0; c < count; ++c) {
			indices[c] = c + at;
		}
		m_Indices.insert(m_Indices.begin() + at, indices.begin(), indices.end());
		for (size_t c = at + count; c < m_Indices.size(); c++)
			m_Indices[c] += count;
	}

	void Set(std::vector<T> items) {
		m_Items = std::move(items);
		auto count = m_Items.size();
		m_Indices.clear();
		m_Indices.reserve(count);
		for (decltype(count) i = 0; i < count; i++)
			m_Indices.push_back(i);
	}

	const T& operator[](size_t index) const {
		return m_Items[m_Indices[index]];
	}

	T& operator[](size_t index) {
		return m_Items[m_Indices[index]];
	}

	const T& GetReal(size_t index) const {
		return m_Items[index];
	}

	void Sort(std::function<bool(const T& value1, const T& value2)> compare) {
		std::sort(m_Indices.begin(), m_Indices.end(), [&](size_t i1, size_t i2) {
			return compare(m_Items[i1], m_Items[i2]);
			});
	}

	void Sort(size_t start, size_t end, std::function<bool(const T& value1, const T& value2)> compare) {
		if (start >= m_Indices.size())
			return;

		std::sort(m_Indices.begin() + start, end == 0 ? m_Indices.end() : (m_Indices.begin() + end), [&](size_t i1, size_t i2) {
			return compare(m_Items[i1], m_Items[i2]);
			});
	}

	size_t size() const {
		return m_Indices.size();
	}

	size_t TotalSize() const {
		return m_Items.size();
	}

	void Filter(std::function<bool(const T&, size_t)> predicate, bool append = false) {
		m_Filter = predicate;
		if (!append) {
			m_Indices.clear();
		}
		auto count = m_Items.size();
		if (predicate == nullptr && !append) {
			for (decltype(count) i = 0; i < count; i++)
				m_Indices.push_back(i);
		}
		else if (append) {
			std::vector<size_t> indices2(m_Indices);
			int j = 0;
			for (decltype(count) i = 0; i < m_Indices.size(); i++, j++) {
				if (!predicate(m_Items[m_Indices[i]], (int)i)) {
					indices2.erase(indices2.begin() + j);
					j--;
				}
			}
			m_Indices = std::move(indices2);
		}
		else {
			for (decltype(count) i = 0; i < count; i++)
				if (predicate(m_Items[i], int(i)))
					m_Indices.push_back(i);
		}
	}

	bool erase(size_t index) {
		if (index >= m_Items.size())
			return false;

		m_Items.erase(m_Items.begin() + m_Indices[index]);
		m_Indices.erase(m_Indices.begin() + index);
		for (; index < m_Indices.size(); index++)
			m_Indices[index]--;

		return true;
	}

	const std::vector<T>& GetRealAll() const {
		return m_Items;
	}

	const std::vector<T> GetItems() const {
		std::vector<T> items;
		items.reserve(size());
		for (size_t i = 0; i < size(); i++)
			items.push_back(m_Items[m_Indices[i]]);
		return items;
	}

	const std::vector<T>& GetAllItems() const {
		return m_Items;
	}

private:
	std::vector<T> m_Items;
	std::vector<size_t> m_Indices;
	std::function<bool(const T&, size_t)> m_Filter;
};

