#include "pch.h"
#include "ThreadStackWindow.h"
#include "Globals.h"
#include "FormatHelper.h"

using namespace std;
using namespace ImGui;
using namespace WinLL;

ThreadStackWindow::ThreadStackWindow(std::shared_ptr<ProcessInfoEx> p, std::shared_ptr<ThreadInfo> t, std::vector<STACKFRAME64> const& frames)
	: m_Thread(move(t)), m_Process(move(p)) {
	m_Title = format("Stack of Thread {} (PID:{}) Image: {}", m_Thread->Id, m_Thread->ProcessId, FormatHelper::UnicodeToUtf8(m_Process->GetImageName().c_str()));

	for (auto& frame : frames) {
		StackFrame sf;
		sf.Address = frame.AddrPC.Offset;
		m_Frames.push_back(move(sf));
	}
}

void ThreadStackWindow::Build() {
	if (IsOpen()) {
		PushFont(Globals::VarFont());
		SetNextWindowSizeConstraints(ImVec2(400, 300), ImVec2(FLT_MAX, FLT_MAX));
		SetNextWindowSize(ImVec2(640, 350), ImGuiCond_FirstUseEver);
		if (Begin(m_Title.c_str(), GetOpenAddress(), ImGuiWindowFlags_NoSavedSettings)) {
			if (BeginTable("##stack", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingFixedFit)) {
				PushFont(Globals::MonoFont());
				TableSetupScrollFreeze(0, 1);

				TableSetupColumn("#");
				TableSetupColumn("Address", ImGuiTableColumnFlags_NoResize);
				TableSetupColumn("Symbol", ImGuiTableColumnFlags_None, 230);
				TableHeadersRow();

				ImGuiListClipper clipper;
				clipper.Begin((int)m_Frames.size());

				int i = 0;
				while (clipper.Step()) {
					for (int j = clipper.DisplayStart; j < clipper.DisplayEnd; j++) {
						auto& frame = m_Frames[j];
						TableNextRow();

						if (TableSetColumnIndex(0)) {
							if (Selectable(format("{:3} ", (int)m_Frames.size() - j).c_str(), m_SelectedIndex == j, ImGuiSelectableFlags_SpanAllColumns)) {
								m_SelectedIndex = j;
							}
						}
						if (TableSetColumnIndex(1)) {
							TextUnformatted(format("0x{:016X}", frame.Address).c_str());
						}
						if (TableSetColumnIndex(2)) {
							auto name = m_Process->GetAddressSymbol(frame.Address);
							if (name[0] == '0')
								name.clear();
							TextUnformatted(name.c_str());
						}
					}
				}

				PopFont();
				EndTable();
			}
		}
		End();
		PopFont();
	}
}

