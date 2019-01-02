// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>

class HotkeyRegistry;

namespace Ui {
class ConfigureDialog;
}

class ConfigureDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConfigureDialog(QWidget* parent, const HotkeyRegistry& registry);
    ~ConfigureDialog() override;

    void applyConfiguration();
    void UpdateVisibleTabs();
    void PopulateSelectionList();

    bool sdmc_dir_changed;

private slots:
    void onLanguageChanged(const QString& locale);

signals:
    void languageChanged(const QString& locale);

private:
    void setConfiguration();
    void retranslateUi();

    std::unique_ptr<Ui::ConfigureDialog> ui;
};
