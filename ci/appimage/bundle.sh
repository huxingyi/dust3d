# Exit when any command fails
set -e

# Download AppImage tools from official upstream releases
LINUXDEPLOYQT_URL="https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
APPIMAGETOOL_URL="https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
wget --no-verbose -O ./ci/appimage/linuxdeployqt.AppImage "$LINUXDEPLOYQT_URL"
wget --no-verbose -O ./ci/appimage/appimagetool.AppImage "$APPIMAGETOOL_URL"

# Create directories
mkdir -p appdir/usr/share/metainfo
mkdir -p appdir/usr/share/applications
mkdir -p appdir/usr/share/icons/hicolor/256x256/apps
mkdir -p appdir/usr/bin
mkdir -p appdir/usr/optional/libstdc++

# Print GLIBC version
ldd --version

# Print libstdc related
sudo ls /usr/lib/x86_64-linux-gnu/ | grep libstdc

# Copy libstdc++
cp /usr/lib/x86_64-linux-gnu/libstdc++.so.6 appdir/usr/optional/libstdc++/libstdc++.so.6

# Copy fontconfig (required for Qt xcb plugin)
if [ -f /usr/lib/x86_64-linux-gnu/libfontconfig.so.1 ]; then
  mkdir -p appdir/usr/lib
  cp /usr/lib/x86_64-linux-gnu/libfontconfig.so.1 appdir/usr/lib/
  cp /usr/lib/x86_64-linux-gnu/libfreetype.so.6 appdir/usr/lib/ 2>/dev/null || true
fi

# Copy application related files
cp ./ci/appimage/dust3d.png appdir/usr/share/icons/hicolor/256x256/apps/dust3d.png
cp ./ci/appimage/dust3d.appdata.xml appdir/usr/share/metainfo/dust3d.appdata.xml
cp ./ci/appimage/dust3d.desktop appdir/usr/share/applications/dust3d.desktop
cp ./application/dust3d appdir/usr/bin/dust3d
chmod a+x appdir/usr/bin/dust3d

# Verify binary was copied
if [ ! -f appdir/usr/bin/dust3d ]; then
  echo "ERROR: Binary not found at appdir/usr/bin/dust3d"
  exit 1
fi
if [ ! -x appdir/usr/bin/dust3d ]; then
  echo "ERROR: Binary is not executable at appdir/usr/bin/dust3d"
  exit 1
fi
echo "✓ Binary found at appdir/usr/bin/dust3d"
ls -lh appdir/usr/bin/dust3d

# Make bundle
chmod a+x ./ci/appimage/linuxdeployqt.AppImage
chmod a+x ./ci/appimage/appimagetool.AppImage
unset QTDIR; unset QT_PLUGIN_PATH 

# Run linuxdeployqt on the desktop file (recommended pattern from linuxdeployqt README)
# linuxdeployqt will find the binary from the Exec= field in the desktop file
./ci/appimage/linuxdeployqt.AppImage appdir/usr/share/applications/dust3d.desktop -bundle-non-qt-libs -verbose=2

# Verify binary still exists and linuxdeployqt didn't move/duplicate it
if [ ! -f appdir/usr/bin/dust3d ]; then
  echo "ERROR: Binary was removed by linuxdeployqt"
  echo "Directory structure:"
  find appdir -type f -name "dust3d*" | head -20
  exit 1
fi
echo "✓ Binary still present after linuxdeployqt at appdir/usr/bin/dust3d"

# Verify desktop file and icon are in place
if [ ! -f appdir/usr/share/applications/dust3d.desktop ]; then
  echo "ERROR: Desktop file missing"
  exit 1
fi
echo "✓ Desktop file present"

if [ ! -f appdir/usr/share/icons/hicolor/256x256/apps/dust3d.png ]; then
  echo "ERROR: Icon missing"
  exit 1
fi
echo "✓ Icon present"

# Show AppDir structure before creating AppImage
echo ""
echo "=== AppDir structure after linuxdeployqt ==="
find appdir -name "dust3d*" -o -name "AppRun" | sort
find appdir/usr/bin -type f -executable | sort

# Verify no duplicate paths
if find appdir -path "*/usr/bin/usr/bin/*" 2>/dev/null | grep -q .; then
  echo "ERROR: Duplicate path structure detected (usr/bin/usr/bin/)"
  find appdir -path "*/usr/bin/usr/bin/*"
  exit 1
fi
echo "✓ No duplicate paths detected"

# Verify AppRun created by linuxdeployqt
if [ ! -x appdir/AppRun ]; then
  echo "ERROR: AppRun missing or not executable after linuxdeployqt"
  exit 1
fi
echo "✓ AppRun present"

# Remove OpenGL/Mesa/DRI/DRM libraries — these MUST come from the system to
# match the kernel's DRM version. Bundling them (as linuxdeployqt does) causes
# the old Ubuntu 22.04 Mesa to run against a newer kernel, resulting in massive
# memory allocation and OOM kill (issue #184).
rm -f appdir/usr/lib/libGL.so* \
      appdir/usr/lib/libGLX*.so* \
      appdir/usr/lib/libEGL*.so* \
      appdir/usr/lib/libGLdispatch*.so* \
      appdir/usr/lib/libGLESv*.so* \
      appdir/usr/lib/libdrm*.so* \
      appdir/usr/lib/libvulkan*.so* \
      appdir/usr/lib/libxcb-dri2.so* \
      appdir/usr/lib/libxcb-dri3.so*
rm -rf appdir/usr/lib/dri/

# Final validation before creating AppImage
echo ""
echo "=== Final AppDir validation ==="
if [ ! -x appdir/AppRun ]; then
  echo "ERROR: AppRun is not executable"
  exit 1
fi
echo "✓ AppRun is executable"

if [ ! -f appdir/usr/share/applications/dust3d.desktop ]; then
  echo "ERROR: Desktop file not found at appdir/usr/share/applications/dust3d.desktop"
  echo "Contents of appdir/usr/share/applications/:"
  ls -lah appdir/usr/share/applications/ 2>/dev/null || echo "Directory does not exist"
  exit 1
fi
echo "✓ Desktop file found"

if [ ! -f appdir/usr/bin/dust3d ]; then
  echo "ERROR: Binary not found at appdir/usr/bin/dust3d"
  exit 1
fi
echo "✓ Binary executable found"

echo ""
echo "Desktop file contents:"
cat appdir/usr/share/applications/dust3d.desktop
echo ""

# Create AppImage with proper environment variables
echo "Creating AppImage..."
export ARCH=x86_64
export VERSION="${VERSION:-debug}"
./ci/appimage/appimagetool.AppImage -g ./appdir/ Dust3D-x86_64.AppImage

# Verify AppImage was created
if [ ! -f Dust3D-x86_64.AppImage ]; then
  echo "ERROR: AppImage creation failed"
  exit 1
fi
echo "✓ AppImage created successfully"
file Dust3D-x86_64.AppImage
