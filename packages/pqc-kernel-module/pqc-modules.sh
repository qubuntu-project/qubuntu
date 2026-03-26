#!/bin/bash
# pqc-modules.sh — load/unload ML-KEM-768 kernel modules
set -e

MODULES=(liboqs_wrapper pqc_kem pqc_kem_test)

usage() { echo "Usage: $0 {load|unload|status}"; exit 1; }

status() {
    echo "=== PQC module status ==="
    for m in "${MODULES[@]}"; do
        if lsmod | grep -q "^$m"; then
            echo "  [loaded]   $m"
        else
            echo "  [unloaded] $m"
        fi
    done
    echo "=== /proc/crypto ==="
    grep -A4 "name.*ml-kem" /proc/crypto 2>/dev/null || echo "  ml-kem-768 not registered"
}

load() {
    echo "Loading PQC modules..."
    for m in "${MODULES[@]}"; do
        sudo modprobe "$m" && echo "  loaded: $m"
    done
    sudo dmesg | grep "pqc_test" | tail -8
}

unload() {
    echo "Unloading PQC modules..."
    for m in $(echo "${MODULES[@]}" | tr ' ' '\n' | tac); do
        if lsmod | grep -q "^$m"; then
            sudo rmmod "$m" && echo "  unloaded: $m"
        fi
    done
}

case "${1:-}" in
    load)   load   ;;
    unload) unload ;;
    status) status ;;
    *)      usage  ;;
esac
