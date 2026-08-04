# Root-cause analysis

## GDB result

The aborting thread is the `libedit` console thread. Its stack is:

```text
abort
libsteam_api.so
libsteam_api.so!__gxx_personality_v0
libgcc_s.so.1
_Unwind_ForcedUnwind
__pthread_unwind
pthread cancellation handler
read
read_char
el_getc
el_gets
editline_threadproc
```

The abort occurs during forced unwinding immediately after a successful `pthread_cancel()`. The cleanup handler's `el_end()` is not reached in the captured stack.

## x64 binary relationship

The affected x64 `libsteam_api.so`:

```text
Build ID: cabfeba9268918058d43dc9d95d8f6a170fad2e8
GLOBAL DEFAULT __gxx_personality_v0
```

The affected x64 `dedicated_srv.so`:

```text
Build ID: d81f696009ba749ac36fc9e88d43385f1effdcd3
R_X86_64_64 __gxx_personality_v0
Relocation offset: 0x266ea8
Symbol version: unversioned
```

Because `dedicated_srv.so` lists `libsteam_api.so` before `libstdc++.so.6` in `DT_NEEDED`, the unversioned relocation binds to the Steam implementation.

## x86 comparison

The supplied x86 `libsteam_api.so` does not export `__gxx_personality_v0`. The x86 `dedicated_srv.so` requests:

```text
__gxx_personality_v0@CXXABI_1.3
```

It therefore binds to `libstdc++.so.6` and does not reproduce this cancellation failure.

## Proper upstream fixes

Any of these would solve the defect at its source:

1. Hide or remove the bundled C++ ABI exports from the x64 `libsteam_api.so`.
2. Version the x64 `dedicated_srv.so` reference as `__gxx_personality_v0@CXXABI_1.3`.
3. Link the x64 components so the dedicated relocation resolves to `libstdc++.so.6`.

The plugin implements the narrow runtime equivalent of option 2 by changing only the dedicated module's resolved relocation slot.

## Game-provider scope

The defect and runtime correction are in the shared Linux x86-64 dedicated launcher module, not in HL2DM GameDLL code. Version 1.3.0 therefore removes the Metamod engine-provider and game-directory restrictions. Runtime relocation and provider validation remain authoritative.
