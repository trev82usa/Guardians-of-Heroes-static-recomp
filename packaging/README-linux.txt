Guardian Heroes (Xbox 360 XBLA) - native build (ReXGlue static recompilation)
Personal use only, from your own copy of the game.

1. Install your game files (once):
   Windows: double-click GuardianHeroes-Installer.exe
   Linux:   run ./install_game.py   (window needs python3-tk: sudo apt install python3-tk;
            without it, it asks for the paths in the terminal)
   Pick your Guardian Heroes package file (Content/0000000000000000/58410B2B/000D0000/<long name>)
   and press Install. It fills the "assets" folder and checks the game version.
   If it says your copy needs the title update, also pick the title update
   (58410B2B/000B0000/<file>, or a default.xexp) and xextool.exe, and press Install again.
2. Linux: run ./guardianheroes   (needs Vulkan: Mesa RADV on AMD is fine)
   Windows: run guardianheroes.exe (needs a DirectX 12 capable GPU)
   If the game files are missing or the wrong version, the game says so and closes.

F1 (or LB+RB+X on a controller, Q+E+X on the keyboard) = settings menu:
   resolution scale, fullscreen, vsync, frame limiter, black bars, texture filtering,
   mute, keyboard and player options. Saved to guardianheroes.toml.
F3 = FPS overlay. F4 = every advanced engine setting.

Players: each controller is its own player (up to 4 co-op); the keyboard is player 1.
  (F1 > Controls can route everything to player 1 instead, e.g. for Steam Input.)

Keyboard: arrows/WASD = move, Enter/Space/K = A, Backspace/L = B, X/J = X, Y/I = Y,
  Escape/P = Start, Tab/R = Back, Q/E = LB/RB, Z/C = LT/RT.

If anything goes wrong, keep crash.txt (if present) and the newest file in "logs".
