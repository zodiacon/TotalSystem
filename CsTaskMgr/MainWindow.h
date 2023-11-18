#pragma once

#include "ProcessesModel.h"

class MainWindow : public QMainWindow {
	CS_OBJECT(MainWindow)

public:
	MainWindow(QWidget* parent = nullptr);

	void RunAsAdmin();
	void KillProcess();

private:
	ProcessesModel m_Processes;
	QSortFilterProxyModel m_ProcessesProxyModel;
	QTabWidget m_Tabs;
	QTableView m_ProcessesView;
	QToolBar m_ToolBar;
};
