#include "pch.h"
#include "MainWindow.h"
#include <WinLowLevel.h>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
	setWindowTitle("CS Task Manager");
	setWindowIcon(QIcon(":/Icons/app.ico"));
	auto sb = statusBar();
	sb->showMessage("Ready");
	auto mb = menuBar();

	//
	// File menu
	//
	auto fileMenu = mb->addMenu("&File");
	if (!WinLL::SecurityHelper::IsRunningElevated()) {
		fileMenu->addAction(QIcon(":/Icons/shield.ico"), "&Run as administrator", this, &MainWindow::RunAsAdmin);
		fileMenu->addSeparator();
	}
	fileMenu->addAction("&Exit", this, &MainWindow::close, QKeySequence::Quit);

	//
	// View menu
	//
	auto viewMenu = mb->addMenu("&View");
	auto viewGridAction = new QAction("&Grid", this);
	viewGridAction->setCheckable(true);
	viewGridAction->setChecked(true);
	connect(viewGridAction, &QAction::toggled, this, [=]() {
		m_ProcessesView.setShowGrid(viewGridAction->isChecked());
		});
	viewMenu->addAction(viewGridAction);

	auto viewToolbarAction = new QAction("&Toolbar", this);
	viewToolbarAction->setCheckable(true);
	viewToolbarAction->setChecked(true);
	connect(viewToolbarAction, &QAction::toggled, this, [=]() {
		m_ToolBar.setVisible(viewToolbarAction->isChecked());
		});
	
	viewMenu->addAction(viewToolbarAction);

	//
	// Process menu
	//
	auto killProcessAction = new QAction(QIcon(":/Icons/process_kill.ico"), "&Kill Process", this);
	auto processMenu = mb->addMenu("&Process");
	processMenu->addAction(killProcessAction);

	addToolBar(&m_ToolBar);
	m_ToolBar.addAction(killProcessAction);
	connect(killProcessAction, &QAction::triggered, this, &MainWindow::KillProcess);

	setCentralWidget(&m_Tabs);
	m_Tabs.addTab(&m_ProcessesView, QIcon(":/Icons/Processes.ico"), "Processes");
	resize(1024, 800);
	m_ProcessesProxyModel.setSourceModel(&m_Processes);
	m_ProcessesView.setModel(&m_ProcessesProxyModel);
	m_ProcessesView.setShowGrid(true);
	m_ProcessesView.setSortingEnabled(true);
	m_ProcessesView.setIconSize(QSize(16, 16));

	m_ProcessesProxyModel.setSortRole(Qt::UserRole);
	m_ProcessesProxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);

	m_ProcessesProxyModel.setSortCaseSensitivity(Qt::CaseInsensitive);
	m_ProcessesView.setSelectionBehavior(QAbstractItemView::SelectRows);
	m_ProcessesView.setSelectionMode(QAbstractItemView::SingleSelection);

	auto header = m_ProcessesView.horizontalHeader();
	header->setSectionsMovable(true);
	header->setSortIndicator(-1, Qt::AscendingOrder);

	header = m_ProcessesView.verticalHeader();
	header->setDefaultSectionSize(20);

	int i = 0;
	for (auto& col : m_Processes.GetColumns()) {
		if (col.Width > 0)
			m_ProcessesView.setColumnWidth(i, col.Width);
		if ((col.Flags & ProcessesModel::ColumnFlags::Visible) == ProcessesModel::ColumnFlags::None)
			m_ProcessesView.setColumnHidden(i, true);
		if ((col.Flags & ProcessesModel::ColumnFlags::Fixed) == ProcessesModel::ColumnFlags::Fixed)
			m_ProcessesView.horizontalHeader()->setSectionResizeMode(i, QHeaderView::Fixed);
		i++;
	}
}

void MainWindow::RunAsAdmin() {
	if (WinLL::SecurityHelper::RunElevated())
		close();
}

void MainWindow::KillProcess() {
	auto row = m_ProcessesProxyModel.mapToSource(m_ProcessesView.currentIndex()).row();
	if (row >= 0) {
		auto proc = m_Processes.GetProcess(row);
		auto res = QMessageBox::warning(this, tr("Kill Process"), 
			QString::fromStdWString(std::format(L"Kill process {} ({})?", proc->Id, proc->GetImageName())), 
			QMessageBox::Yes, QMessageBox::No);
		if (res == QMessageBox::Yes) {
			if (!m_Processes.KillProcess(row))
				QMessageBox::warning(this, tr("Kill Process"), tr("Failed to kill process"));
		}
	}
}
