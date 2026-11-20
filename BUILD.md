# Building the KMGR Debian packages with Docker

KMGR is packaged for two Raspberry Pi generations. Both packages are built in
Docker containers that emulate the Pi's CPU, so any Linux host with Docker can
build them. No Raspberry Pi is needed for the build.

| Target | Pi / OS | Make target | Package produced |
| --- | --- | --- | --- |
| `jessie` | Pi 2/3 running Raspbian/Debian 8 jessie (e.g. kernel `4.4.34-v7+`) | `make deb-jessie` | `dist/jessie/kmgr_<version>+deb8_armhf.deb` |
| `noble` | Pi running Ubuntu 24.04 (64-bit) | `make deb-noble` | `dist/noble/kmgr_<version>+ubuntu24.04_arm64.deb` |

`<version>` comes from the top entry of `debian/changelog` (currently `1.0.0`).

## 1. Prerequisites

- Linux host with Docker installed, and a user that can run `docker`.
- Internet access, to pull the base images and Debian/Ubuntu packages.
- The K60 firmware image `z_skm.hex`. It is not stored in the repository.

### Register ARM emulation (x86 hosts only)

On an x86 host, register QEMU so Docker can run ARM containers. This is needed
once per boot:

```sh
docker run --privileged --rm tonistiigi/binfmt --install arm,arm64
```

Check that it worked:

```sh
ls /proc/sys/fs/binfmt_misc/ | grep qemu
# expect: qemu-aarch64  qemu-arm
```

If emulation is missing, the build script stops and prints the command above.

## 2. Provide the firmware

Either place the firmware at the default location:

```sh
mkdir -p kfs
cp /path/to/z_skm.hex kfs/z_skm.hex
```

or pass its path on each build with `K60_FIRMWARE=/absolute/path/z_skm.hex`.
The build stops if the file is missing, unreadable or empty.

## 3. Build

From the repository root:

```sh
make deb-jessie        # Raspbian/Debian 8 jessie, armhf
make deb-noble         # Ubuntu 24.04, arm64
make deb-all           # both
```

With a firmware file outside `kfs/`:

```sh
make deb-all K60_FIRMWARE=/absolute/path/z_skm.hex
```

The make targets call `scripts/docker-build.sh`, which can also be run directly:

```sh
scripts/docker-build.sh jessie
K60_FIRMWARE=/absolute/path/z_skm.hex scripts/docker-build.sh noble
```

### What a build does

1. Builds the image `kmgr-build:<target>` from `docker/Dockerfile.<target>`
   (pulling the base image for the right CPU each time), or pulls a prebuilt
   image from your registry (see [Prebuilt images](#prebuilt-images-in-a-company-registry)).
   It checks that the image's CPU architecture matches the target.
2. Mounts the source tree read-only and copies it inside the container, so
   your working tree is not modified.
3. Adds a changelog entry with the version suffix (`+deb8` or `+ubuntu24.04`)
   and the distribution name.
4. Runs `dpkg-buildpackage`, which compiles KMGR and runs `make test`. A
   failing test stops the build.
5. Prints the package info and contents, and the highest glibc symbol version
   the binary needs.
6. Copies the `.deb` to `dist/<target>/`, owned by your user.

Under emulation the first build takes several minutes, mostly for installing
packages into the image. Later builds reuse the cached image layers.

### Build environments

| | jessie | noble |
| --- | --- | --- |
| Base image | `debian/eol:jessie` | `ubuntu:24.04` |
| Docker platform | `linux/arm/v7` | `linux/arm64` |
| Compiler | gcc 4.9 | gcc 13 |
| C library | glibc 2.19 | glibc 2.39 |
| debhelper | 10 (jessie-backports) | 13 |
| Unit file location | `/lib/systemd/system` | `/usr/lib/systemd/system` |

The same `debian/` directory serves both. It uses debhelper compat level 10,
and the maintainer scripts handle the systemd services themselves.

## 4. Check the result

Look for these lines at the end of the build output:

```text
== Highest GLIBC symbol version required by /usr/bin/kmgr:
GLIBC_2.x
== Wrote dist/jessie/kmgr_1.0.0+deb8_armhf.deb
```

For jessie, the glibc version must be 2.19 or lower.

Inspect a package on the host:

```sh
dpkg-deb --info dist/jessie/kmgr_*_armhf.deb
dpkg-deb --contents dist/jessie/kmgr_*_armhf.deb
```

## 5. Install on the Pi

Copy the matching package to the Pi.

Jessie (Pi 2/3):

```sh
sudo dpkg -i kmgr_1.0.0+deb8_armhf.deb
sudo apt-get -f install        # only if dpkg reports missing dependencies
```

Ubuntu 24.04:

```sh
sudo apt install ./kmgr_1.0.0+ubuntu24.04_arm64.deb
```

Installation asks whether to enable the GU service (default: No). `kmgr_uut` is
always enabled. Then set the serial devices in `/etc/kmgr/env0.cfg` (UUT) and
`/etc/kmgr/env1.cfg` (GU), and check the services:

```sh
systemctl status kmgr_uut kmgr_gu
journalctl -u kmgr_uut -u kmgr_gu
```

See [README.md](README.md) for the full service, configuration and removal
details.

## Prebuilt images in a company registry

The build images hold only the toolchain (compiler, debhelper, dpkg tools).
The source tree, firmware and `docker/build-in-container.sh` are mounted
when the container starts. So one published image serves every KMGR version.
Rebuild and republish only when the Dockerfiles change, or to pick up OS
updates. Pulling a prebuilt image skips the slow emulated image build. A
frozen jessie image also keeps working if `archive.debian.org` ever stops
serving Debian 8.

Image tags in the repository:

| Tag | Contents |
| --- | --- |
| `<repository>:jessie-armhf` | Latest jessie toolchain (`linux/arm/v7`) |
| `<repository>:noble-arm64` | Latest Ubuntu 24.04 toolchain (`linux/arm64`) |
| `<repository>:jessie-armhf-YYYYMMDD`, `<repository>:noble-arm64-YYYYMMDD` | Dated copies, kept for reproducible builds |

### Publish the images (maintainers)

Requires ARM emulation (step 1) and push rights to the registry:

```sh
docker login registry.example.com
make push-images KMGR_BUILD_REGISTRY=registry.example.com/team/kmgr-build
```

This builds both images, checks that they run, then pushes the moving tag and
a dated tag for each. To build the images locally without pushing, run
`make docker-images`.

### Build packages from the published images

Set `KMGR_BUILD_REGISTRY` to the same repository. The script pulls
`<repository>:jessie-armhf` or `<repository>:noble-arm64` instead of building:

```sh
docker login registry.example.com     # if the registry needs it
make deb-all KMGR_BUILD_REGISTRY=registry.example.com/team/kmgr-build
```

or set it once in your shell:

```sh
export KMGR_BUILD_REGISTRY=registry.example.com/team/kmgr-build
make deb-jessie
```

To pin an exact image, for example a dated tag, use `KMGR_BUILD_IMAGE`. It
selects one image, so build one target at a time:

```sh
make deb-jessie KMGR_BUILD_IMAGE=registry.example.com/team/kmgr-build:jessie-armhf-20261120
```

| Variable | Effect |
| --- | --- |
| `KMGR_BUILD_IMAGE` | Use exactly this image. Takes precedence. |
| `KMGR_BUILD_REGISTRY` | Pull `<repository>:jessie-armhf` / `:noble-arm64`. Also the push target for `make push-images`. |
| neither | Build `kmgr-build:<target>` locally from `docker/`. |

The host still needs ARM emulation (step 1) on x86, because the build runs ARM
code inside the container. The script refuses an image built for the wrong
CPU, and an image without the build tools.

## Troubleshooting

| Symptom | Cause / fix |
| --- | --- |
| `ERROR: cannot run linux/arm/v7 containers on this host` | ARM emulation is not registered. Run the `tonistiigi/binfmt` command in step 1. Re-run it after every host reboot. |
| `exec format error` during `docker build` | Same as above. |
| `ERROR: Required K60 firmware is missing, unreadable or empty` | Provide `kfs/z_skm.hex` or set `K60_FIRMWARE` (step 2). |
| Test failure stops the build | The same tests run on the host with `make test`. Fix them there first. `make check-c90` reports code that gcc 4.9 cannot compile. |
| Package does not run on a Pi 1 or Pi Zero | Those boards are ARMv6. The jessie image targets ARMv7 (Pi 2/3) only. |
| `ERROR: image ... is for 'arm64', target jessie needs 'arm'` | `KMGR_BUILD_IMAGE` points to the wrong target's image. Use the `jessie-armhf` tag for jessie and `noble-arm64` for noble. |
| `ERROR: dpkg-buildpackage not found: this image is not a kmgr build image` | The selected image was not built from `docker/Dockerfile.<target>`. |
| `pull access denied` / `unauthorized` | Run `docker login` for the registry. |
| `apt-get update` fails in the jessie image | Debian 8 is end of life and served from `archive.debian.org`. Check that the host can reach it. |

## Notes

- `make deb` still builds natively on a Pi, without Docker, for the OS that Pi
  is running.
- Debian 8 jessie is end of life. Its packages come from `archive.debian.org`,
  and the image accepts the archive's expired signing keys.
- Remove the local build images with `docker rmi kmgr-build:jessie kmgr-build:noble`.
