#!/bin/sh
# Launches BrainBoost.
#
# LD_LIBRARY_PATH is unset because terminals inside the VSCode Flatpak inject
# host libraries (/run/host/usr/lib64) that break EGL/Wayland window creation.
cd "$(dirname "$0")/.." || exit 1

if [ ! -x build/brainboost ]; then
    echo "Executável não encontrado. Compile primeiro:"
    echo "  cmake -B build && cmake --build build -j\$(nproc)"
    exit 1
fi

exec env -u LD_LIBRARY_PATH ./build/brainboost "$@"
