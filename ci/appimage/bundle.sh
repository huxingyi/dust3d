# Exit when any command fails
set -e

# Download appimage tools
wget --no-verbose -O ./ci/appimage/AppRun-patched-x86_64 https://github.com/huxingyi/dust3d/blob/1.0.0-rc.6/ci/AppRun-patched-x86_64?raw=true
wget --no-verbose -O ./ci/appimage/exec-x86_64.so https://github.com/huxingyi/dust3d/blob/1.0.0-rc.6/ci/exec-x86_64.so?raw=true
wget --no-verbose -O ./ci/appimage/linuxdeployqt.AppImage https://github.com/huxingyi/dust3d/blob/1.0.0-rc.6/ci/linuxdeployqt.AppImage?raw=true

# Create directories
mkdir -p appdir/usr/share/metainfo
mkdir -p appdir/usr/share/applications
mkdir -p appdir/usr/bin
mkdir -p appdir/usr/optional/libstdc++

# Print GLIBC version
ldd --version

# Print libstdc related
sudo ls /usr/lib/x86_64-linux-gnu/ | grep libstdc

# Copy libstdc++
cp /usr/lib/x86_64-linux-gnu/libstdc++.so.6 appdir/usr/optional/libstdc++/libstdc++.so.6

# Copy exec
cp ./ci/appimage/exec-x86_64.so appdir/usr/optional/exec.so

# Copy application related files
cp ./ci/appimage/dust3d.png appdir/dust3d.png
cp ./ci/appimage/dust3d.appdata.xml appdir/usr/share/metainfo/dust3d.appdata.xml
cp ./ci/appimage/dust3d.desktop appdir/usr/share/applications/dust3d.desktop
cp ./application/dust3d appdir/usr/bin/dust3d

# Make bundle
chmod a+x ./ci/appimage/linuxdeployqt.AppImage
unset QTDIR; unset QT_PLUGIN_PATH 
./ci/appimage/linuxdeployqt.AppImage appdir/usr/share/applications/dust3d.desktop -bundle-non-qt-libs -verbose=2
rm appdir/AppRun
cp ./ci/appimage/AppRun-patched-x86_64 appdir/AppRun
chmod a+x appdir/AppRun
./ci/appimage/linuxdeployqt.AppImage --appimage-extract
export PATH=$(readlink -f ./squashfs-root/usr/bin):$PATH
rm -f "./appdir/usr/lib/libxcb-dri2.so" "./appdir/usr/lib/libxcb-dri3.so"
./squashfs-root/usr/bin/appimagetool -g ./appdir/ Dust3D-x86_64.AppImage
