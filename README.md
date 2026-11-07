# KMGR

## Project diagnostics and tests

The process starts a manager, serial I/O worker, socket server and log poller.
The manager bootstraps the K60 firmware and optional Broadcom firmware, dispatches
commands and monitors health. Calibration data is converted from Lua to IPF.

Failure logs identify the operation and include return codes, file paths, sizes,
device names or OS errors where applicable. Bootloader error returns include the
function and source line. Broadcom status returns preserve the original enum
value and log non-success results. Normal polling timeouts, lookup misses,
checksum values, byte counts and successful low-level calls stay quiet.
Completion logs are limited to useful initialization and conversion boundaries.

| Area | Return-path handling and tests |
| --- | --- |
| Startup and configuration | Argument errors, thread creation, semaphore initialization, missing configuration and truncated values are logged. |
| Sockets | Listener failures and descriptor cleanup; malformed/oversized requests, incomplete bodies, invalid responses and disconnects. |
| Command management | Invalid clients, semaphore waits, help storage exhaustion and management response validation. |
| Serial and framing | Initialization/read/write errors, subchannel bounds, checksum round trips and optional-checksum handling. |
| K60 loader | Temporary files, conversion writes, image/header bounds, MCU-version responses and protocol error returns. |
| Calibration | Missing files, malformed/oversized tokens, error propagation and output capacity. |
| Broadcom | Typed status returns and relay adapter errors; arithmetic/endian utilities return data and are not error-logged. |
| Logging | Invalid logger arguments are reported directly to stderr to avoid recursive logging. Lookup misses remain ordinary results. |
| Deployment | Launch scripts use `exec`; package hooks stop on errors; systemd captures output in the journal and restarts services. |

Run the complete hardware-independent regression suite:

```sh
make test
```

Individual targets are `test-loader`, `test-core`, `test-management`,
`test-protocol`, `test-serial` and `test-brcm`. Serial tests use Linux
pseudo-terminals; socket tests use loopback listeners and local socket pairs.
MCU and relay responses are simulated. No attached hardware is needed.

Run memory and undefined-behavior checks:

```sh
make test CFLAGS='-W -O1 -g -DFRAMER_ALLOW_HANDLERS=1 -DFIX_ENDIAN=0 -fsanitize=address,undefined -fno-omit-frame-pointer'
```

These tests cover representative success and failure paths, not every vendor
opcode or fault-injection case. Real K60/Broadcom programming still requires
hardware verification. SPI/MII hooks in the supplied adapter are placeholders.
Concurrent servlet cancellation still uses shared connection state and needs a
separate concurrency review. Serial/socket short writes are diagnosed but are
not universally retried. The existing switch fallthrough and vendor signed-shift
compiler warnings are outside this diagnostics change.

## Debian package for Raspberry Pi / Ubuntu 24.04

Build natively on the Raspberry Pi running Ubuntu 24.04 (64-bit `arm64`
recommended). The packaging also permits `armhf` when built in a matching
32-bit environment. Do not package the prebuilt binary or object files from
another system; `dpkg-buildpackage` cleans and rebuilds the application.

```sh
sudo apt update
sudo apt install build-essential debhelper dpkg-dev
make deb
sudo apt install ../kmgr_1.0.0_arm64.deb
```

The package is written to the parent directory. Use the `armhf` filename
instead when building for that architecture. A compiler failure stops package
creation.

Installation uses debconf to ask whether to enable/start `kmgr_gu`. The default
is No. `kmgr_uut` is always enabled and started (when systemd and local service
policy permit). Both unit files are shipped; declining GU stops and disables
it. Services run as the system account `kmgr`, with `dialout` access for serial
devices, rather than relying on a pre-existing `pi` user. Output goes to the
systemd journal.

| Files | Installed location |
| --- | --- |
| Application | `/usr/bin/kmgr` |
| `resources/*.cfg` | `/etc/kmgr/` |
| `resources/*.sh` | `/usr/local/bin/` |
| `resources/*.service` | `/usr/lib/systemd/system/` |
| KFS firmware | `/usr/share/kmgr/kfs/z_skm.hex` |

Configuration files are Debian conffiles: local modifications are preserved
across upgrades. Before use, set the serial-device paths in `/etc/kmgr/env0.cfg`
(UUT) and `/etc/kmgr/env1.cfg` (GU) to match the attached hardware. The packaged
defaults refer to specific FTDI serial numbers; verify that the `kmgr` account
has access to your devices.

Change the GU selection later:

```sh
sudo dpkg-reconfigure kmgr
```

For unattended installation, preseed the answer (use `false` to disable GU):

```sh
echo 'kmgr kmgr/install-gu boolean true' | sudo debconf-set-selections
sudo DEBIAN_FRONTEND=noninteractive apt install ../kmgr_1.0.0_arm64.deb
```

Inspect the services with:

```sh
systemctl status kmgr_uut kmgr_gu
journalctl -u kmgr_uut -u kmgr_gu
```

Removal stops both services. Purging also removes packaged configuration and
debconf answers. The system account is retained to avoid reassigning its UID.

The requested `/usr/local/bin` installation is intentional, although Debian
policy normally reserves `/usr/local` for administrator-managed files; this
package is intended for local deployment, not submission to the Ubuntu archive.
