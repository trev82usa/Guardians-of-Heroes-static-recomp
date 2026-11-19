// Guardian Heroes - ReXGlue Recompiled Project
//
// App hooks, following the Trials HD port: F1 settings menu, F3 FPS counter (fed from the
// game's present function), keyboard controls, crash.txt, clean exit from the game's menu.

#pragma once

#include <rex/cvar.h>
#include <rex/audio/sdl/sdl_audio_system.h>
#include <rex/filesystem.h>
#include <rex/input/device_assignment.h>
#include <rex/input/input_system.h>
#include <rex/logging/api.h>
#include <rex/rex_app.h>
#include <rex/runtime.h>
#include <rex/ui/keybinds.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <thread>

#include "game_files_check.h"
#include "gh_menu.h"
#include "crash_handler.h"
#include "keyboard_input.h"
#include "perf_tools.h"

void StartTitleExitWatch();                      // game_exit.cpp
void RemoveSharedMemoryFiles(const char* when);  // game_exit.cpp

class GuardianHeroesApp : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<GuardianHeroesApp>(new GuardianHeroesApp(ctx, "guardianheroes",
        PPCImageConfig));
  }

  // Use the Xenos GPU backend unless --gpu_plugin says otherwise (no graphics otherwise).
  // GH_NO_GPU=1 runs headless (testing on machines without a GPU).
  void OnPreSetup(rex::RuntimeConfig& config) override {
    SetHighTimerResolution();
    RemoveSharedMemoryFiles("left over from earlier runs");
    if (!config.graphics && config.gpu_plugin.empty() && !std::getenv("GH_NO_GPU"))
      config.gpu_plugin = "xenos";
    if (!config.audio_factory)
      config.audio_factory = REX_AUDIO_BACKEND(rex::audio::sdl::SDLAudioSystem);
    rex::cvar::SetFlagByName("input_backend", "sdl");
    rex::cvar::SetFlagByName("audio_mute", "false");
    // Full game rather than the Xbox Live Arcade trial: report every license bit as granted to
    // XamContentGetLicenseMask (ReXGlue defaults to 0 = trial).
    rex::cvar::SetFlagByName("license_mask", "4294967295");
  }

  // Installed after setup so nothing in the runtime replaces it.
  void OnPostSetup() override {
    if (rex::cvar::Query<bool>("gh_crash_handler")) InstallCrashHandler();
    SetGuestFrameStats([] { return GetGuestFrameStats(); });  // F3 overlay
    StartProfilerIfEnabled();
    StartTitleExitWatch();
    RemoveSharedMemoryFiles("(this run's; freed automatically when the game closes)");
    // After the SDK set its own input rule (in ConstructRuntime), so ours replaces it.
    if (runtime() && runtime()->input_system()) {
      auto* input = static_cast<rex::input::InputSystem*>(runtime()->input_system());
      input->AddDriver(CreateKeyboardDriver(window()));  // before the callback is set
      // Up to 4 players: by default the SDK gives each pad its own player slot (first pad and
      // the keyboard = player 1). Optionally route everything to player 1 instead (e.g. when
      // Steam Input's virtual pad takes a slot).
      if (rex::cvar::Query<bool>("gh_all_controllers_player1"))
        input->SetDeviceAssignment(std::make_unique<rex::input::SharedAssignment>());
      ConnectGhMenuInput(input);
    }
  }

  // F1: the Guardian Heroes settings menu. Controller: LB+RB+X.
  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
    InitGhMenu(drawer, window(),
                rex::filesystem::GetExecutableFolder() / "guardianheroes.toml");
    rex::ui::RegisterBind("bind_gh_menu", "F1", "Toggle the Guardian Heroes settings menu",
                          [] { ToggleGhMenu(); });
  }

  // Closing the window runs the engine's full teardown, which can hang on a game thread that
  // never stops (the process then lingers invisibly). Give it a few seconds, then save the log
  // and end the process.
  bool OnWindowCloseRequested() override {
    StartExitWatchdog();
    return true;
  }
  void OnShutdown() override { StartExitWatchdog(); }

  static void StartExitWatchdog() {
    static std::once_flag once;
    std::call_once(once, [] {
      std::thread([] {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        rex::FlushLogging();
        std::_Exit(0);
      }).detach();
    });
  }

  // Find the game files without a command-line argument: an "assets" folder next to the
  // program, or default.xex placed right next to it.
  void OnConfigurePaths(rex::PathConfig& paths) override {
    if (paths.game_data_root.empty()) {  // --game_data_root given: respect it
      auto exe_dir = rex::filesystem::GetExecutableFolder();
      paths.game_data_root = exe_dir / "assets";  // so the message names the right folder
      for (const auto& dir : {exe_dir / "assets", exe_dir}) {
        std::error_code ec;
        if (std::filesystem::exists(dir / "default.xex", ec)) {
          paths.game_data_root = dir;
          break;
        }
      }
    }
    // Wrong or missing game files: say so clearly instead of crashing later.
    gh::RequireGameFiles(paths.game_data_root);
  }
};
