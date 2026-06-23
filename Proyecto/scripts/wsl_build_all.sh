#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
IDF_DIR="${IDF_PATH:-${HOME}/esp/esp-idf}"

if [ ! -f "${IDF_DIR}/export.sh" ]; then
    printf '[ERROR] No existe %s\n' "${IDF_DIR}/export.sh" >&2
    printf 'Define IDF_PATH o instala ESP-IDF en ~/esp/esp-idf.\n' >&2
    exit 1
fi

# shellcheck disable=SC1090
source "${IDF_DIR}/export.sh"

build_project() {
    local name="$1"
    local dir="$2"

    printf '\n==> Compilando %s\n' "${name}"
    cd "${PROJECT_ROOT}/${dir}"
    idf.py set-target esp32
    idf.py reconfigure
    idf.py build
}

build_project "cam_identificador" "cam_identificador/code"
build_project "cam_bordes" "cam_bordes/code"

printf '\n[OK] Builds terminadas.\n'
printf 'Binarios esperados:\n'
printf '  %s\n' "${PROJECT_ROOT}/cam_identificador/code/build/robot_sumo_identifier_camera.bin"
printf '  %s\n' "${PROJECT_ROOT}/cam_bordes/code/build/robot_sumo_edge_camera.bin"
