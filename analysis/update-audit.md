# Supplied binary and loader audit

## Confirmed release defect

The plugin downloaded from release `v1.3` matches the repository's packaged binary:

```text
3b11ee313f78c3c2e205142f2fb0b3729fa446978ee22fe059a0dadaa347a61c
```

It exports `CreateInterface_MMS` and `UnloadInterface_MMS`, but not `CreateInterface`.

Metamod's loader API 2.0 passes a version structure containing `sh_iface` and `sh_impl` before the plugin API range. Loader API 2.1 removes those fields. The old plugin reads the wrong fields when presented with the new structure and rejects the advanced factory call. Metamod then tries the standard factory and reports `Function CreateInterface not found`.

This failure was reproduced using the released binary and an adapter compiled against the actual upstream API 18 header. Version 1.3.1 reads the layout selected by the loader API version, negotiates plugin APIs 16, 17, or 18, and exports the standard factory. It rejects unsupported loader versions and plugin ranges.

References: [current version structure](https://github.com/alliedmodders/metamod-source/blob/7e24ce9e7a03bfeb5c8ab1e4dd55d5d5747f3d33/core/ISmmPluginExt.h), [loader fallback](https://github.com/alliedmodders/metamod-source/blob/7e24ce9e7a03bfeb5c8ab1e4dd55d5d5747f3d33/core/metamod_plugins.cpp), and [legacy version structure](https://github.com/alliedmodders/metamod-source/blob/9fd977df/core/ISmmPluginExt.h).

## Supplied dedicated modules

All four are Linux x86-64 ELF shared libraries. Both the plugin's file-based resolver and the native launcher's dynamic-metadata resolver locate one writable, aligned `R_X86_64_64` relocation for `__gxx_personality_v0`, with zero addend, at `0x266ea8`. The slot is outside GNU RELRO in every file. The offset is an audit result; neither implementation hardcodes it.

| Supplied game | Dedicated SHA-256 | Relocation |
| --- | --- | --- |
| CS:S | `ebfff454e9c8e77bdced91b1625c575a0e9f08d579bacdb8f5259906cf88db1e` | `0x266ea8` |
| DoD:S | `18aba9090ce0ff0d53a25e1a7e33c2e94900b8b99cbf3b655b81d6928bbeb431` | `0x266ea8` |
| HL2DM | `18aba9090ce0ff0d53a25e1a7e33c2e94900b8b99cbf3b655b81d6928bbeb431` | `0x266ea8` |
| TF2 | `18aba9090ce0ff0d53a25e1a7e33c2e94900b8b99cbf3b655b81d6928bbeb431` | `0x266ea8` |

The four supplied Steam API libraries are identical, with SHA-256 `c0cc3d2802e5f2463bfa0046c41d2f65a6335baaeefbba6c7dbd5681d5ca7c46` and build ID `cabfeba9268918058d43dc9d95d8f6a170fad2e8`. They still export the unversioned C++ personality symbol. All dedicated modules still list Steam API before libstdc++ in their dependencies.

The supplied CS:S, DoD:S, and HL2DM engine binaries are identical. TF2's engine differs in build metadata and timestamps but has the same `.text` section. This plugin uses no engine vtable offsets or signatures, so there are none to update for this correction.

These findings apply to the uploaded files. They do not establish which build Steam currently installs or which binary another operator has loaded.

## Native launcher

The supplied `srcds_linux64` loads `libtier0_srv.so`, `libvstdlib_srv.so`, and `dedicated_srv.so` from `bin/linux64`, then calls `int DedicatedMain(int, char **)`. All four dedicated modules export that entry point with the same observed calling convention. One generic native launcher can follow this sequence and apply the correction before server startup.

The implementation reads loaded ELF dynamic relocation metadata, validates the writable slot and its provider, and retains a local reference to libstdc++ until after the dedicated module unloads. It does not rebuild `dedicated_srv.so` or alter library files on disk.

## Validation

- The released plugin reproduces the reported factory error with the upstream modern version structure.
- The corrected plugin passes classic-factory and API 16, 17, and 18 load/unload/reload tests. Unsupported API 19 is rejected.
- Independent adapters built against upstream legacy and modern headers negotiate APIs 16 and 18 successfully.
- The native launcher's resolver accepts all four supplied dedicated modules using mapped ELF segments, without executing those modules.
- A separate C++ module reproduces a `SIGABRT` on pthread cancellation when linked against the supplied Steam API. The native launcher corrects its personality binding, after which cancellation runs the cleanup destructor and `pthread_join` succeeds.
- Native launcher tests cover argument boundaries, exit status, root and game selection, library and certificate paths, missing libraries and entry points, unexpected providers, already-correct bindings, and absent section headers.
- Builds use GCC 13 on Ubuntu 24.04. The distributed binaries retain stack protection and require GLIBC symbols through 2.4. libdl is an explicit dependency for hosts predating its merge into libc.

Full SRCDS installations and their supporting libraries are not available in this workspace. Complete game startup, Metamod operation inside a running game, interactive terminal handling, and actual game shutdown remain to be tested on the target hosts.

## Shutdown hang

The screenshot does not identify a blocked thread. The plugin's unload function prints the message and returns; it leaves a pointer to libstdc++ in the dedicated module, not a pointer into the unloaded plugin.

The supplied `CTextConsoleUnix::ShutDown` calls `pthread_cancel`, then `pthread_join` without a timeout. This is a plausible waiting point, but the screenshot does not prove it is where the reported process stops. Steam's preceding assertions do not establish the cause either.

On a stuck server, first identify the actual server PID rather than its restart script:

```bash
ps -eo pid,ppid,stat,wchan:32,args | rg 'srcds_linux64|srcds-launcher|srcds_run_64'
```

Then replace `PID` below with that process ID and capture all thread stacks:

```bash
gdb -q -nx -batch -p PID -ex 'set pagination off' -ex 'thread apply all bt' -ex detach > shutdown-backtrace.txt 2>&1
```

Attach as the server user, or use `sudo` if the host's ptrace policy requires it. GDB pauses the process while collecting stacks and detaches afterward. Include the console output, `meta version`, `meta list`, the stop method (`quit`, Ctrl+C, SIGTERM, or service stop), and whether the remaining process is the server or wrapper. No shutdown watchdog is included: a timeout would not explain or repair the blocked cleanup.
