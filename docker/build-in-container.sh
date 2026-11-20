#!/bin/sh
# Runs inside the build container. Expects:
#   /src                 read-only project tree
#   /firmware/z_skm.hex  K60 firmware image
#   /out                 output directory for the .deb
#   PKG_SUFFIX, PKG_DIST, HOST_UID, HOST_GID
set -eu

for tool in dpkg-buildpackage dpkg-parsechangelog dh gcc make; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "ERROR: $tool not found: this image is not a kmgr build image." >&2
        echo "Use an image built from docker/Dockerfile.<target>." >&2
        exit 1
    fi
done

rm -rf /build
mkdir -p /build/kmgr
cp -a /src/. /build/kmgr/
cd /build/kmgr
rm -rf .git dist obj kmgr debian/kmgr debian/.debhelper debian/files \
    debian/debhelper-build-stamp debian/*.debhelper debian/*.substvars

mkdir -p kfs
cp /firmware/z_skm.hex kfs/z_skm.hex

base_version=$(dpkg-parsechangelog | sed -n 's/^Version: //p')
version="${base_version}${PKG_SUFFIX}"
{
    printf 'kmgr (%s) %s; urgency=medium\n\n' "$version" "$PKG_DIST"
    printf '  * Build for %s.\n\n' "$PKG_DIST"
    printf ' -- KMGR Maintainers <root@localhost>  %s\n\n' "$(date -R)"
    cat debian/changelog
} > debian/changelog.new
mv debian/changelog.new debian/changelog

echo "== $(gcc --version | head -n 1)"
echo "== $(ldd --version | head -n 1)"

# dh_auto_test runs "make test" as part of the package build.
dpkg-buildpackage -us -uc -b

deb=$(ls ../kmgr_"${version}"_*.deb)
dpkg-deb --info "$deb"
dpkg-deb --contents "$deb"
echo "== Highest GLIBC symbol version required by /usr/bin/kmgr:"
objdump -T kmgr | grep -o 'GLIBC_[0-9.]*' | sort -u -t. -k1,1 -k2,2n -k3,3n | tail -n 1

cp "$deb" /out/
chown "${HOST_UID}:${HOST_GID}" "/out/$(basename "$deb")"
echo "== Wrote dist/${PKG_DIST}/$(basename "$deb")"
