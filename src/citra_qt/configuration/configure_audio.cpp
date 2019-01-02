// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include <SDL.h>
#include "audio_core/sink.h"
#include "audio_core/sink_details.h"
#include "citra_qt/configuration/configure_audio.h"
#include "core/core.h"
#include "core/settings.h"
#include "ui_configure_audio.h"

ConfigureAudio::ConfigureAudio(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureAudio>()) {
    ui->setupUi(this);

    ui->output_sink_combo_box->clear();
    ui->output_sink_combo_box->addItem("auto");
    for (const char* id : AudioCore::GetSinkIDs()) {
        ui->output_sink_combo_box->addItem(id);
    }

    ui->emulation_combo_box->addItem(tr("HLE (fast)"));
    ui->emulation_combo_box->addItem(tr("LLE (accurate)"));
    ui->emulation_combo_box->addItem(tr("LLE multi-core"));
    ui->emulation_combo_box->setEnabled(!Core::System::GetInstance().IsPoweredOn());

    ui->input_device_combo_box->setEnabled(Settings::values.enable_input_device);
    connect(ui->enable_input_device, &QCheckBox::stateChanged, ui->input_device_combo_box, &QComboBox::setEnabled);
    ui->input_device_combo_box->clear();
    ui->input_device_combo_box->addItem("auto");
    int num_devices{SDL_GetNumAudioDevices(1)};
    for (int i{}; i < num_devices; ++i) {
            ui->input_device_combo_box->addItem(SDL_GetAudioDeviceName(i, 1));
    }

    connect(ui->volume_slider, &QSlider::valueChanged, this,
            &ConfigureAudio::setVolumeIndicatorText);

    this->setConfiguration();
    connect(ui->output_sink_combo_box,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            &ConfigureAudio::updateAudioDevices);
}

ConfigureAudio::~ConfigureAudio() {}

void ConfigureAudio::setConfiguration() {
    setOutputSinkFromSinkID();

    // The device list cannot be pre-populated (nor listed) until the output sink is known.
    updateAudioDevices(ui->output_sink_combo_box->currentIndex());

    setAudioDeviceFromDeviceID();

    ui->enable_input_device->setChecked(Settings::values.enable_input_device);
    {
    int new_device_index = -1;
    for (int index = 0; index < ui->input_device_combo_box->count(); index++) {
        if (ui->input_device_combo_box->itemText(index).toStdString() ==
            Settings::values.input_device) {
            new_device_index = index;
            break;
        }
    }

    ui->input_device_combo_box->setCurrentIndex(new_device_index);
    }

    ui->toggle_audio_stretching->setChecked(Settings::values.enable_audio_stretching);
    ui->volume_slider->setValue(Settings::values.volume * ui->volume_slider->maximum());
    setVolumeIndicatorText(ui->volume_slider->sliderPosition());

    int selection;
    if (Settings::values.enable_dsp_lle) {
        if (Settings::values.enable_dsp_lle_multithread) {
            selection = 2;
        } else {
            selection = 1;
        }
    } else {
        selection = 0;
    }
    ui->emulation_combo_box->setCurrentIndex(selection);
}

void ConfigureAudio::setOutputSinkFromSinkID() {
    int new_sink_index = 0;

    const QString sink_id = QString::fromStdString(Settings::values.sink_id);
    for (int index = 0; index < ui->output_sink_combo_box->count(); index++) {
        if (ui->output_sink_combo_box->itemText(index) == sink_id) {
            new_sink_index = index;
            break;
        }
    }

    ui->output_sink_combo_box->setCurrentIndex(new_sink_index);
}

void ConfigureAudio::setAudioDeviceFromDeviceID() {
    int new_device_index = -1;

    const QString device_id = QString::fromStdString(Settings::values.audio_device_id);
    for (int index = 0; index < ui->audio_device_combo_box->count(); index++) {
        if (ui->audio_device_combo_box->itemText(index) == device_id) {
            new_device_index = index;
            break;
        }
    }

    ui->audio_device_combo_box->setCurrentIndex(new_device_index);
}

void ConfigureAudio::setVolumeIndicatorText(int percentage) {
    ui->volume_indicator->setText(tr("%1%", "Volume percentage (e.g. 50%)").arg(percentage));
}

void ConfigureAudio::applyConfiguration() {
    Settings::values.sink_id =
        ui->output_sink_combo_box->itemText(ui->output_sink_combo_box->currentIndex())
            .toStdString();
    Settings::values.enable_audio_stretching = ui->toggle_audio_stretching->isChecked();
    Settings::values.audio_device_id =
        ui->audio_device_combo_box->itemText(ui->audio_device_combo_box->currentIndex())
            .toStdString();
    Settings::values.enable_input_device = ui->enable_input_device->isChecked();
    Settings::values.input_device = ui->input_device_combo_box->currentText().toStdString();
    Settings::values.volume =
        static_cast<float>(ui->volume_slider->sliderPosition()) / ui->volume_slider->maximum();
    Settings::values.enable_dsp_lle = ui->emulation_combo_box->currentIndex() != 0;
    Settings::values.enable_dsp_lle_multithread = ui->emulation_combo_box->currentIndex() == 2;
}

void ConfigureAudio::updateAudioDevices(int sink_index) {
    ui->audio_device_combo_box->clear();
    ui->audio_device_combo_box->addItem(AudioCore::auto_device_name);

    const std::string sink_id = ui->output_sink_combo_box->itemText(sink_index).toStdString();
    for (const auto& device : AudioCore::GetDeviceListForSink(sink_id)) {
        ui->audio_device_combo_box->addItem(QString::fromStdString(device));
    }
}

void ConfigureAudio::retranslateUi() {
    ui->retranslateUi(this);
}
