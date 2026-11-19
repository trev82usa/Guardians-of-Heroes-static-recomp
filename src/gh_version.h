// Per-version settings. The port is built once per game version (CMake sets GH_XEX_VERSION and
// the two hook addresses; see CMakeLists.txt and variants/). Defaults: 0.0.0.2.
#pragma once

#ifndef GH_XEX_VERSION
#define GH_XEX_VERSION 2               // default.xex "version" this build is recompiled from
#endif
#ifndef GH_PRESENT_FN                  // the game's present function (only caller of VdSwap)
#define GH_PRESENT_FN sub_825DE710
#endif
#ifndef GH_LAUNCH_FN                   // the game's XLaunchNewImage (only XamLoaderLaunchTitle caller)
#define GH_LAUNCH_FN sub_825CF5C8
#endif

#define GH_CAT2(a, b) a##b
#define GH_CAT(a, b) GH_CAT2(a, b)
#define GH_IMP(fn) GH_CAT(__imp__, fn)
#define GH_HOOK(fn) REX_HOOK_RAW(fn)
#define GH_EXTERN(fn) REX_EXTERN(fn)
