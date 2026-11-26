#include "inspector_panel.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QProcess>
#include <QPushButton>
#include <QResizeEvent>
#include <QSizePolicy>

#include <csignal>
#include <filesystem>
#include <functional>

#include "actions/file_manager.h"
#include "inspect/wine_path.h"

namespace {

/**
 * Wraps a value in a selectable, word-wrapping QLabel, falling back to a
 * placeholder when the value is empty (an unreadable /proc field).
 * @param value text to show
 * @param placeholder shown instead when value is empty
 * @return a QLabel ready to drop into the form layout
 */
QLabel* valueLabel(const std::string& value, const char* placeholder = "-") {
    auto* label = new QLabel(value.empty() ? QString(placeholder) : QString::fromStdString(value));
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return label;
}

/**
 * Joins a process ancestry chain into one display string, closest first.
 * @param chain entries as returned by getProcessTree
 * @return e.g. "game (1234) ← reaper (1200) ← steam (1000)", or empty if chain is empty
 */
std::string formatProcessTree(const std::vector<ProcessTreeEntry>& chain) {
    std::string out;
    for (size_t i = 0; i < chain.size(); ++i) {
        if (i > 0) out += " ← ";
        out += chain[i].name + " (" + std::to_string(chain[i].pid) + ")";
    }
    return out;
}

/**
 * Picks the first argv entry that resolves to a real, existing path and
 * differs from exePath. Handles both plain absolute Unix paths (the
 * `interpreter /path/to/thing args...` case, where the interpreter at
 * argv[0] usually equals exePath and gets skipped in favor of the real
 * target later in argv) and Windows-style paths from a Wine process's argv
 * -- Wine sets argv[0] itself to the target exe (e.g. Wow.exe's
 * "C:\Program Files...\Wow.exe"), which is why this doesn't start at argv[1].
 * @param info proc info for the inspected process
 * @param exePath that process's resolved /proc/<pid>/exe, to skip duplicates
 * @return the candidate path, or empty if none of the arguments qualify
 */
std::string findCommandLinePath(const ProcInfo& info, const std::string& exePath) {
    for (const std::string& arg : info.argv) {
        std::string candidate;
        if (!arg.empty() && arg.front() == '/' && std::filesystem::exists(arg)) {
            candidate = arg;
        } else if (auto winePath = translateWinePath(arg, info.environ)) {
            candidate = *winePath;
        }
        if (!candidate.empty() && candidate != exePath) return candidate;
    }
    return {};
}

/**
 * Builds a button offering a path to act on: a plain single-action button
 * when only one of exePath/cmdlinePath is usable (or they're identical --
 * showing the same path twice as a "choice" adds nothing), or a dropdown
 * with both when they're genuinely distinct.
 * @param label button text
 * @param exePath executable path choice, may be empty
 * @param cmdlinePath command line path choice, may be empty or == exePath
 * @param action called with whichever path ends up chosen
 * @return the ready-to-place QPushButton
 */
QPushButton* pathChoiceButton(const QString& label, const std::string& exePath, const std::string& cmdlinePath,
                               std::function<void(const std::string&)> action) {
    auto* button = new QPushButton(label);

    const bool hasExe = !exePath.empty();
    const bool hasDistinctCmdlinePath = !cmdlinePath.empty() && cmdlinePath != exePath;

    if (hasExe && hasDistinctCmdlinePath) {
        auto* menu = new QMenu(button);
        auto* exeAction = menu->addAction("Executable path");
        QObject::connect(exeAction, &QAction::triggered, button, [exePath, action]() { action(exePath); });
        auto* cmdlineAction = menu->addAction("Command line path");
        QObject::connect(cmdlineAction, &QAction::triggered, button, [cmdlinePath, action]() { action(cmdlinePath); });
        button->setMenu(menu);
    } else if (hasExe) {
        QObject::connect(button, &QPushButton::clicked, button, [exePath, action]() { action(exePath); });
    } else if (hasDistinctCmdlinePath) {
        QObject::connect(button, &QPushButton::clicked, button, [cmdlinePath, action]() { action(cmdlinePath); });
    } else {
        button->setEnabled(false);
    }

    return button;
}

}  // namespace

/**
 * Builds a static form showing one inspected window's process info,
 * detected source, and the M4 extras (install date, dual-install note,
 * process tree) when available, plus a row of action buttons (open
 * folder, copy path, system monitor, kill) wired to the process's pid/exe.
 * @param info proc info for the inspected window
 * @param source detected source for that process
 * @param extras optional install date / dual-install / process tree info
 */
InspectorPanel::InspectorPanel(const ProcInfo& info, const SourceResult& source, const PanelExtras& extras) {
    setWindowTitle("Kwhich");
    setMinimumWidth(420);

    auto* layout = new QFormLayout(this);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->addRow("PID:", valueLabel(std::to_string(info.pid)));
    layout->addRow("Executable:", valueLabel(info.exe, "(unreadable)"));
    layout->addRow("Command line:", valueLabel(info.cmdline, "(unreadable)"));
    layout->addRow("Source:", valueLabel(source.kind));
    layout->addRow("Detail:", valueLabel(source.detail));

    if (!extras.installDate.empty()) {
        layout->addRow("Installed:", valueLabel(extras.installDate));
    }
    if (!extras.dualInstallNote.empty()) {
        layout->addRow("Note:", valueLabel(extras.dualInstallNote));
    }
    if (!extras.processTree.empty()) {
        layout->addRow("Process tree:", valueLabel(formatProcessTree(extras.processTree)));
    }

    const int pid = info.pid;
    const std::string exePathStd = info.exe;
    const std::string cmdlinePathStd = findCommandLinePath(info, exePathStd);

    auto* openFolderButton =
        pathChoiceButton("Open folder", exePathStd, cmdlinePathStd, [](const std::string& path) { showInFileManager(path); });
    auto* copyPathButton = pathChoiceButton("Copy path", exePathStd, cmdlinePathStd, [](const std::string& path) {
        QApplication::clipboard()->setText(QString::fromStdString(path));
    });
    auto* sysMonitorButton = new QPushButton("System Monitor");
    auto* killButton = new QPushButton("Kill");

    connect(sysMonitorButton, &QPushButton::clicked, this, []() {
        QProcess::startDetached("plasma-systemmonitor", {"--page-name", "Processes"});
    });
    connect(killButton, &QPushButton::clicked, this, [this, pid]() {
        ::kill(pid, SIGTERM);
        close();
    });

    auto* buttonRow = new QHBoxLayout();
    buttonRow->addWidget(openFolderButton);
    buttonRow->addWidget(copyPathButton);
    buttonRow->addWidget(sysMonitorButton);
    buttonRow->addStretch();
    buttonRow->addWidget(killButton);
    layout->addRow(buttonRow);
}

/**
 * Forces the form layout to re-activate on every resize. QFormLayout (like
 * QGridLayout underneath it) doesn't always recompute wrapped labels'
 * heightForWidth-based row heights on an interactive resize, which can
 * leave text looking clipped even though the window has grown. Explicitly
 * re-activating the layout keeps row heights in sync with the new width.
 * @param event the resize event, forwarded to QWidget as usual
 */
void InspectorPanel::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    if (layout()) layout()->activate();
}
