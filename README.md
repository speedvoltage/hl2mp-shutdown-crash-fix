# SRCDS Shutdown Fix 1.3.0

A Metamod:Source plugin that fixes a Linux x86-64 Source dedicated server crash during shutdown.

## The problem

On affected servers, running:

```text
quit
```

causes SRCDS to abort during cancellation of the dedicated console thread. The loaded `dedicated_srv.so` has resolved `__gxx_personality_v0` to the copy exported by `libsteam_api.so` instead of the implementation in `libstdc++.so.6`.

This plugin corrects that one resolved relocation and allows the server to exit normally.

## Requirements

* Linux x86-64
* A Source dedicated server using `dedicated_srv.so`
* Metamod:Source

Version 1.3.0 is not restricted to the HL2DM Metamod engine provider or the `hl2mp` game directory. It can load under any Metamod engine provider and game directory.

The plugin still refuses to modify anything unless it finds the expected x86-64 relocation in the loaded `dedicated_srv.so` and its current target is either the known bad `libsteam_api.so` provider or the already-correct `libstdc++.so.6` provider.

## Installation

Extract the release archive:

```bash
cd /tmp
tar -xzf ~/Downloads/srcds-shutdown-fix-mms-x64-1.3.0.tar.gz
cd srcds-shutdown-fix-mms-x64-1.3.0
```

Optionally verify the package:

```bash
./verify.sh
```

Install it into the game directory, for example:

```bash
sudo ./install.sh /home/user/servers/css-server/cstrike
```

Restart the server and check that the plugin loaded:

```text
meta list
```

You should see:

```text
[META] Loaded SRCDS Shutdown Fix
```

## Testing the fix

From the server console, run:

```text
quit
```

A successful shutdown ends with:

```text
Server Quit
```

The process should exit without aborting or crashing.

## Building

```bash
./build.sh /path/to/source-sdk-2013/src
```

## Tests

```bash
HL2SDK=/path/to/source-sdk-2013/src ./tests/run.sh
```

## Author

Peter Brev
