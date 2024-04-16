#pragma once

#include <string>

enum class MessageBoxResult {
	Error = -1,
	None = 0,
	Cancel = 1,
	OK = 2,
	No = Cancel,
	Yes = OK,
};

enum class MessageBoxButtons {
	OK,
	YesNo,
	OkCancel
};

class SimpleMessageBox {
public:
	void Init(std::string title, std::string text, MessageBoxButtons buttons = MessageBoxButtons::OK);
	MessageBoxResult ShowModal();

	bool IsEmpty() const;
	void Empty();

private:
	bool m_Open{ true };
	std::string m_Title, m_Text;
	MessageBoxButtons m_Buttons;
};
