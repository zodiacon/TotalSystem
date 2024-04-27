#include "SimpleMessageBox.h"
#include "imgui.h"

using namespace ImGui;
using namespace std;

SimpleMessageBox::SimpleMessageBox(std::string title, std::string text, MessageBoxButtons buttons) :
	m_Title(move(title)), m_Text(move(text)), m_Buttons(buttons) {
}

void SimpleMessageBox::Init(string title, string text, MessageBoxButtons buttons) {
	m_Title = move(title);
	m_Text = move(text);
	m_Buttons = buttons;
}

void SimpleMessageBox::SetFont(ImFont* font) {
	m_Font = font;
}

void SimpleMessageBox::Empty() {
	m_Title.clear();
	m_Text.clear();
}

MessageBoxResult SimpleMessageBox::ShowModal() {
	if (m_Title.empty())
		return MessageBoxResult::Error;

	if (!IsPopupOpen(m_Title.c_str()))
		OpenPopup(m_Title.c_str());

	auto result = MessageBoxResult::None;
	auto count = 1;
	auto button1Text = "OK", button2Text = "Cancel";

	switch (m_Buttons) {
		case MessageBoxButtons::YesNo:
			button1Text = "Yes";
			button2Text = "No";
			[[fallthrough]];
		case MessageBoxButtons::OkCancel:
			count = 2;
			break;
	}

	bool open = true;
	if (m_Font)
		PushFont(m_Font);
	if (BeginPopupModal(m_Title.c_str(), &open, ImGuiWindowFlags_AlwaysAutoResize)) {
		auto winWidth = GetWindowSize().x;
		Text(m_Text.c_str());
		Dummy(ImVec2(0, 6));
		Separator();
		Dummy(ImVec2(0, 10));
		NewLine();

		auto width = 100.0f;
		SameLine((winWidth - width * count - GetStyle().ItemSpacing.x * (count - 1)) / 2);
		if (Button(button1Text, ImVec2(width, 0))) {
			CloseCurrentPopup();
			result = MessageBoxResult::OK;
			Empty();
		}
		if (count > 1) {
			SetItemDefaultFocus();
			SameLine();
			if (Button(button2Text, ImVec2(width, 0))) {
				CloseCurrentPopup();
				result = MessageBoxResult::Cancel;
				Empty();
			}
		}
		if (IsKeyPressed(ImGuiKey_Escape)) {
			CloseCurrentPopup();
			result = MessageBoxResult::Cancel;
			Empty();
		}
		if (IsKeyPressed(ImGuiKey_Enter)) {
			CloseCurrentPopup();
			result = MessageBoxResult::OK;
			Empty();
		}
		EndPopup();
	}

	if (m_Font)
		PopFont();
	if (!open)
		Empty();

	return result;
}

bool SimpleMessageBox::IsEmpty() const {
	return m_Title.empty();
}
