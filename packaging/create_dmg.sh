#!/bin/bash
set -euo pipefail

APP_NAME="MacBenchForge"
VERSION="0.1.0"
BUILD_DIR="build/bin"
DMG_NAME="${APP_NAME}_v${VERSION}_AppleSilicon.dmg"
APP_DIR="${BUILD_DIR}/${APP_NAME}.app"

echo "Creating ${APP_NAME}.app bundle..."

# Create directory structure
mkdir -p "${APP_DIR}/Contents/MacOS"
mkdir -p "${APP_DIR}/Contents/Resources"
mkdir -p "${APP_DIR}/Contents/Frameworks"

# Copy binary
cp "${BUILD_DIR}/${APP_NAME}" "${APP_DIR}/Contents/MacOS/"
# Also need llama-bench if built
if [ -f "${BUILD_DIR}/llama-bench" ]; then
    cp "${BUILD_DIR}/llama-bench" "${APP_DIR}/Contents/MacOS/"
fi

# Copy resources
cp -R frontend "${APP_DIR}/Contents/Resources/"
cp config.toml "${APP_DIR}/Contents/Resources/"
cp packaging/AppIcon.icns "${APP_DIR}/Contents/Resources/"
cp packaging/Info.plist "${APP_DIR}/Contents/"

# Fix rpaths for Homebrew libs if necessary (simplified for this script)
# In a real distribution, we'd copy libcurl and libssl into Frameworks and use install_name_tool

echo "Creating DMG..."
hdiutil create -volname "${APP_NAME}" -srcfolder "${APP_DIR}" -ov -format UDZO "${DMG_NAME}"

echo "Success! Created ${DMG_NAME}"
