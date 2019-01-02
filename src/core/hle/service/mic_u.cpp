// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <SDL.h>
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/ipc.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/event.h"
#include "core/hle/kernel/handle_table.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/shared_memory.h"
#include "core/hle/service/mic_u.h"
#include "core/settings.h"

namespace Service::MIC {

MIC_U* current_mic{};

enum class Encoding : u8 {
    PCM8 = 0,
    PCM16 = 1,
    PCM8Signed = 2,
    PCM16Signed = 3,
};

enum class SampleRate : u8 {
    SampleRate32730 = 0,
    SampleRate16360 = 1,
    SampleRate10910 = 2,
    SampleRate8180 = 3
};

struct MIC_U::Impl {
    SDL_AudioDeviceID dev;

    explicit Impl(Core::System& system) {
        buffer_full_event =
            system.Kernel().CreateEvent(Kernel::ResetType::OneShot, "MIC_U::buffer_full_event");
    }

    void MapSharedMem(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x01, 1, 2};
        const u32 size = rp.Pop<u32>();
        shared_memory = rp.PopObject<Kernel::SharedMemory>();

        if (shared_memory) {
            shared_memory->SetName("MIC_U:shared_memory");
        }

        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        rb.Push(RESULT_SUCCESS);

        LOG_WARNING(Service_MIC, "called, size=0x{:X}", size);
    }

    void UnmapSharedMem(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x02, 0, 0};
        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        shared_memory = nullptr;
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_MIC, "called");
    }

    void StartSampling() {
        SDL_AudioSpec want{}, have;
        want.channels = 1;
        switch (encoding) {
        case Encoding::PCM16:
            want.format = AUDIO_U16;
            break;
        case Encoding::PCM16Signed:
            want.format = AUDIO_S16;
            break;
        case Encoding::PCM8:
            want.format = AUDIO_U8;
            break;
        case Encoding::PCM8Signed:
            want.format = AUDIO_S8;
            break;
        }
        want.samples = 1024;
        switch (sample_rate) {
        case SampleRate::SampleRate10910:
            want.freq = 10910;
            break;
        case SampleRate::SampleRate16360:
            want.freq = 16360;
            break;
        case SampleRate::SampleRate32730:
            want.freq = 32730;
            break;
        case SampleRate::SampleRate8180:
            want.freq = 8180;
            break;
        }
        want.userdata = this;
        want.callback = [](void* userdata, Uint8* data, int len) {
            Impl* impl{static_cast<Impl*>(userdata)};
            if (!impl) {
                return;
            }

            u8* buffer{impl->shared_memory->GetPointer()};
            if (!buffer) {
                return;
            }

            u32 offset;
            std::memcpy(&offset, buffer + impl->audio_buffer_size, sizeof(offset));

            // TODO: How does the 3DS handles looped input buffers
            if (len > impl->audio_buffer_size - offset) {
                offset = impl->audio_buffer_offset;
            }

            std::memcpy(buffer + offset, data, len);
            offset += len;
            std::memcpy(buffer + impl->audio_buffer_size, &offset, sizeof(offset));
        };
        dev = SDL_OpenAudioDevice(
            (Settings::values.input_device.empty() || Settings::values.input_device == "auto")
                ? NULL
                : Settings::values.input_device.c_str(),
            1, &want, &have, 0);
        if (dev == 0) {
            LOG_ERROR(Service_MIC, "Failed to open device: {}", SDL_GetError());
        } else {
            if (have.format != want.format) {
                LOG_WARNING(Service_MIC, "Format not supported");
            } else {
                SDL_PauseAudioDevice(dev, 0);
                is_sampling = true;
            }
        }
    }

    void StartSampling(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x03, 5, 0};

        encoding = rp.PopEnum<Encoding>();
        sample_rate = rp.PopEnum<SampleRate>();
        audio_buffer_offset = rp.Pop<u32>();
        audio_buffer_size = rp.Pop<u32>();
        audio_buffer_loop = rp.Pop<bool>();

        if (Settings::values.enable_input_device) {
            StartSampling();
        }
        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        rb.Push(RESULT_SUCCESS);
        is_sampling = true;
        LOG_WARNING(Service_MIC,
                    "(STUBBED) called, encoding={}, sample_rate={}, "
                    "audio_buffer_offset={}, audio_buffer_size={}, audio_buffer_loop={}",
                    static_cast<u32>(encoding), static_cast<u32>(sample_rate), audio_buffer_offset,
                    audio_buffer_size, audio_buffer_loop);
    }

    void AdjustSampling(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x04, 1, 0};
        sample_rate = rp.PopEnum<SampleRate>();
        if (Settings::values.enable_input_device) {
            StopSampling();
            StartSampling();
        }
        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_MIC, "(STUBBED) called, sample_rate={}", static_cast<u32>(sample_rate));
    }

    void StopSampling() {
        if (dev != 0) {
            SDL_CloseAudioDevice(dev);
            is_sampling = false;
        }
    }

    void StopSampling(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x05, 0, 0};
        if (Settings::values.enable_input_device) {
            StopSampling();
        }
        IPC::RequestBuilder rb{rp.MakeBuilder(1, 0)};
        rb.Push(RESULT_SUCCESS);
        is_sampling = false;
        LOG_DEBUG(Service_MIC, "called");
    }

    void IsSampling(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x06, 0, 0};
        IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
        rb.Push(RESULT_SUCCESS);
        rb.Push<bool>(is_sampling);
        LOG_WARNING(Service_MIC, "(STUBBED) called");
    }

    void GetBufferFullEvent(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x07, 0, 0};
        IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
        rb.Push(RESULT_SUCCESS);
        rb.PushCopyObjects(buffer_full_event);
        LOG_WARNING(Service_MIC, "(STUBBED) called");
    }

    void SetGain(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x08, 1, 0};
        mic_gain = rp.Pop<u8>();

        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_MIC, "(STUBBED) called, mic_gain={}", mic_gain);
    }

    void GetGain(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x09, 0, 0};

        IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
        rb.Push(RESULT_SUCCESS);
        rb.Push<u8>(mic_gain);
        LOG_WARNING(Service_MIC, "(STUBBED) called");
    }

    void SetPower(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x0A, 1, 0};
        mic_power = rp.Pop<bool>();

        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_MIC, "(STUBBED) called, mic_power={}", mic_power);
    }

    void GetPower(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x0B, 0, 0};
        IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
        rb.Push(RESULT_SUCCESS);
        rb.Push<u8>(mic_power);
        LOG_WARNING(Service_MIC, "(STUBBED) called");
    }

    void SetIirFilterMic(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x0C, 1, 2};
        const u32 size = rp.Pop<u32>();
        const Kernel::MappedBuffer& buffer = rp.PopMappedBuffer();

        IPC::RequestBuilder rb = rp.MakeBuilder(1, 2);
        rb.Push(RESULT_SUCCESS);
        rb.PushMappedBuffer(buffer);
        LOG_WARNING(Service_MIC, "(STUBBED) called, size=0x{:X}, buffer=0x{:08X}", size,
                    buffer.GetId());
    }

    void SetClamp(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x0D, 1, 0};
        clamp = rp.Pop<bool>();

        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_MIC, "(STUBBED) called, clamp={}", clamp);
    }

    void GetClamp(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x0E, 0, 0};
        IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
        rb.Push(RESULT_SUCCESS);
        rb.Push<bool>(clamp);
        LOG_WARNING(Service_MIC, "(STUBBED) called");
    }

    void SetAllowShellClosed(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x0F, 1, 0};
        allow_shell_closed = rp.Pop<bool>();

        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        rb.Push(RESULT_SUCCESS);
        LOG_WARNING(Service_MIC, "(STUBBED) called, allow_shell_closed={}", allow_shell_closed);
    }

    void SetClientVersion(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx, 0x10, 1, 0};

        const u32 version = rp.Pop<u32>();
        LOG_WARNING(Service_MIC, "(STUBBED) called, version: 0x{:08X}", version);

        IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
        rb.Push(RESULT_SUCCESS);
    }

    void ReloadDevice() {
        if (is_sampling) {
            StopSampling();
            StartSampling();
        }
    }

    u32 client_version = 0;
    Kernel::SharedPtr<Kernel::Event> buffer_full_event;
    Kernel::SharedPtr<Kernel::SharedMemory> shared_memory;
    u8 mic_gain = 0;
    bool mic_power = false;
    bool is_sampling = false;
    bool allow_shell_closed;
    bool clamp = false;
    Encoding encoding = Encoding::PCM8;
    SampleRate sample_rate = SampleRate::SampleRate32730;
    s32 audio_buffer_offset = 0;
    u32 audio_buffer_size = 0;
    bool audio_buffer_loop = false;
};

void MIC_U::MapSharedMem(Kernel::HLERequestContext& ctx) {
    impl->MapSharedMem(ctx);
}

void MIC_U::UnmapSharedMem(Kernel::HLERequestContext& ctx) {
    impl->UnmapSharedMem(ctx);
}

void MIC_U::StartSampling(Kernel::HLERequestContext& ctx) {
    impl->StartSampling(ctx);
}

void MIC_U::AdjustSampling(Kernel::HLERequestContext& ctx) {
    impl->AdjustSampling(ctx);
}

void MIC_U::StopSampling(Kernel::HLERequestContext& ctx) {
    impl->StopSampling(ctx);
}

void MIC_U::IsSampling(Kernel::HLERequestContext& ctx) {
    impl->IsSampling(ctx);
}

void MIC_U::GetBufferFullEvent(Kernel::HLERequestContext& ctx) {
    impl->GetBufferFullEvent(ctx);
}

void MIC_U::SetGain(Kernel::HLERequestContext& ctx) {
    impl->SetGain(ctx);
}

void MIC_U::GetGain(Kernel::HLERequestContext& ctx) {
    impl->GetGain(ctx);
}

void MIC_U::SetPower(Kernel::HLERequestContext& ctx) {
    impl->SetPower(ctx);
}

void MIC_U::GetPower(Kernel::HLERequestContext& ctx) {
    impl->GetPower(ctx);
}

void MIC_U::SetIirFilterMic(Kernel::HLERequestContext& ctx) {
    impl->SetIirFilterMic(ctx);
}

void MIC_U::SetClamp(Kernel::HLERequestContext& ctx) {
    impl->SetClamp(ctx);
}

void MIC_U::GetClamp(Kernel::HLERequestContext& ctx) {
    impl->GetClamp(ctx);
}

void MIC_U::SetAllowShellClosed(Kernel::HLERequestContext& ctx) {
    impl->SetAllowShellClosed(ctx);
}

void MIC_U::SetClientVersion(Kernel::HLERequestContext& ctx) {
    impl->SetClientVersion(ctx);
}

MIC_U::MIC_U(Core::System& system)
    : ServiceFramework{"mic:u", 1}, impl{std::make_unique<Impl>(system)} {
    static const FunctionInfo functions[] = {
        {0x00010042, &MIC_U::MapSharedMem, "MapSharedMem"},
        {0x00020000, &MIC_U::UnmapSharedMem, "UnmapSharedMem"},
        {0x00030140, &MIC_U::StartSampling, "StartSampling"},
        {0x00040040, &MIC_U::AdjustSampling, "AdjustSampling"},
        {0x00050000, &MIC_U::StopSampling, "StopSampling"},
        {0x00060000, &MIC_U::IsSampling, "IsSampling"},
        {0x00070000, &MIC_U::GetBufferFullEvent, "GetBufferFullEvent"},
        {0x00080040, &MIC_U::SetGain, "SetGain"},
        {0x00090000, &MIC_U::GetGain, "GetGain"},
        {0x000A0040, &MIC_U::SetPower, "SetPower"},
        {0x000B0000, &MIC_U::GetPower, "GetPower"},
        {0x000C0042, &MIC_U::SetIirFilterMic, "SetIirFilterMic"},
        {0x000D0040, &MIC_U::SetClamp, "SetClamp"},
        {0x000E0000, &MIC_U::GetClamp, "GetClamp"},
        {0x000F0040, &MIC_U::SetAllowShellClosed, "SetAllowShellClosed"},
        {0x00100040, &MIC_U::SetClientVersion, "SetClientVersion"},
    };

    RegisterHandlers(functions);

    current_mic = this;
}

MIC_U::~MIC_U() {}

void MIC_U::ReloadDevice() {
    impl->ReloadDevice();
}

void InstallInterfaces(Core::System& system) {
    auto& service_manager = system.ServiceManager();
    std::make_shared<MIC_U>(system)->InstallAsService(service_manager);
}

void ReloadDevice() {
    if (!current_mic)
        return;
    current_mic->ReloadDevice();
}

} // namespace Service::MIC
