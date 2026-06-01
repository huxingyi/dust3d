#!/usr/bin/env bash
# collect_oom_report.sh — Dust3D issue #184 diagnostic script
#
# Note: the 1.1.3 AppImage is built with Qt 5.15.2 (NOT Qt6).
# Qt 5.15 includes the RHI abstraction layer (introduced in Qt 5.14),
# so QSG_RHI_BACKEND and qt.rhi.* logging rules are valid here.
#
# Run this on the affected machine (Intel HD 4400 / Mesa crocus-i965).
# It downloads Dust3D 1.1.3, runs it under several Qt 5.15 RHI backend
# configurations, monitors memory, and writes a self-contained report.
#
# Usage:
#   chmod +x collect_oom_report.sh
#   ./collect_oom_report.sh
#
# Output:  dust3d_oom_report_<hostname>_<date>.txt  (directory where script is run)
#
# Requirements: wget or curl, awk, timeout, glxinfo (mesa-utils),
#               vulkaninfo (optional), free, lscpu.
#
# CI mode: if DISPLAY is not set, the script starts a temporary Xvfb on :99
# automatically and tears it down on exit.

set -euo pipefail

APPIMAGE_URL="https://github.com/huxingyi/dust3d/releases/download/1.1.3/dust3d-1.1.3.AppImage"
APPIMAGE_BIN="dust3d-1.1.3.AppImage"
EXTRACT_DIR="squashfs-root"
RUN_SECONDS=30          # seconds each test runs before being killed
# Use absolute path so the report stays in the original CWD even after
# the script changes directory into the tmpdir for AppImage extraction.
REPORT="$(pwd)/dust3d_oom_report_$(hostname -s)_$(date +%Y%m%d_%H%M%S).txt"
WORK_DIR="$(mktemp -d /tmp/dust3d_diag.XXXXXX)"
XVFB_OWN=0   # 1 if we started Xvfb ourselves

# ── helpers ──────────────────────────────────────────────────────────────────

log()  { echo "$*" | tee -a "$REPORT"; }
sep()  { log ""; log "$(printf '=%.0s' {1..72})"; log "$*"; log "$(printf '=%.0s' {1..72})"; }
sep2() { log ""; log "$(printf -- '-%.0s' {1..72})"; log "$*"; log "$(printf -- '-%.0s' {1..72})"; }

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || { log "WARNING: '$1' not found — skipping that section."; return 1; }
}

cleanup() {
  kill "${MONITOR_PID:-}" 2>/dev/null || true
  kill "${APP_PID:-}"     2>/dev/null || true
  [ "$XVFB_OWN" = "1" ] && pkill -f 'Xvfb :99' 2>/dev/null || true
  # Guard: only remove if WORK_DIR looks like the tmp path we created
  case "${WORK_DIR:-}" in
    /tmp/dust3d_diag.*) rm -rf "$WORK_DIR" ;;
    *) echo "WARNING: unexpected WORK_DIR='${WORK_DIR:-}' — skipping rm -rf" ;;
  esac
}
trap cleanup EXIT

# ── display setup (CI: start Xvfb if no display is present) ──────────────────
if [ -z "${DISPLAY:-}" ]; then
  if command -v Xvfb >/dev/null 2>&1; then
    Xvfb :99 -screen 0 1280x800x24 > /dev/null 2>&1 &
    XVFB_OWN=1
    export DISPLAY=:99
    export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/tmp/runtime-root}"
    mkdir -p "$XDG_RUNTIME_DIR" && chmod 700 "$XDG_RUNTIME_DIR"
    sleep 1   # let Xvfb initialise
    echo "Started Xvfb on :99 (CI mode)"
  else
    echo "WARNING: No DISPLAY set and Xvfb not found. Tests may fail to connect to display."
  fi
else
  echo "Using existing DISPLAY=$DISPLAY"
fi

# ── header ───────────────────────────────────────────────────────────────────

> "$REPORT"   # truncate / create
sep "Dust3D OOM Diagnostic Report  —  issue #184  (AppImage: Qt 5.15.2 / RHI)"
log "Generated : $(date)"
log "Host      : $(hostname -f 2>/dev/null || hostname)"
log "User      : $(id)"
log ""

# ── 1. system info ───────────────────────────────────────────────────────────

sep "1. SYSTEM INFORMATION"

log "--- Kernel ---"
uname -a | tee -a "$REPORT"

log ""
log "--- CPU ---"
if need_cmd lscpu; then
  lscpu 2>&1 | tee -a "$REPORT"
fi

log ""
log "--- Memory ---"
free -h 2>&1 | tee -a "$REPORT"
log ""
grep -E "^MemTotal|^MemFree|^MemAvailable|^SwapTotal|^SwapFree" /proc/meminfo | tee -a "$REPORT"

log ""
log "--- OS Release ---"
cat /etc/os-release 2>/dev/null | tee -a "$REPORT" || true
lsb_release -a 2>/dev/null | tee -a "$REPORT" || true

# ── 2. GPU / Mesa / OpenGL ────────────────────────────────────────────────────

sep "2. GPU / MESA / OPENGL"

log "--- lspci (VGA/Display) ---"
if need_cmd lspci; then
  lspci -v 2>&1 | grep -A8 -iE "VGA|Display|3D|Render" | tee -a "$REPORT" || true
fi

log ""
log "--- Mesa version ---"
if need_cmd glxinfo; then
  DISPLAY="${DISPLAY:-:0}" glxinfo 2>&1 | grep -iE "OpenGL vendor|OpenGL renderer|OpenGL version|OpenGL core|direct rendering|Mesa" | tee -a "$REPORT"
  log ""
  log "--- Full glxinfo ---"
  DISPLAY="${DISPLAY:-:0}" glxinfo 2>&1 | tee -a "$REPORT"
else
  log "glxinfo not found. Install with:  sudo apt install mesa-utils"
fi

log ""
log "--- EGL info ---"
if need_cmd eglinfo; then
  DISPLAY="${DISPLAY:-:0}" eglinfo 2>&1 | tee -a "$REPORT" || true
fi

log ""
log "--- Vulkan info ---"
if need_cmd vulkaninfo; then
  vulkaninfo --summary 2>&1 | tee -a "$REPORT" || true
else
  log "vulkaninfo not found (install vulkan-tools if desired)."
fi

log ""
log "--- DRI_PRIME / GPU env ---"
env | grep -iE "DISPLAY|WAYLAND|DRI|MESA|LIBGL|QT_|QSG_|GPU|INTEL|GALLIUM" | sort | tee -a "$REPORT" || true

log ""
log "--- /sys GPU info ---"
find /sys/class/drm -name "card*" -maxdepth 1 2>/dev/null | while read -r d; do
  echo "  $d"
  cat "$d/device/vendor"  2>/dev/null | xargs printf "    vendor : %s\n" || true
  cat "$d/device/device"  2>/dev/null | xargs printf "    device : %s\n" || true
  cat "$d/device/uevent"  2>/dev/null | sed 's/^/    /' || true
done | tee -a "$REPORT" || true

# ── 3. Qt / Library environment ──────────────────────────────────────────────

sep "3. QT / LIBRARY ENVIRONMENT"

log "--- Installed Qt packages ---"
dpkg -l 2>/dev/null | grep -i "libqt\|qt6\|qt5" | tee -a "$REPORT" || \
  rpm -qa 2>/dev/null | grep -i qt | tee -a "$REPORT" || true

log ""
log "--- ld.so.conf / library paths ---"
ldconfig -p 2>/dev/null | grep -iE "libGL|libEGL|libvulkan|libQt6" | sort | tee -a "$REPORT" || true

# ── 4. Download & extract AppImage ───────────────────────────────────────────

sep "4. APPIMAGE SETUP"

cd "$WORK_DIR"

if [ -f "$HOME/$APPIMAGE_BIN" ]; then
  log "Found cached AppImage at $HOME/$APPIMAGE_BIN — copying."
  cp "$HOME/$APPIMAGE_BIN" .
elif [ -f "$(pwd)/$APPIMAGE_BIN" ]; then
  log "AppImage already present."
else
  log "Downloading $APPIMAGE_URL ..."
  if need_cmd wget; then
    wget -q --show-progress -O "$APPIMAGE_BIN" "$APPIMAGE_URL" 2>&1 | tee -a "$REPORT"
  elif need_cmd curl; then
    curl -L --progress-bar -o "$APPIMAGE_BIN" "$APPIMAGE_URL" 2>&1 | tee -a "$REPORT"
  else
    log "ERROR: neither wget nor curl found. Cannot download AppImage."
    exit 1
  fi
fi

chmod +x "$APPIMAGE_BIN"
log "Extracting AppImage (FUSE-free) ..."
./"$APPIMAGE_BIN" --appimage-extract 2>&1 | tee -a "$REPORT" || true

if [ ! -x "$EXTRACT_DIR/AppRun" ]; then
  log "ERROR: extraction failed — $EXTRACT_DIR/AppRun not found."
  exit 1
fi
log "Extraction OK."

APP_BIN="$WORK_DIR/$EXTRACT_DIR/AppRun"

# ── 5. Memory monitor helper ─────────────────────────────────────────────────

cat > "$WORK_DIR/monitor_mem.sh" << 'MONITOR_EOF'
#!/usr/bin/env bash
# monitor_mem.sh <stats_file> <command> [args...]
STATS_FILE="$1"; shift
THRESH_RSS_KB=$((1024 * 1024))         # 1 GB
THRESH_VM_KB=$((4 * 1024 * 1024))      # 4 GB
PEAK_RSS=0; PEAK_VM=0; EXIT_CODE=0
SAMPLES=0

write_stats() {
  {
    echo "PEAK_RSS_KB=$PEAK_RSS"
    echo "PEAK_VM_KB=$PEAK_VM"
    echo "PEAK_RSS_GB=$(awk "BEGIN{printf \"%.2f\", $PEAK_RSS/1048576}")"
    echo "PEAK_VM_GB=$(awk "BEGIN{printf \"%.2f\", $PEAK_VM/1048576}")"
    echo "SAMPLES=$SAMPLES"
    echo "CHILD_EXIT=${EXIT_CODE}"
    [ "$PEAK_RSS" -gt "$THRESH_RSS_KB" ] && \
      echo "FLAG_HIGH_RSS: ${PEAK_RSS} KB  (threshold ${THRESH_RSS_KB} KB)"
    [ "$PEAK_VM" -gt "$THRESH_VM_KB" ] && \
      echo "FLAG_HIGH_VM:  ${PEAK_VM} KB  (threshold ${THRESH_VM_KB} KB)"
  } > "$STATS_FILE"
}

trap 'kill "$TARGET_PID" 2>/dev/null; write_stats; exit 124' TERM INT

"$@" &
TARGET_PID=$!

while kill -0 "$TARGET_PID" 2>/dev/null; do
  if [ -r "/proc/$TARGET_PID/status" ]; then
    RSS=$(awk '/^VmRSS:/{print $2}' "/proc/$TARGET_PID/status" 2>/dev/null || echo 0)
    VM=$(awk '/^VmPeak:/{print $2}' "/proc/$TARGET_PID/status" 2>/dev/null || echo 0)
    [ "${RSS:-0}" -gt "$PEAK_RSS" ] && PEAK_RSS=${RSS:-0}
    [ "${VM:-0}" -gt "$PEAK_VM" ]  && PEAK_VM=${VM:-0}
    SAMPLES=$((SAMPLES + 1))
  fi
  sleep 0.2
done

wait "$TARGET_PID" 2>/dev/null; EXIT_CODE=$?
write_stats

if [ "$PEAK_RSS" -gt "$THRESH_RSS_KB" ] || [ "$PEAK_VM" -gt "$THRESH_VM_KB" ]; then
  exit 42
fi
exit "$EXIT_CODE"
MONITOR_EOF
chmod +x "$WORK_DIR/monitor_mem.sh"

# ── 6. Per-test runner ───────────────────────────────────────────────────────

MONITOR="$WORK_DIR/monitor_mem.sh"

# Restart Xvfb between tests (only when we own it) to give each test a
# clean X server state — avoids residual OpenGL context state leaking.
restart_display() {
  if [ "$XVFB_OWN" = "1" ]; then
    pkill -f 'Xvfb :99' 2>/dev/null || true
    sleep 0.3
    Xvfb :99 -screen 0 1280x800x24 > /dev/null 2>&1 &
    sleep 1
  fi
}

run_test() {
  local NAME="$1"; shift
  local LOG="$WORK_DIR/$NAME.log"
  local STATS="$WORK_DIR/$NAME.stats"
  local EC_FILE="$WORK_DIR/$NAME.exitcode"

  sep2 "Test: $NAME  (env: $*)"

  restart_display
  # QT_QPA_PLATFORM=xcb: force X11 backend (needed when DISPLAY is set but
  # the Wayland socket may also be present on the reporter's desktop).
  set +e
  timeout "$RUN_SECONDS" \
    "$MONITOR" "$STATS" \
    env "$@" \
        QT_QPA_PLATFORM=xcb \
        "$APP_BIN" \
      2>&1 | tee "$LOG"
  TIMED_OUT_OR_EXIT=$?
  set -e

  echo "$TIMED_OUT_OR_EXIT" > "$EC_FILE"

  # Print stats
  log ""
  log "--- Memory stats ($NAME) ---"
  if [ -s "$STATS" ]; then
    cat "$STATS" | tee -a "$REPORT"
  else
    log "  (no stats — process may have exited before first sample)"
  fi

  log ""
  log "--- Exit code: $TIMED_OUT_OR_EXIT ---"
  case "$TIMED_OUT_OR_EXIT" in
    0)   log "  App exited cleanly." ;;
    42)  log "  MEMORY THRESHOLD EXCEEDED (likely OOM candidate)." ;;
    124) log "  Timed out after ${RUN_SECONDS}s (app still running — memory stats valid)." ;;
    137) log "  SIGKILL received — possibly OOM-killed by kernel." ;;
    *)   log "  Exited with code $TIMED_OUT_OR_EXIT." ;;
  esac

  # First 60 lines of app log for quick review
  log ""
  log "--- App log head ($NAME) ---"
  head -60 "$LOG" | tee -a "$REPORT"

  # Look for OOM/allocation keywords in log
  log ""
  log "--- OOM / allocation keywords in log ---"
  grep -iE "out.of.memory|oom|malloc|alloc.*fail|cannot allocate|bad_alloc|rhi|scenegraph|opengl|egl|glcontext|mesa|crocus|i965|intel|driver" \
    "$LOG" 2>/dev/null | head -40 | tee -a "$REPORT" || log "  (none found)"
}

# ── 7. Run test matrix ───────────────────────────────────────────────────────

sep "5. TEST MATRIX  (each test runs for up to ${RUN_SECONDS}s)"

log "Tests will launch Dust3D 1.1.3. The app UI will briefly appear."
log "Do not interact with it — each test closes automatically."
log ""

# Test 1 — Default OpenGL (the reported OOM path)
run_test "t1-opengl" \
  QSG_RHI_BACKEND=opengl \
  LIBGL_DEBUG=verbose \
  MESA_DEBUG=1 \
  QT_LOGGING_RULES="qt.scenegraph.*=true;qt.rhi.*=true"

# Test 2 — Qt software renderer (reporter's partial workaround)
run_test "t2-software" \
  QSG_RHI_BACKEND=software \
  QT_LOGGING_RULES="qt.scenegraph.*=true;qt.rhi.*=true"

# Test 3 — Qt OpenGL software (ANGLE / llvmpipe path)
run_test "t3-qt-software-gl" \
  QT_OPENGL=software \
  QSG_RHI_BACKEND=opengl \
  QT_LOGGING_RULES="qt.scenegraph.*=true;qt.rhi.*=true"

# Test 4 — Mesa llvmpipe forced
run_test "t4-llvmpipe" \
  LIBGL_ALWAYS_SOFTWARE=1 \
  QSG_RHI_BACKEND=opengl \
  GALLIUM_DRIVER=llvmpipe \
  QT_LOGGING_RULES="qt.scenegraph.*=true;qt.rhi.*=true"

# Test 5 — Vulkan (Haswell has no native Vulkan; expect quick failure / fallback)
run_test "t5-vulkan" \
  QSG_RHI_BACKEND=vulkan \
  QT_LOGGING_RULES="qt.scenegraph.*=true;qt.rhi.*=true"

# ── 8. /proc/meminfo at rest ─────────────────────────────────────────────────

sep "6. SYSTEM MEMORY AFTER TESTS"
free -h | tee -a "$REPORT"
grep -E "^MemTotal|^MemFree|^MemAvailable|^SwapFree" /proc/meminfo | tee -a "$REPORT"

# ── 9. dmesg OOM lines ───────────────────────────────────────────────────────

sep "7. KERNEL OOM EVENTS (dmesg)"
if dmesg 2>/dev/null | grep -iE "oom|killed process|out of memory" | tail -30 | tee -a "$REPORT"; then
  true
else
  log "(no OOM events found in dmesg, or access denied)"
fi

# ── 10. Summary table ─────────────────────────────────────────────────────────

sep "8. SUMMARY TABLE"
printf "%-22s %12s %12s %12s %8s\n" "Test" "Peak RSS" "Peak VM" "Samples" "Exit" | tee -a "$REPORT"
printf "%-22s %12s %12s %12s %8s\n" "$(printf -- '-%.0s' {1..22})" "$(printf -- '-%.0s' {1..12})" "$(printf -- '-%.0s' {1..12})" "$(printf -- '-%.0s' {1..12})" "$(printf -- '-%.0s' {1..8})" | tee -a "$REPORT"

for t in t1-opengl t2-software t3-qt-software-gl t4-llvmpipe t5-vulkan; do
  STATS="$WORK_DIR/$t.stats"
  EC_FILE="$WORK_DIR/$t.exitcode"
  if [ -s "$STATS" ]; then
    RSS_GB=$(grep PEAK_RSS_GB "$STATS" | cut -d= -f2)
    VM_GB=$(grep PEAK_VM_GB  "$STATS" | cut -d= -f2)
    SAMP=$(grep SAMPLES      "$STATS" | cut -d= -f2)
    EC=$(cat "$EC_FILE" 2>/dev/null || echo "?")
    printf "%-22s %11s G %11s G %12s %8s\n" "$t" "$RSS_GB" "$VM_GB" "$SAMP" "$EC"
  else
    printf "%-22s %12s %12s %12s %8s\n" "$t" "N/A" "N/A" "N/A" "$(cat "$EC_FILE" 2>/dev/null || echo "?")"
  fi | tee -a "$REPORT"
done

# ── 11. Copy full per-test logs into report ───────────────────────────────────

sep "9. FULL PER-TEST LOGS"
for t in t1-opengl t2-software t3-qt-software-gl t4-llvmpipe t5-vulkan; do
  LOG="$WORK_DIR/$t.log"
  sep2 "Full log: $t"
  if [ -s "$LOG" ]; then
    cat "$LOG" | tee -a "$REPORT"
  else
    log "(empty)"
  fi
done

# ── done ──────────────────────────────────────────────────────────────────────

sep "DONE"
log "Report written to: $REPORT"
log ""
log "Please attach  $REPORT  to GitHub issue #184."
log "If the file is large, you can compress it:  gzip $REPORT"

echo ""
echo "Report saved to: $REPORT"
