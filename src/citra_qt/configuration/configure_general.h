// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

class HotkeyRegistry;

namespace Ui {
class ConfigureGeneral;
}

class ConfigureGeneral : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureGeneral(QWidget* parent = nullptr);
    ~ConfigureGeneral() override;

    void PopulateHotkeyList(const HotkeyRegistry& registry);
    void ResetDefaults();
    void applyConfiguration();
    void retranslateUi();
    bool sdmc_dir_changed;
    void setConfiguration();

private:
    std::unique_ptr<Ui::ConfigureGeneral> ui;
};
