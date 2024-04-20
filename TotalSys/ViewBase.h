#pragma once

class ViewBase abstract {
public:
	bool IsOpen() const;
	void Open(bool open = true);
	bool* GetOpenAddress();

private:
	bool m_Open{ false };
};

