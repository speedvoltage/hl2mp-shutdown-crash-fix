# HL2MP Shutdown Fix 1.2.1

A Metamod:Source plugin that fixes a Linux Half-Life 2: Deathmatch server crash during shutdown.

## The problem

On some Linux x86-64 HL2DM servers, running:

```text
quit
```

causes SRCDS to abort or crash instead of shutting down normally.

This plugin corrects the underlying problem and allows the server to exit cleanly.

## Requirements

* Linux x86-64
* Half-Life 2: Deathmatch dedicated server
* Metamod:Source

The plugin only loads on a compatible HL2DM server. If the expected problem is not found, it refuses to make any changes.

## Installation

Extract the release archive:

```bash
cd /tmp
tar -xzf ~/Downloads/hl2mp-shutdown-fix-mms-x64-1.2.1.tar.gz
cd hl2mp-shutdown-fix-mms-x64-1.2.1
```

Optinally, verify the package:

```bash
./verify.sh
```

Install it into your HL2DM server, for example:

```bash
sudo ./install.sh /home/user/servers/hl2dm-serverfiles/hl2mp
```

Replace the path with the location of your own `hl2mp` directory.

Restart the server and check that the plugin loaded:

```text
meta list
```

You should see:

```text
[META] Loaded HL2MP Shutdown Fix
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

The server should exit without aborting or crashing.

## How it works

The plugin corrects one incorrect function reference inside the loaded HL2DM dedicated-server library.

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
