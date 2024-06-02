#pragma once

#include <string>
#include "imgui.h"

struct ImFont;

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
	SimpleMessageBox() = default;
	SimpleMessageBox(std::string title, std::string text, MessageBoxButtons buttons = MessageBoxButtons::OK);
	void Init(std::string title, std::string text, MessageBoxButtons buttons = MessageBoxButtons::OK);
	void SetFont(ImFont* font);
	void SetImage(ImTextureID image, float width = 0, float height = 0);

	MessageBoxResult ShowModal();

	bool IsEmpty() const;
	void Empty();

private:
	std::string m_Title, m_Text;
	ImFont* m_Font{ nullptr };
	MessageBoxButtons m_Buttons;
	ImTextureID m_Image;
	ImVec2 m_ImageSize{ 32, 32 };
	bool m_Open{ true };
};
