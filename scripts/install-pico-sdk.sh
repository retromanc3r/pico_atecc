#!/usr/bin/env bash
# Install and configure the Pico SDK for tests or builds.
# Fetches the submodule if it isn't already present and
# exports PICO_SDK_PATH for the current shell.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Initialize the Pico SDK submodule if necessary
if [ ! -d "${REPO_ROOT}/pico-sdk/.git" ]; then
    git -C "${REPO_ROOT}" submodule update --init --recursive pico-sdk
fi

export PICO_SDK_PATH="${REPO_ROOT}/pico-sdk"
echo "PICO_SDK_PATH set to ${PICO_SDK_PATH}"
