#include "pch.h"
#include "ProcessesModel.h"
#include <WinLowLevel.h>
#include "FormatHelper.h"

ProcessesModel::ProcessesModel() {
	std::vector<ColumnInfo> columns{
		{ "Name", Qt::AlignLeft, 180 },
		{ "ID", Qt::AlignRight, 70, ColumnFlags::Visible | ColumnFlags::Fixed },
		{ "Parent ID", Qt::AlignRight, 60, ColumnFlags::Fixed },
		{ "CPU %", Qt::AlignRight, 60, ColumnFlags::Visible | ColumnFlags::Fixed },
		{ "Session", Qt::AlignRight, 60, ColumnFlags::Visible | ColumnFlags::Fixed },
		{ "User Name", Qt::AlignLeft, 150 },
		{ "Working Set", Qt::AlignRight },
		{ "Commit Size", Qt::AlignRight },
		{ "Threads", Qt::AlignRight, 50, ColumnFlags::Visible | ColumnFlags::Fixed },
		{ "Handles", Qt::AlignRight, 60, ColumnFlags::Visible | ColumnFlags::Fixed },
		{ "Pri", Qt::AlignRight, 40, ColumnFlags::Visible | ColumnFlags::Fixed },
		{ "Created", Qt::AlignRight, 140 },
		{ "Pri Cls", Qt::AlignLeft, 100, ColumnFlags::Fixed },
		{ "Command Line", Qt::AlignLeft, 300 },
		{ "Image Path", Qt::AlignLeft, 250, ColumnFlags::None },
		{ "Package Name", Qt::AlignLeft, 200, ColumnFlags::None  },
		{ "CPU Time", Qt::AlignRight, 80, ColumnFlags::None  },
		{ "Kernel Time", Qt::AlignRight, 80, ColumnFlags::None  },
		{ "User Time", Qt::AlignRight, 80, ColumnFlags::None },
		{ "Peak Thr", Qt::AlignRight, 60, ColumnFlags::Fixed | ColumnFlags::Visible },
	};
	m_Columns = std::move(columns);
	m_ProcMgr.EnumProcesses();
	m_Processes = m_ProcMgr.GetProcesses();
	m_Timer.setInterval(1000);

	connect(&m_Timer, &QTimer::timeout, this, &ProcessesModel::OnTick);

	m_Timer.start();
}

void ProcessesModel::OnTick() {
	DoUpdate();
}

void ProcessesModel::DoUpdate() {
	auto tick = ::GetTickCount64();
	for (size_t i = 0; i < m_NewProcesses.size(); i++) {
		auto& proc = m_NewProcesses[i];
		if (proc->TargetTime < tick) {
			proc->State = ProcessState::Running;
			m_NewProcesses.erase(m_NewProcesses.begin() + i);
			i--;
		}
	}
	int term = 0;
	for (size_t i = 0; i < m_TerminatedProcesses.size(); i++) {
		auto& proc = m_TerminatedProcesses[i];
		if (proc->TargetTime < tick) {
			m_Processes.erase(std::find(m_Processes.begin(), m_Processes.end(), proc));
			m_TerminatedProcesses.erase(m_TerminatedProcesses.begin() + i);
			term++;
			i--;
		}
	}
	if (term) {
		beginRemoveRows(QModelIndex(), rowCount() - term, rowCount() - 1);
		endRemoveRows();
	}
	m_ProcMgr.EnumProcesses();
	int count = (int)m_ProcMgr.GetNewProcesses().size();
	if (count) {
		beginInsertRows(QModelIndex(), rowCount(), rowCount() + count - 1);
		for (auto& proc : m_ProcMgr.GetNewProcesses()) {
			proc->State = ProcessState::New;
			proc->TargetTime = tick + 2000;
			m_Processes.push_back(proc);
		}
		endInsertRows();
		m_NewProcesses.insert(m_NewProcesses.end(), m_ProcMgr.GetNewProcesses().begin(), m_ProcMgr.GetNewProcesses().end());
	}
	for (auto& proc : m_ProcMgr.GetTerminatedProcesses()) {
		proc->State = ProcessState::Terminated;
		proc->TargetTime = tick + 2000;
		m_TerminatedProcesses.push_back(proc);
	}

	static const QVector<int> roles = { Qt::DisplayRole };
	dataChanged(createIndex(0, 0), createIndex(rowCount() - 1, columnCount() - 1), roles);
}

QString const& ProcessesModel::GetImagePath(ProcessInfoEx* proc) const {
	if (proc->ImagePath.isEmpty()) {
		WinLL::Process p;
		if (p.Open(proc->Id, WinLL::ProcessAccessMask::QueryLimitedInformation))
			proc->ImagePath = QString::fromStdWString(p.GetImagePath());
	}
	return proc->ImagePath;
}

QString const& ProcessesModel::GetCommandLine(ProcessInfoEx* proc) const {
	if (proc->CommandLine.isEmpty()) {
		WinLL::Process p;
		if (p.Open(proc->Id, WinLL::ProcessAccessMask::QueryLimitedInformation))
			proc->CommandLine = QString::fromStdWString(p.GetCommandLine());
	}
	return proc->CommandLine;
}

QVariant ProcessesModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (orientation == Qt::Horizontal) {
		switch (role) {
			case Qt::DisplayRole: return m_Columns[section].Text;

		}
	}
	return QVariant();
}

int ProcessesModel::rowCount(const QModelIndex& parent) const {
	return (int)m_Processes.size();
}

int ProcessesModel::columnCount(const QModelIndex& parent) const {
	return (int)m_Columns.size();
}

QVariant ProcessesModel::data(const QModelIndex& index, int role) const {
	switch (role) {
		case Qt::TextAlignmentRole:
			return QVariant(m_Columns[index.column()].Align | Qt::AlignVCenter);

		case Qt::BackgroundRole:
		{
			auto row = index.row();
			auto const& proc = m_Processes[row];
			switch (proc->State) {
				case ProcessState::New: return QColor(0x00, 0xff, 0x00, 0x80);
				case ProcessState::Terminated: return QColor(0xff, 0x00, 0x00, 0x80);
			}
			break;
		}

		case Qt::UserRole:	// sorting
		{
			auto row = index.row();
			auto& proc = m_Processes[row];
			switch (static_cast<ColumnType>(index.column())) {
				case ColumnType::CPU: return proc->CPU;
				case ColumnType::WorkingSet: return proc->WorkingSetSize;
				case ColumnType::CommitSize: return proc->PagefileUsage;
				case ColumnType::Handles: return proc->HandleCount;
				case ColumnType::CreateTime: return proc->CreateTime;
				case ColumnType::ID: proc->Id;
			}
		}
		[[fallthrough]];
		case Qt::DisplayRole:
		{
			auto row = index.row();
			auto proc = m_Processes[row].get();
			switch (static_cast<ColumnType>(index.column())) {
				case ColumnType::Name:	return QString::fromStdWString(proc->GetImageName());
				case ColumnType::CommandLine: return GetCommandLine(proc);
				case ColumnType::ImagePath:	return GetImagePath(proc);
				case ColumnType::ParentID: return QVariant(proc->ParentId);
				case ColumnType::Session: return proc->SessionId;
				case ColumnType::Threads: return proc->ThreadCount;
				case ColumnType::PeakThreads: return proc->PeakThreads;
				case ColumnType::UserName:
				{
					auto name = QString::fromStdWString(proc->GetUserName());
					if (name.isEmpty())
						name = "<Access Denied>";
					return name;
				}

				case ColumnType::Priority: return proc->BasePriority;
				case ColumnType::ID: return QString::fromStdWString(FormatHelper::FormatNumber(proc->Id));
				case ColumnType::Handles: return QString::fromStdWString(FormatHelper::FormatNumber(proc->HandleCount));
				case ColumnType::WorkingSet: return QString::fromStdWString(FormatHelper::FormatNumber(proc->WorkingSetSize >> 10)) + " K";
				case ColumnType::CommitSize: return QString::fromStdWString(FormatHelper::FormatNumber(proc->PrivatePageCount >> 10)) + " K";
				case ColumnType::CreateTime:
					if (proc->CreateTimeAsString.isEmpty())
						proc->CreateTimeAsString = QString::fromStdWString(FormatHelper::FormatDateTime(proc->CreateTime));
					return proc->CreateTimeAsString;

				case ColumnType::CPU:
					if (proc->CPU == 0)
						return QString();
					return QString::fromStdWString(FormatHelper::FormatNumber(proc->CPU / 10000.0f, 2));
			}
		}
	}
	return QVariant();
}

Qt::ItemFlags ProcessesModel::flags(const QModelIndex& index) const {
	return QAbstractTableModel::flags(index);
}

