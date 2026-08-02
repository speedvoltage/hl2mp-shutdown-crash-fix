# HL2MP Shutdown Fix 1.2.1

Linux x86-64 Metamod:Source plugin for the Half-Life 2: Deathmatch dedicated-server shutdown crash.

## What this version fixes

The x64 `dedicated_srv.so` contains an unversioned relocation for `__gxx_personality_v0`. The x64 `libsteam_api.so` exports its own unversioned implementation of that C++ ABI routine and is loaded before `libstdc++.so.6`. The dynamic loader therefore binds the dedicated console's exception-handling metadata to Valve's Steam copy.

When `CTextConsoleUnix::ShutDown()` calls `pthread_cancel()` on the libedit thread, glibc begins `_Unwind_ForcedUnwind`. The incorrectly bound Steam personality routine handles that unwind and calls `abort()`.

Version 1.2.1 corrects only the affected relocation in the loaded `dedicated_srv.so`:

- Old target: `libsteam_api.so::__gxx_personality_v0`
- New target: `libstdc++.so.6::__gxx_personality_v0`

## What it does not do

- It does not hook `CGameServer::Shutdown()`.
- It does not hook `CSteam3Server::Shutdown()`.
- It does not hook or suppress `CTextConsoleUnix::ShutDown()`.
- It does not call or suppress `pthread_cancel()`, `pthread_join()`, or `pthread_mutex_destroy()`.
- It does not use Funchook, SourceHook, `RTLD_NODELETE`, fixed offsets, or Steam callback draining.

Valve's original Steam and console shutdown paths run normally.

## Installation

```bash
cd /tmp
tar -xzf ~/Downloads/hl2mp-shutdown-fix-mms-x64-1.2.1.tar.gz
cd hl2mp-shutdown-fix-mms-x64-1.2.1
./verify.sh
sudo ./install.sh /home/user/Downloads/hl2dm-serverfiles/hl2mp
```

Restart SRCDS and run:

```text
meta list
```

Expected load output:

```text
[META] Loaded HL2MP Shutdown Fix
```

Then run:

```text
quit
```

A successful test ends with `Server Quit` and no abort. There are no v1.1 diagnostic commands in this release.

## SDK warning

Do not keep the old SDK detours that suppress `CSteam3Server::Shutdown()` or `CTextConsoleUnix::ShutDown()`. They hide whether the relocation correction works and are no longer part of the fix.

## Unloading

On unload, the plugin prints `[META] HL2MP Shutdown Fix unloaded, but retaining shutdown fix!`. The plugin leaves the corrected data pointer in `dedicated_srv.so` when manually unloaded. The pointer targets `libstdc++.so.6`, not plugin code, so no plugin image pinning is required. The correction disappears naturally when the process exits.

## Runtime guards

The plugin refuses to load unless:

- Metamod reports the HL2DM engine provider.
- The game directory is exactly `hl2mp`.
- `dedicated_srv.so` is an x86-64 ELF object with the expected dynamic relocation.
- The current relocation target is either the known bad Steam provider or the already-correct `libstdc++` provider.

An unexpected provider is not modified.

## Building

```bash
./build.sh /path/to/hl2dm-src
```

## Tests

```bash
HL2SDK=/path/to/hl2dm-src ./tests/run.sh
```


## Author

Peter Brev
