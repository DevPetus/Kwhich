#pragma once
#include <QWidget>
#include <string>
#include <vector>

#include "inspect/proc_info.h"
#include "inspect/process_tree.h"
#include "inspect/source_rules.h"

struct PanelExtras {
    std::string installDate;
    std::string dualInstallNote;
    std::vector<ProcessTreeEntry> processTree;
};

class QResizeEvent;

class InspectorPanel : public QWidget {
public:
    InspectorPanel(const ProcInfo& info, const SourceResult& source, const PanelExtras& extras,
                   const std::string& desktopFile);

protected:
    void resizeEvent(QResizeEvent* event) override;
};
