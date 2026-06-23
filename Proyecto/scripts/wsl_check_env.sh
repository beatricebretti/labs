#!/usr/bin/env bash
set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
IDF_DIR="${IDF_PATH:-${HOME}/esp/esp-idf}"

errors=0

ok() {
    printf '[OK] %s\n' "$1"
}

warn() {
    printf '[WARN] %s\n' "$1"
}

fail() {
    printf '[ERROR] %s\n' "$1"
    errors=$((errors + 1))
}

check_file() {
    if [ -f "$1" ]; then
        ok "$2"
    else
        fail "$2: no existe $1"
    fi
}

check_grep() {
    local pattern="$1"
    local file="$2"
    local message="$3"

    if grep -q "$pattern" "$file" 2>/dev/null; then
        ok "$message"
    else
        fail "$message: no se encontro '$pattern' en $file"
    fi
}

printf 'Proyecto: %s\n' "${PROJECT_ROOT}"
printf 'ESP-IDF:  %s\n\n' "${IDF_DIR}"

if [ -n "${BASH_VERSION:-}" ]; then
    ok "La shell actual es Bash ${BASH_VERSION%%.*}"
else
    fail "Ejecuta este script con Bash: bash scripts/wsl_check_env.sh"
fi

if grep -qiE 'microsoft|wsl' /proc/version 2>/dev/null; then
    ok "La terminal parece estar corriendo dentro de WSL"
else
    warn "No se detecto WSL en /proc/version; esto puede estar bien si estas en Linux nativo"
fi

check_file "${IDF_DIR}/export.sh" "export.sh de ESP-IDF disponible"
check_file "${PROJECT_ROOT}/cam_identificador/code/CMakeLists.txt" "Proyecto cam_identificador disponible"
check_file "${PROJECT_ROOT}/cam_bordes/code/CMakeLists.txt" "Proyecto cam_bordes disponible"

if [ -f "${IDF_DIR}/export.sh" ]; then
    # shellcheck disable=SC1090
    source "${IDF_DIR}/export.sh" >/dev/null
    if command -v idf.py >/dev/null 2>&1; then
        ok "idf.py disponible: $(idf.py --version 2>/dev/null | head -n 1)"
    else
        fail "idf.py no quedo en PATH despues de source ${IDF_DIR}/export.sh"
    fi
fi

check_grep '^CONFIG_IDF_TARGET="esp32"' "${PROJECT_ROOT}/cam_identificador/code/sdkconfig" "cam_identificador usa target esp32"
check_grep '^CONFIG_CAMERA_MODULE_AI_THINKER=y' "${PROJECT_ROOT}/cam_identificador/code/sdkconfig" "cam_identificador usa modulo AI-Thinker"
check_grep '^CONFIG_IDF_TARGET="esp32"' "${PROJECT_ROOT}/cam_bordes/code/sdkconfig" "cam_bordes usa target esp32"
check_grep '^CONFIG_CAMERA_MODULE_AI_THINKER=y' "${PROJECT_ROOT}/cam_bordes/code/sdkconfig" "cam_bordes usa modulo AI-Thinker"

serial_ports="$(find /dev -maxdepth 1 \( -name 'ttyUSB*' -o -name 'ttyACM*' \) -print 2>/dev/null | sort)"
if [ -n "${serial_ports}" ]; then
    ok "Puertos seriales visibles en WSL:"
    printf '%s\n' "${serial_ports}"
else
    warn "No hay /dev/ttyUSB* ni /dev/ttyACM* visibles; para flashear conecta la placa con usbipd-win"
fi

printf '\n'
if [ "${errors}" -eq 0 ]; then
    ok "Revision terminada sin errores criticos"
    exit 0
fi

fail "Revision terminada con ${errors} error(es)"
exit 1
