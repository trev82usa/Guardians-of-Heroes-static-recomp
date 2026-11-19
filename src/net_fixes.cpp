// Socket fixes for Linux.
//
// The game sets its (system link) sockets non-blocking with Windows' ioctlsocket(FIONBIO). On
// Windows the SDK hands that to Winsock unchanged, but on Linux it passes the Windows command
// number straight to ioctl(), where it means nothing: the call fails, the socket stays blocking, and the
// game's main thread waits forever in recvfrom during boot (as in the Simpsons Arcade port).
// This hook translates FIONBIO / FIONREAD; anything else goes to the SDK.
#include <rex/hook.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/xsocket.h>
#include <rex/system/xthread.h>

#include <atomic>
#include <cstdint>
#include <cstring>

#if defined(__linux__)
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/ioctl.h>

namespace {

PPCFunc* Sdk(const char* symbol) { return reinterpret_cast<PPCFunc*>(dlsym(RTLD_NEXT, symbol)); }

uint32_t LoadBE32(const uint8_t* p) {
  return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
}
void StoreBE32(uint8_t* p, uint32_t v) {
  p[0] = uint8_t(v >> 24); p[1] = uint8_t(v >> 16); p[2] = uint8_t(v >> 8); p[3] = uint8_t(v);
}

// Native fd for a guest socket handle, or -1.
int NativeFd(uint32_t handle) {
  auto socket = REX_KERNEL_OBJECTS()->LookupObject<rex::system::XSocket>(handle);
  return socket ? static_cast<int>(socket->native_handle()) : -1;
}

constexpr uint32_t kWsaENOTSOCK = 0x2736, kWsaEINVAL = 0x2726;
constexpr uint32_t kFIONBIO = 0x8004667E, kFIONREAD = 0x4004667F;

void LogOnce(std::atomic<int>& n, const char* what, uint32_t a, uint32_t b) {
  if (n.fetch_add(1) < 6) REXLOG_INFO("[net] {} {:X} {:X}", what, a, b);
}

}  // namespace

// ioctlsocket(caller, socket, cmd, u_long* arg)
REX_HOOK_RAW(__imp__NetDll_ioctlsocket) {
  static PPCFunc* sdk = Sdk("__imp__NetDll_ioctlsocket");
  static std::atomic<int> logged{0};
  const uint32_t handle = ctx.r4.u32, cmd = ctx.r5.u32, arg = ctx.r6.u32;
  if (cmd == kFIONBIO || cmd == kFIONREAD) {
    int fd = NativeFd(handle);
    if (fd < 0) {
      rex::system::XThread::SetLastError(kWsaENOTSOCK);
      ctx.r3.u64 = uint64_t(-1);
      return;
    }
    uint8_t* p = arg ? base + arg : nullptr;
    int ok = -1;
    if (cmd == kFIONBIO && p) {
      int flags = fcntl(fd, F_GETFL, 0);
      ok = fcntl(fd, F_SETFL, LoadBE32(p) ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
    } else if (cmd == kFIONREAD && p) {
      int avail = 0;
      ok = ioctl(fd, FIONREAD, &avail);
      if (ok == 0) StoreBE32(p, uint32_t(avail));
    }
    LogOnce(logged, cmd == kFIONBIO ? "ioctlsocket FIONBIO" : "ioctlsocket FIONREAD", handle,
            p ? LoadBE32(p) : 0);
    if (ok < 0) rex::system::XThread::SetLastError(kWsaEINVAL);
    ctx.r3.u64 = ok < 0 ? uint64_t(-1) : 0;
    return;
  }
  if (sdk) sdk(ctx, base); else ctx.r3.u64 = uint64_t(-1);
}
static rex::ppc::detail::PPCFuncRegistrar _gh_net_ioctl("__imp__NetDll_ioctlsocket",
                                                          &__imp__NetDll_ioctlsocket);

#endif  // __linux__
