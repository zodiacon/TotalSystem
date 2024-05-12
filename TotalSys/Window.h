#pragma once

class Window abstract {
public:
	bool IsOpen() const;
	void Open(bool open = true);
	bool* GetOpenAddress();

	virtual void Build() = 0;

private:
	bool m_Open{ false };
};

