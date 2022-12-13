# Exit when any command fails
set -e

# Download appimage tools
wget --no-verbose -O ./ci/appimage/linuxdeployqt-continuous-x86_64.AppImage https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x ./ci/appimage/linuxdeployqt-continuous-x86_64.AppImage

# Create appdir directories
mkdir -p appdir/usr/share/applications
mkdir -p appdir/usr/bin

# Copy application related files
cp ./ci/appimage/dust3d.png appdir/dust3d.png
cp ./ci/appimage/dust3d.desktop appdir/usr/share/applications/dust3d.desktop
cp ./application/dust3d appdir/usr/bin/dust3d

# Make bundle
./ci/appimage/linuxdeployqt-continuous-x86_64.AppImage appdir/usr/share/applications/*.desktop -appimage
mv Dust3D-*-x86_64.AppImage Dust3D-x86_64.AppImage