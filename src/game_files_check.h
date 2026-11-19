// Checks the game files before the engine starts, so a missing or wrong default.xex gives a
// clear message instead of "No function registered at ..." or a silent crash.
//
// The program is recompiled from one exact build of the game: default.xex version 3 (the last
// Xbox Live Arcade release, package 1.0.306). An older copy that never got the update has
// different code, so nothing would line up. The installer (install_game.py /
// GuardianHeroes-Installer.exe) extracts the package and applies the title update.
#pragma once

#include "gh_version.h"

#include <rex/logging/api.h>
#include <rex/filesystem.h>
#include <rex/system.h>
#include <SDL3/SDL_messagebox.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace gh {

constexpr uint32_t kTitleId = 0x58410B2B;
constexpr uint32_t kRequiredVersion = GH_XEX_VERSION;  // default.xex version this build is made from

inline uint32_t Be32(const std::vector<uint8_t>& d, size_t o) {
  return o + 4 <= d.size() ? (uint32_t(d[o]) << 24) | (uint32_t(d[o + 1]) << 16) |
                                 (uint32_t(d[o + 2]) << 8) | d[o + 3]
                           : 0;
}

inline std::string VersionString(uint32_t v) {
  return std::to_string(v >> 28) + "." + std::to_string((v >> 24) & 0xF) + "." +
         std::to_string((v >> 8) & 0xFFFF) + "." + std::to_string(v & 0xFF);
}

// Empty string = files look right; otherwise a message for the player.
inline std::string CheckGameFiles(const std::filesystem::path& dir, uint32_t* found_version = nullptr) {
  const std::string install_hint =
      "\n\nRun the installer next to the game:\n"
      "  Windows: GuardianHeroes-Installer.exe\n"
      "  Linux: install_game.py\n"
      "It extracts your Guardian Heroes package into the assets folder\n"
      "and applies the title update if your copy needs it.";
  std::error_code ec;
  const auto xex = dir / "default.xex";
  if (!std::filesystem::exists(xex, ec))
    return "Game files not found.\n\nExpected default.xex and the media folder in:\n" +
           dir.string() + install_hint;
  if (!std::filesystem::exists(dir / "media", ec))
    return "The media folder is missing from:\n" + dir.string() +
           "\n\ndefault.xex was found, but the game also needs the media folder from the "
           "same package." + install_hint;

  std::ifstream f(xex, std::ios::binary);
  std::vector<uint8_t> d(0x10000);
  f.read(reinterpret_cast<char*>(d.data()), std::streamsize(d.size()));
  d.resize(size_t(f.gcount()));
  if (d.size() < 24 || d[0] != 'X' || d[1] != 'E' || d[2] != 'X' || d[3] != '2')
    return "default.xex in\n" + dir.string() + "\nis not an Xbox 360 executable." + install_hint;

  uint32_t version = 0, title = 0;
  const uint32_t count = Be32(d, 20);
  for (uint32_t i = 0; i < count && 24 + i * 8 + 8 <= d.size(); ++i) {
    if (Be32(d, 24 + i * 8) == 0x00040006) {  // execution info
      const uint32_t off = Be32(d, 24 + i * 8 + 4);
      version = Be32(d, off + 4);
      title = Be32(d, off + 12);
    }
  }
  if (found_version) *found_version = version;
  if (title != kTitleId)
    return "default.xex is not from Guardian Heroes (title ID 58410B2B)." + install_hint;
  if (version != kRequiredVersion)
    return "Your default.xex is version " + VersionString(version) +
           ", which this download doesn't support.\n\nSupported versions: 0.0.0.2 (the primary "
           "release)\nand 0.0.0.3 (the title update release).\n\nPlease open an issue "
           "on GitHub with the version number so it can be added." + install_hint;
  return {};
}

// The build for each game version, next to each other in the download:
//   0.0.0.2 -> guardianheroes(.exe)       0.0.0.3 -> guardianheroes-v3(.exe)
inline std::string ProgramForVersion(uint32_t v) {
  std::string name = (v == GH_XEX_VERSION || v == 2) ? "guardianheroes" : "guardianheroes-v" + std::to_string(v & 0xFF) +
                                                     (v >> 8 ? "-" + std::to_string(v >> 8) : "");
#if defined(_WIN32)
  name += ".exe";
#endif
  return name;
}

// If the game files are a version that another build in this folder is made for, start that
// build instead (same arguments) and return true. Players can start either program.
inline bool HandOffToMatchingBuild(uint32_t version) {
  if (std::getenv("GH_HANDED_OFF")) return false;  // never bounce back and forth
  const auto exe = rex::filesystem::GetExecutableFolder() / ProgramForVersion(version);
  std::error_code ec;
  if (!std::filesystem::exists(exe, ec)) return false;
  REXLOG_WARN("[files] default.xex is version {}; starting {}", VersionString(version),
              exe.filename().string());
  rex::FlushLogging();
#if defined(_WIN32)
  _putenv("GH_HANDED_OFF=1");
  std::wstring cmd = L"\"" + exe.wstring() + L"\"";
  // Keep the original arguments: everything after the program name on our command line.
  const wchar_t* line = GetCommandLineW();
  bool quoted = false;
  for (; *line; ++line) {
    if (*line == L'"') quoted = !quoted;
    else if (*line == L' ' && !quoted) break;
  }
  cmd += line;
  STARTUPINFOW si{sizeof(si)};
  PROCESS_INFORMATION pi{};
  if (!CreateProcessW(exe.wstring().c_str(), cmd.data(), nullptr, nullptr, FALSE, 0, nullptr,
                      nullptr, &si, &pi))
    return false;
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  std::_Exit(0);
#else
  setenv("GH_HANDED_OFF", "1", 1);
  std::vector<std::string> args;
  if (std::ifstream cl("/proc/self/cmdline", std::ios::binary); cl) {
    std::string a;
    while (std::getline(cl, a, '\0')) args.push_back(a);
  }
  if (args.empty()) args.push_back(exe.string());
  args[0] = exe.string();
  std::vector<char*> argv;
  for (auto& a : args) argv.push_back(a.data());
  argv.push_back(nullptr);
  execv(exe.c_str(), argv.data());
  return false;  // execv only returns on failure
#endif
}

// Shows the problem and exits. GH_SKIP_FILE_CHECK=1 skips it (for testing other builds).
inline void RequireGameFiles(const std::filesystem::path& dir) {
  if (std::getenv("GH_SKIP_FILE_CHECK")) return;
  uint32_t found = 0;
  const std::string problem = CheckGameFiles(dir, &found);
  if (problem.empty()) return;
  if (found && found != kRequiredVersion && HandOffToMatchingBuild(found)) return;
  REXLOG_ERROR("[files] {}", problem);
  rex::FlushLogging();
  // A real popup on every platform (SDL needs no initialisation for this); the runtime's own
  // message box only prints to the terminal on Linux.
  if (!SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Guardian Heroes", problem.c_str(), nullptr))
    rex::ShowSimpleMessageBox(rex::SimpleMessageBoxType::Error, "Guardian Heroes\n\n" + problem);
  std::_Exit(1);
}

}  // namespace gh
