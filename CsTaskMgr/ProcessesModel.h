#pragma once

#include <ProcessManager.h>

class ProcessesModel : public QAbstractTableModel {
public:
    enum class ProcessState {
        Running,
        New,
        Terminated
    };
    struct ProcessInfoEx : WinLL::ProcessInfo {
        ProcessState State{ ProcessState::Running };
        DWORD64 TargetTime;
        QString CreateTimeAsString;
        QString ImagePath;
        QString CommandLine;
    };

    enum class ColumnType {
        Name,
        ID,
        ParentID,
        CPU,
        Session,
        UserName,
        WorkingSet,
        CommitSize,
        Threads,
        Handles,
        Priority,
        CreateTime,
        PriorityClass,
        CommandLine,
        ImagePath,
        PackageFullName,
        CPUTime,
        KernelTime,
        UserTime,
        PeakThreads,
    };

    enum class ColumnFlags {
        None = 0,
        Visible = 1,
        Fixed = 2,
    };

    struct ColumnInfo {
        QString Text;
        Qt::Alignment Align{ Qt::AlignLeft };
        int Width{ -1 };
        ColumnFlags Flags { ColumnFlags::Visible };
    };

    ProcessesModel();

    std::vector<ColumnInfo>& GetColumns() { return m_Columns; }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    void OnTick();
    void DoUpdate();

private:
    QString const& GetImagePath(ProcessInfoEx* proc) const;
    QString const& GetCommandLine(ProcessInfoEx* proc) const;

private:
    QTimer m_Timer;
    std::vector<ColumnInfo> m_Columns;
    WinLL::ProcessManager<ProcessInfoEx> m_ProcMgr;
    std::vector<std::shared_ptr<ProcessInfoEx>> m_Processes;
    std::vector<std::shared_ptr<ProcessInfoEx>> m_NewProcesses;
    std::vector<std::shared_ptr<ProcessInfoEx>> m_TerminatedProcesses;
};

DEFINE_ENUM_FLAG_OPERATORS(ProcessesModel::ColumnFlags);
