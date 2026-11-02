# KMGR

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
