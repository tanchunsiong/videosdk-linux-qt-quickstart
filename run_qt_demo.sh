#!/bin/bash

# Wrapper script to run Qt VideoSDK demo with proper library paths
# This ensures the application uses the SDK's Qt libraries instead of system ones

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_QT_LIB_PATH="${SCRIPT_DIR}/src/lib/zoom_video_sdk/qt_libs/Qt/lib"
SDK_LIB_PATH="${SCRIPT_DIR}/src/lib/zoom_video_sdk"

# Set library paths to prioritize SDK libraries
export LD_LIBRARY_PATH="${SDK_QT_LIB_PATH}:${SDK_LIB_PATH}:${LD_LIBRARY_PATH}"

# Force X11 platform to avoid Wayland issues
export QT_QPA_PLATFORM=xcb

# Also set Qt plugin path if needed
export QT_QPA_PLATFORM_PLUGIN_PATH="${SDK_QT_LIB_PATH}/../plugins/platforms:${QT_QPA_PLATFORM_PLUGIN_PATH}"

echo "Running VideoSDK Qt Demo with SDK library paths:"
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
echo "QT_QPA_PLATFORM_PLUGIN_PATH=${QT_QPA_PLATFORM_PLUGIN_PATH}"
echo ""

# Run the application
"${SCRIPT_DIR}/src/bin/VideoSDKQtDemo" "$@"
