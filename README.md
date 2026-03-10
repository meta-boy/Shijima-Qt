# Shijima-Qt

![Shijima-Qt running on Fedora 41](.images/Shijima-Qt-Fedora.jpg)

Cross-platform shimeji desktop pet simulator. Built with Qt6. Supports macOS, Linux and Windows.

- [Download the latest release](https://github.com/pixelomer/Shijima-Qt/releases/latest)
- [See all releases](https://github.com/pixelomer/Shijima-Qt/releases)
- [Report a bug or make a feature request](https://github.com/pixelomer/Shijima-Qt/issues)
- [Shijima homepage](https://getshijima.app)

If you'd like to support the development of Shijima, consider becoming a [sponsor on GitHub](https://github.com/sponsors/pixelomer) or [buy me a coffee](https://buymeacoffee.com/pixelomer).

## Building

If you have any problems with building Shijima-Qt, see the GitHub workflows in this repository.

### macOS

#### Using Homebrew (recommended)

1. Install [Homebrew](https://brew.sh).
2. Install build dependencies.

```bash
brew install qt@6 libarchive pkg-config cmake
```

3. Build.

```bash
PKG_CONFIG_PATH="$(brew --prefix libarchive)/lib/pkgconfig" make CONFIG=debug -j$(sysctl -n hw.ncpu)
```

> **Note:** libarchive is keg-only in Homebrew, so `PKG_CONFIG_PATH` must be set. The `release` config requires LTO support across CMake sub-builds; use `debug` for local builds.

4. Package as a signed `.app` bundle, create a DMG, and install to `/Applications`:

```bash
./package-macos.sh
```

The script handles `macdeployqt`, ad-hoc code re-signing, DMG creation, and copies the app to `/Applications`.

#### Using MacPorts

1. Install MacPorts.
2. Install build dependencies.

```bash
sudo port install qt6-qtbase qt6-qtmultimedia pkgconfig libarchive
```

3. Build.

```bash
CONFIG=release make -j8
```

### Linux

```bash
CONFIG=release make -j8
```

### Windows

A Docker image is provided to build Shijima-Qt.

```bash
docker build -t shijima-qt-dev docker-dev
docker run -e CONFIG=release --rm -v "$(pwd)":/work shijima-qt-dev bash -c 'mingw64-make -j8'
```

## Claude Code Integration

Each mascot can be bound to a Claude Code terminal session:

- **Double-click** a mascot to pick a project directory (recent projects + auto-discovered `~/.claude/projects/`)
- **Double-click again** to launch `claude` in the selected project directory
- **Double-click while terminal is running** to focus the existing terminal window
- When the terminal exits, the mascot is automatically dismissed
- **Right-click** for "Open Terminal" and "Change Project..." options
- Configure the terminal app under **Settings > Terminal application...** (supports Terminal.app, iTerm2, Kitty, Ghostty, WezTerm, or a custom command)

Auto-splitting (breeding) is disabled by default. Re-enable it via **Settings > Enable multiplication**.

## Platform Notes

### macOS

Shijima-Qt needs the Accessibility permission to access the frontmost window.

### Linux

Shijima-Qt supports KDE Plasma 6 and GNOME 46 in both Wayland and X11. To get the frontmost window, Shijima-Qt automatically installs and enables a shell plugin when started.  
- On KDE, this is transparent to the user.
- On GNOME, the shell needs to be restarted on the first run. This can be done by logging out and logging back in. Shijima-Qt will exit with an appropriate error message if this is required.
- On other desktop environments, window tracking will not be available.

### Windows

Only tested on Windows 11. May also work on Windows 10. Window tracking is supported and no extra actions should be necessary to run Shijima-Qt.
