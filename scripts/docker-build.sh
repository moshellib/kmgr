#!/bin/sh
# Build kmgr Debian packages inside emulated ARM containers, and manage the
# build images.
#
#   scripts/docker-build.sh jessie            # package: Debian 8 armhf (Pi 2/3)
#   scripts/docker-build.sh noble             # package: Ubuntu 24.04 arm64
#   scripts/docker-build.sh --image-only noble        # build image only
#   scripts/docker-build.sh --push REPOSITORY jessie  # build and push image
#
# Image selection for package builds (first match wins):
#   KMGR_BUILD_IMAGE=<image:tag>     use exactly this image (pulled)
#   KMGR_BUILD_REGISTRY=<repository> pull <repository>:jessie-armhf / :noble-arm64
#   (neither)                        build kmgr-build:<target> locally
#
# K60_FIRMWARE selects the firmware image (default: kfs/z_skm.hex).
# On a non-ARM host, QEMU binfmt handlers must be registered first:
#   docker run --privileged --rm tonistiigi/binfmt --install arm,arm64
set -eu

cd "$(dirname "$0")/.."

usage() {
    echo "usage: $0 [--image-only | --push REPOSITORY] jessie|noble" >&2
    exit 2
}

mode=package
push_repo=
case "${1:-}" in
--image-only)
    mode=image
    shift
    ;;
--push)
    mode=push
    push_repo=${2:-}
    [ -n "$push_repo" ] || usage
    shift 2
    ;;
esac
[ $# -eq 1 ] || usage
target=$1

case "$target" in
jessie)
    platform=linux/arm/v7
    arch=arm
    tag=jessie-armhf
    suffix=+deb8
    ;;
noble)
    platform=linux/arm64
    arch=arm64
    tag=noble-arm64
    suffix=+ubuntu24.04
    ;;
*)
    usage
    ;;
esac

local_image=kmgr-build:$target

build_image() {
    docker build --pull --platform "$platform" -f "docker/Dockerfile.$target" \
        -t "$local_image" docker
}

# Refuse an image built for the wrong CPU (e.g. noble image for jessie).
check_arch() {
    image_arch=$(docker image inspect --format '{{.Architecture}}' "$1")
    if [ "$image_arch" != "$arch" ]; then
        echo "ERROR: image $1 is for '$image_arch', target $target needs '$arch'." >&2
        exit 1
    fi
}

check_emulation() {
    if ! docker run --rm --platform "$platform" "$1" true >/dev/null 2>&1; then
        echo "ERROR: cannot run $platform containers on this host." >&2
        echo "Register QEMU emulation first:" >&2
        echo "  docker run --privileged --rm tonistiigi/binfmt --install arm,arm64" >&2
        exit 1
    fi
}

case "$mode" in
image)
    build_image
    check_arch "$local_image"
    echo "== Built $local_image"
    exit 0
    ;;
push)
    build_image
    check_arch "$local_image"
    check_emulation "$local_image"
    dated="$push_repo:$tag-$(date +%Y%m%d)"
    docker tag "$local_image" "$push_repo:$tag"
    docker tag "$local_image" "$dated"
    docker push "$push_repo:$tag"
    docker push "$dated"
    echo "== Pushed $push_repo:$tag and $dated"
    exit 0
    ;;
esac

# Package build.
firmware=${K60_FIRMWARE:-kfs/z_skm.hex}
if [ ! -f "$firmware" ] || [ ! -r "$firmware" ] || [ ! -s "$firmware" ]; then
    echo "ERROR: Required K60 firmware is missing, unreadable or empty: $firmware" >&2
    exit 1
fi
firmware=$(cd "$(dirname "$firmware")" && pwd)/$(basename "$firmware")

if [ -n "${KMGR_BUILD_IMAGE:-}" ]; then
    image=$KMGR_BUILD_IMAGE
elif [ -n "${KMGR_BUILD_REGISTRY:-}" ]; then
    image=$KMGR_BUILD_REGISTRY:$tag
else
    image=
fi

if [ -n "$image" ]; then
    echo "== Using prebuilt image $image"
    docker pull --platform "$platform" "$image"
else
    build_image
    image=$local_image
fi
check_arch "$image"
check_emulation "$image"

outdir=$(pwd)/dist/$target
mkdir -p "$outdir"

docker run --rm --platform "$platform" \
    -v "$(pwd):/src:ro" \
    -v "$firmware:/firmware/z_skm.hex:ro" \
    -v "$outdir:/out" \
    -v "$(pwd)/docker/build-in-container.sh:/usr/local/bin/build-in-container.sh:ro" \
    -e PKG_SUFFIX="$suffix" -e PKG_DIST="$target" \
    -e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" \
    "$image" sh /usr/local/bin/build-in-container.sh
