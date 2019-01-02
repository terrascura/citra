// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QColorDialog>
#ifdef __APPLE__
#include <QMessageBox>
#endif
#include "citra_qt/configuration/configure_graphics.h"
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_graphics.h"

ConfigureGraphics::ConfigureGraphics(QWidget* parent)
    : QWidget(parent), ui(new Ui::ConfigureGraphics) {

    ui->setupUi(this);
    this->setConfiguration();

    ui->toggle_glsync->setEnabled(!Core::System::GetInstance().IsPoweredOn());
    ui->frame_limit->setEnabled(Settings::values.use_frame_limit);
    connect(ui->toggle_frame_limit, &QCheckBox::stateChanged, ui->frame_limit,
            &QSpinBox::setEnabled);
    ui->AddTicks->setEnabled(Settings::values.FMV_hack);
    connect(ui->FMV_hack, &QCheckBox::stateChanged, ui->AddTicks, &QSpinBox::setEnabled);
    ui->screen_refresh_rate->setEnabled(Settings::values.custom_refresh_rate);
    connect(ui->custom_refresh_rate, &QCheckBox::stateChanged, ui->screen_refresh_rate, &QSpinBox::setEnabled);
    ui->clear_cache_secs->setEnabled(Settings::values.enable_cache_clear);
    connect(ui->enable_cache_clear, &QCheckBox::stateChanged, ui->clear_cache_secs, &QSpinBox::setEnabled);

    ui->custom_layout->setChecked(Settings::values.custom_layout);
    ui->layout_combobox->setEnabled(!Settings::values.custom_layout);
    ui->custom_layout_group->setVisible(Settings::values.custom_layout);
    ui->custom_top_left->setValue(Settings::values.custom_top_left);
    ui->custom_top_top->setValue(Settings::values.custom_top_top);
    ui->custom_top_right->setValue(Settings::values.custom_top_right);
    ui->custom_top_bottom->setValue(Settings::values.custom_top_bottom);
    ui->custom_bottom_left->setValue(Settings::values.custom_bottom_left);
    ui->custom_bottom_top->setValue(Settings::values.custom_bottom_top);
    ui->custom_bottom_right->setValue(Settings::values.custom_bottom_right);
    ui->custom_bottom_bottom->setValue(Settings::values.custom_bottom_bottom);

    ui->layoutBox->setEnabled(!Settings::values.custom_layout);

    ui->hw_renderer_group->setEnabled(ui->toggle_hw_renderer->isChecked());
    connect(ui->toggle_hw_renderer, &QCheckBox::stateChanged, ui->hw_renderer_group,
            &QWidget::setEnabled);
    ui->hw_shader_group->setEnabled(ui->toggle_hw_shader->isChecked());
    connect(ui->toggle_hw_shader, &QCheckBox::stateChanged, ui->hw_shader_group,
            &QWidget::setEnabled);
    connect(ui->custom_layout, &QCheckBox::stateChanged, ui->custom_layout_group,
            &QWidget::setVisible);
    connect(ui->custom_layout, &QCheckBox::stateChanged, ui->layout_combobox,
            &QWidget::setDisabled);
#ifdef __APPLE__
    connect(ui->toggle_hw_shader, &QCheckBox::stateChanged, this, [this](int state) {
        if (state == Qt::Checked) {
            QMessageBox::warning(
                this, tr("Hardware Shader Warning"),
                tr("Hardware Shader support is broken on macOS, and will cause graphical issues "
                   "like showing a black screen.<br><br>The option is only there for "
                   "test/development purposes. If you experience graphical issues with Hardware "
                   "Shader, please turn it off."));
        }
    });
#endif
    connect(ui->bg_button, &QPushButton::clicked, this, [this] {
        const QColor new_bg_color = QColorDialog::getColor(bg_color);
        if (!new_bg_color.isValid())
            return;
        bg_color = new_bg_color;
        ui->bg_button->setStyleSheet(
            QString("QPushButton { background-color: %1 }").arg(bg_color.name()));
    });
}

ConfigureGraphics::~ConfigureGraphics() = default;

void ConfigureGraphics::setConfiguration() {
    ui->toggle_hw_renderer->setChecked(Settings::values.use_hw_renderer);
    ui->toggle_hw_shader->setChecked(Settings::values.use_hw_shader);
    ui->toggle_accurate_gs->setChecked(Settings::values.shaders_accurate_gs);
    ui->toggle_accurate_mul->setChecked(Settings::values.shaders_accurate_mul);
    ui->toggle_shader_jit->setChecked(Settings::values.use_shader_jit);
    ui->resolution_factor_combobox->setCurrentIndex(Settings::values.resolution_factor);
    ui->toggle_glsync->setChecked(Settings::values.use_glsync);
    ui->toggle_frame_limit->setChecked(Settings::values.use_frame_limit);
    ui->frame_limit->setValue(Settings::values.frame_limit);
    ui->toggle_format_reinterpret_hack->setChecked(Settings::values.use_format_reinterpret_hack);
    ui->factor_3d->setValue(Settings::values.factor_3d);
    ui->toggle_3d->setChecked(Settings::values.toggle_3d);
    ui->layout_combobox->setCurrentIndex(static_cast<int>(Settings::values.layout_option));
    ui->swap_screen->setChecked(Settings::values.swap_screen);
    bg_color = QColor::fromRgbF(Settings::values.bg_red, Settings::values.bg_green,
                                Settings::values.bg_blue);
    ui->bg_button->setStyleSheet(
        QString("QPushButton { background-color: %1 }").arg(bg_color.name()));
    ui->FMV_hack->setChecked(Settings::values.FMV_hack);
    ui->AddTicks->setValue(Settings::values.AddTicks);
    ui->custom_refresh_rate->setChecked(Settings::values.custom_refresh_rate);
    ui->screen_refresh_rate->setValue(Settings::values.screen_refresh_rate);
    ui->enable_cache_clear->setChecked(Settings::values.enable_cache_clear);
    ui->clear_cache_secs->setValue(Settings::values.clear_cache_secs);
    ui->min_vertices_per_thread->setValue(Settings::values.min_vertices_per_thread);
}

void ConfigureGraphics::applyConfiguration() {
    Settings::values.use_hw_renderer = ui->toggle_hw_renderer->isChecked();
    Settings::values.use_hw_shader = ui->toggle_hw_shader->isChecked();
    Settings::values.shaders_accurate_gs = ui->toggle_accurate_gs->isChecked();
    Settings::values.shaders_accurate_mul = ui->toggle_accurate_mul->isChecked();
    Settings::values.use_shader_jit = ui->toggle_shader_jit->isChecked();
    Settings::values.resolution_factor =
        static_cast<u16>(ui->resolution_factor_combobox->currentIndex());
    Settings::values.use_glsync = ui->toggle_glsync->isChecked();
    Settings::values.use_frame_limit = ui->toggle_frame_limit->isChecked();
    Settings::values.frame_limit = ui->frame_limit->value();
    Settings::values.use_format_reinterpret_hack = ui->toggle_format_reinterpret_hack->isChecked();
    Settings::values.factor_3d = ui->factor_3d->value();
    Settings::values.toggle_3d = ui->toggle_3d->isChecked();
    Settings::values.layout_option =
        static_cast<Settings::LayoutOption>(ui->layout_combobox->currentIndex());
    Settings::values.swap_screen = ui->swap_screen->isChecked();
    Settings::values.bg_red = static_cast<float>(bg_color.redF());
    Settings::values.bg_green = static_cast<float>(bg_color.greenF());
    Settings::values.bg_blue = static_cast<float>(bg_color.blueF());
    Settings::values.FMV_hack = ui->FMV_hack->isChecked();
    Settings::values.AddTicks = ui->AddTicks->value();
    Settings::values.custom_refresh_rate = ui->custom_refresh_rate->isChecked();
    Settings::values.screen_refresh_rate = ui->screen_refresh_rate->value();
    Settings::values.enable_cache_clear = ui->enable_cache_clear->isChecked();
    Settings::values.clear_cache_secs = ui->clear_cache_secs->value();
    Settings::values.min_vertices_per_thread = ui->min_vertices_per_thread->value();
    Settings::values.custom_layout = ui->custom_layout->isChecked();
    Settings::values.custom_top_left = ui->custom_top_left->value();
    Settings::values.custom_top_top = ui->custom_top_top->value();
    Settings::values.custom_top_right = ui->custom_top_right->value();
    Settings::values.custom_top_bottom = ui->custom_top_bottom->value();
    Settings::values.custom_bottom_left = ui->custom_bottom_left->value();
    Settings::values.custom_bottom_top = ui->custom_bottom_top->value();
    Settings::values.custom_bottom_right = ui->custom_bottom_right->value();
    Settings::values.custom_bottom_bottom = ui->custom_bottom_bottom->value();
}

void ConfigureGraphics::retranslateUi() {
    ui->retranslateUi(this);
}
