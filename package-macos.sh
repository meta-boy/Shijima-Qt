#!/bin/bash
set -euo pipefail

CONFIG="${1:-debug}"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

APP_NAME="Shijima-Qt"
DMG_NAME="${APP_NAME}.dmg"
APP_BUNDLE="publish/macOS/${CONFIG}/${APP_NAME}.app"
INSTALL_DIR="/Applications"

# Ensure libarchive is discoverable (Homebrew keg-only)
if command -v brew &>/dev/null; then
    export PKG_CONFIG_PATH="$(brew --prefix libarchive)/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
fi

echo "==> Building (CONFIG=${CONFIG})..."
make CONFIG="$CONFIG" -j"$(sysctl -n hw.ncpu)"

echo "==> Creating app bundle..."
rm -rf "$APP_BUNDLE"
cp -r "${APP_NAME}.app" "$APP_BUNDLE"
mkdir -p "$APP_BUNDLE/Contents/MacOS"
cp "publish/macOS/${CONFIG}/shijima-qt" "$APP_BUNDLE/Contents/MacOS/"
cp "publish/macOS/${CONFIG}/libunarr.1.dylib" "$APP_BUNDLE/Contents/MacOS/"

echo "==> Running macdeployqt..."
macdeployqt "$APP_BUNDLE" 2>&1 | grep -v "^ERROR:" || true

echo "==> Re-signing app bundle..."
# macdeployqt rewrites dylib load paths, invalidating the ad-hoc signature.
# Must sign inside-out: individual binaries, then frameworks, then plugins, then app.

# Sign all .framework bundles (each is a nested code object)
find "$APP_BUNDLE/Contents/Frameworks" -name "*.framework" -maxdepth 1 -print0 | \
    while IFS= read -r -d '' fw; do
        codesign --force --sign - "$fw" 2>/dev/null || true
    done

# Sign loose dylibs in Frameworks/
find "$APP_BUNDLE/Contents/Frameworks" -maxdepth 1 -name "*.dylib" -print0 | \
    while IFS= read -r -d '' lib; do
        codesign --force --sign - "$lib" 2>/dev/null || true
    done

# Sign all plugin dylibs
find "$APP_BUNDLE/Contents/PlugIns" -name "*.dylib" -print0 | \
    while IFS= read -r -d '' lib; do
        codesign --force --sign - "$lib" 2>/dev/null || true
    done

# Sign main executable and the bundle itself
codesign --force --sign - "$APP_BUNDLE/Contents/MacOS/shijima-qt"
codesign --force --sign - "$APP_BUNDLE"

echo "==> Verifying signature..."
codesign --verify --deep --strict "$APP_BUNDLE" && echo "    Signature OK" || echo "    Warning: signature verification had issues (may still run)"

echo "==> Creating DMG..."
rm -f "$DMG_NAME"
hdiutil create \
    -volname "$APP_NAME" \
    -srcfolder "$APP_BUNDLE" \
    -ov -format UDZO \
    "$DMG_NAME"

echo "==> Installing to ${INSTALL_DIR}..."
if [ -d "${INSTALL_DIR}/${APP_NAME}.app" ]; then
    echo "    Removing existing installation..."
    rm -rf "${INSTALL_DIR}/${APP_NAME}.app"
fi
cp -R "$APP_BUNDLE" "${INSTALL_DIR}/"
echo "==> Installed to ${INSTALL_DIR}/${APP_NAME}.app"

echo ""
echo "Done!"
echo "  DMG: ${SCRIPT_DIR}/${DMG_NAME}"
echo "  App: ${INSTALL_DIR}/${APP_NAME}.app"
