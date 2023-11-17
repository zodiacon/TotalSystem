#include "pch.h"
#include "MainWindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
	setWindowTitle("CS Task Manager");
	setWindowIcon(QIcon(":/Icons/app.ico"));
	auto sb = statusBar();
	sb->showMessage("Ready");
	auto mb = menuBar();
	auto fileMenu = mb->addMenu("&File");
	if (!WinLL::SecurityHelper::IsRunningElevated()) {
		fileMenu->addAction("&Run as administrator", this, &MainWindow::RunAsAdmin);
		fileMenu->addSeparator();
	}
	fileMenu->addAction("&Exit", this, &MainWindow::close, QKeySequence::Quit);

	setCentralWidget(&m_Tabs);
	m_Tabs.addTab(&m_ProcessesView, QIcon(":/Icons/Processes.ico"), "Processes");
	resize(1024, 800);

	m_ProcessesProxyModel.setSourceModel(&m_Processes);
	m_ProcessesView.setModel(&m_ProcessesProxyModel);
	m_ProcessesView.setShowGrid(true);
	m_ProcessesView.setSortingEnabled(true);
	
	m_ProcessesProxyModel.setSortRole(Qt::UserRole);
	m_ProcessesProxyModel.setFilterCaseSensitivity(Qt::CaseInsensitive);
	m_ProcessesProxyModel.setSortCaseSensitivity(Qt::CaseInsensitive);
	m_ProcessesView.setSelectionBehavior(QAbstractItemView::SelectRows);
	m_ProcessesView.setSelectionMode(QAbstractItemView::SingleSelection);

	auto header = m_ProcessesView.horizontalHeader();
	header->setSectionsMovable(true);
	header->setSortIndicator(-1, Qt::AscendingOrder);

	header = m_ProcessesView.verticalHeader();
	header->setSectionResizeMode(QHeaderView::Interactive);
	header->setDefaultSectionSize(16);

	int i = 0;
	for (auto& col : m_Processes.GetColumns()) {
		if (col.Width > 0)
			m_ProcessesView.setColumnWidth(i, col.Width);
		if ((col.Flags & ProcessesModel::ColumnFlags::Visible) == ProcessesModel::ColumnFlags::None)
			m_ProcessesView.setColumnHidden(i, true);
		if((col.Flags & ProcessesModel::ColumnFlags::Fixed) == ProcessesModel::ColumnFlags::Fixed)
			m_ProcessesView.horizontalHeader()->setSectionResizeMode(i, QHeaderView::Fixed);
		i++;
	}
}

void MainWindow::RunAsAdmin() {
	if(WinLL::SecurityHelper::RunElevated())
		close();
}

