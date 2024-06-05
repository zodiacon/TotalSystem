#pragma once

template<typename T>
struct IDataProvider abstract {
	virtual T const& Get(uint32_t index) const = 0;
	virtual uint32_t GetLimit() const = 0;
};
