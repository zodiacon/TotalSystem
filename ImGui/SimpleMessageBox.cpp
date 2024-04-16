#include "SimpleMessageBox.h"
#include "imgui.h"

using namespace ImGui;
using namespace std;

void SimpleMessageBox::Init(string title, string text, MessageBoxButtons buttons) {
	m_Title = move(title);
	m_Text = move(text);
	m_Buttons = buttons;
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

	if (BeginPopupModal(m_Title.c_str(), &m_Open, ImGuiWindowFlags_AlwaysAutoResize)) {
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
		EndPopup();
	}
	return result;
}

bool SimpleMessageBox::IsEmpty() const {
	return m_Title.empty();
}
