// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include "common/logging/log.h"
#include "core/settings.h"
#include "video_core/pica.h"
#include "video_core/renderer_base.h"
#include "video_core/renderer_opengl/renderer_opengl.h"
#include "video_core/video_core.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Video Core namespace

namespace VideoCore {

std::unique_ptr<RendererBase> g_renderer; ///< Renderer plugin

std::atomic<bool> g_hw_renderer_enabled;
std::atomic<bool> g_shader_jit_enabled;
std::atomic<bool> g_hw_shader_enabled;
std::atomic<bool> g_hw_shader_accurate_gs;
std::atomic<bool> g_hw_shader_accurate_mul;
std::atomic<bool> g_renderer_bg_color_update_requested;
std::atomic<bool> g_use_format_reinterpret_hack;
// Screenshot
std::atomic<bool> g_renderer_screenshot_requested;
void* g_screenshot_bits;
std::function<void()> g_screenshot_complete_callback;
Layout::FramebufferLayout g_screenshot_framebuffer_layout;

Memory::MemorySystem* g_memory;

/// Initialize the video core
Core::System::ResultStatus Init(EmuWindow& emu_window, Memory::MemorySystem& memory) {
    g_memory = &memory;
    Pica::Init();

    g_renderer = std::make_unique<OpenGL::RendererOpenGL>(emu_window);
    Core::System::ResultStatus result = g_renderer->Init();

    if (result != Core::System::ResultStatus::Success) {
        LOG_ERROR(Render, "initialization failed !");
    } else {
        LOG_DEBUG(Render, "initialized OK");
    }

    return result;
}

/// Shutdown the video core
void Shutdown() {
    Pica::Shutdown();
    if (Settings::values.use_glsync) {
        ReleaseSyncGPU(0);
    }

    g_renderer.reset();

    LOG_DEBUG(Render, "shutdown OK");
}

// GL Sync
static std::array<GLsync, 2> syncObject{};
static std::array<bool, 2> is_sync{};
void LockSyncGPU(int idx) {
    if (is_sync[idx]) return;
    syncObject[idx] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    is_sync[idx] = true;
}
void ReleaseSyncGPU(int idx) {
    if (is_sync[idx]) {
        glWaitSync(syncObject[idx], 0, GL_TIMEOUT_IGNORED);
        glDeleteSync(syncObject[idx]);
        is_sync[idx] = false;
    }
}
bool WaitSyncGPU(int idx) {
    if (is_sync[idx]) {
        GLenum sync_state = glClientWaitSync(syncObject[idx], 0, 0); // no wait
        if (sync_state == GL_ALREADY_SIGNALED || sync_state == GL_CONDITION_SATISFIED) {
            ReleaseSyncGPU(idx);
            return true;
        }
        if (sync_state == GL_WAIT_FAILED) {
            LOG_CRITICAL(Render_OpenGL, "GPU is GL_WAIT_FAILED");
            ReleaseSyncGPU(idx);
            return true;
        }
        return false;
    }
    return true;
}

void RequestScreenshot(void* data, std::function<void()> callback,
                       const Layout::FramebufferLayout& layout) {
    if (g_renderer_screenshot_requested) {
        LOG_ERROR(Render, "A screenshot is already requested or in progress, ignoring the request");
        return;
    }
    g_screenshot_bits = data;
    g_screenshot_complete_callback = std::move(callback);
    g_screenshot_framebuffer_layout = layout;
    g_renderer_screenshot_requested = true;
}

u16 GetResolutionScaleFactor() {
    if (g_hw_renderer_enabled) {
        return !Settings::values.resolution_factor
                   ? g_renderer->GetRenderWindow().GetFramebufferLayout().GetScalingRatio()
                   : Settings::values.resolution_factor;
    } else {
        // Software renderer always render at native resolution
        return 1;
    }
}

} // namespace VideoCore
