#!/bin/bash

# Simple script to run the Zoom SDK session join test without Qt GUI

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_QT_LIB_PATH="${SCRIPT_DIR}/src/lib/zoom_video_sdk/qt_libs/Qt/lib"
SDK_LIB_PATH="${SCRIPT_DIR}/src/lib/zoom_video_sdk"

# Set library paths to prioritize SDK libraries
export LD_LIBRARY_PATH="${SDK_QT_LIB_PATH}:${SDK_LIB_PATH}:${LD_LIBRARY_PATH}"

echo "Running Simple Zoom SDK Session Join Test:"
echo "LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
echo ""

# Run the simple join test
"${SCRIPT_DIR}/src/bin/simple_join" "$@"
