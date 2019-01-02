// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QFileDialog>
#include <QMessageBox>
#include "citra_qt/configuration/configure_general.h"
#include "citra_qt/ui_settings.h"
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_general.h"

ConfigureGeneral::ConfigureGeneral(QWidget* parent)
    : QWidget(parent), ui(new Ui::ConfigureGeneral) {

    ui->setupUi(this);

    connect(ui->button_sdmc_dir_empty, &QPushButton::clicked, this, [&](bool checked) {
        Q_UNUSED(checked);
        ui->sdmc_dir->setText(QString());
    });

   connect(ui->button_sdmc_dir, &QToolButton::clicked, this, [&](bool checked) {
        Q_UNUSED(checked);
        ui->sdmc_dir->setText(
            QFileDialog::getExistingDirectory(this, tr("Select SD card root")));
    });

    this->setConfiguration();

    ui->toggle_is_new_3ds->setEnabled(!Core::System::GetInstance().IsPoweredOn());
    ui->button_sdmc_dir->setEnabled(!Core::System::GetInstance().IsPoweredOn());
    ui->button_sdmc_dir_empty->setEnabled(!Core::System::GetInstance().IsPoweredOn());
    ui->updateBox->setVisible(UISettings::values.updater_found);
    connect(ui->button_reset_defaults, &QPushButton::clicked, this,
            &ConfigureGeneral::ResetDefaults);
}

ConfigureGeneral::~ConfigureGeneral() = default;

void ConfigureGeneral::setConfiguration() {
    ui->toggle_check_exit->setChecked(UISettings::values.confirm_before_closing);
    ui->toggle_is_new_3ds->setChecked(Settings::values.is_new_3ds);
    ui->toggle_use_priority_boost->setChecked(Settings::values.use_priority_boost);
    ui->toggle_use_force_indexed->setChecked(Settings::values.use_force_indexed);
    ui->sdmc_dir->setText(QString::fromStdString(Settings::values.sdmc_dir));

    ui->toggle_update_check->setChecked(UISettings::values.check_for_update_on_start);
    ui->toggle_auto_update->setChecked(UISettings::values.update_on_close);

    // The first item is "auto-select" with actual value -1, so plus one here will do the trick
    ui->region_combobox->setCurrentIndex(Settings::values.region_value + 1);
}

void ConfigureGeneral::PopulateHotkeyList(const HotkeyRegistry& registry) {
    ui->hotkeysDialog->Populate(registry);
}

void ConfigureGeneral::ResetDefaults() {
    QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("Citra"),
        tr("Are you sure you want to <b>reset your settings</b> and close Citra?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (answer == QMessageBox::No)
        return;

    FileUtil::Delete(FileUtil::GetUserPath(FileUtil::UserPath::ConfigDir) + "qt-config.ini");
    std::exit(0);
}

void ConfigureGeneral::applyConfiguration() {
    UISettings::values.confirm_before_closing = ui->toggle_check_exit->isChecked();

    UISettings::values.check_for_update_on_start = ui->toggle_update_check->isChecked();
    UISettings::values.update_on_close = ui->toggle_auto_update->isChecked();

    Settings::values.region_value = ui->region_combobox->currentIndex() - 1;

    Settings::values.is_new_3ds = ui->toggle_is_new_3ds->isChecked();
    Settings::values.use_priority_boost = ui->toggle_use_priority_boost->isChecked();
    Settings::values.use_force_indexed = ui->toggle_use_force_indexed->isChecked();
    sdmc_dir_changed = Settings::values.sdmc_dir != ui->sdmc_dir->text().toStdString();
    Settings::values.sdmc_dir = ui->sdmc_dir->text().toStdString();

}

void ConfigureGeneral::retranslateUi() {
    ui->retranslateUi(this);
    ui->hotkeysDialog->retranslateUi();
}
