### Description
**Fork of [autowaybar](https://github.com/Direwolfesp/autowaybar)** - A program that automatically hides waybar based on cursor position for Hyprland.

**Original work by [@Direwolfesp](https://github.com/Direwolfesp)** - all credit for the core functionality belongs to them.

This fork has been refactored to follow strict anti-kruft engineering principles: simplicity over cleverness, direct solutions over abstractions, and focused functions that do one thing well.

> **Note**: This is a fork of the original project. Please consider supporting the original author [@Direwolfesp](https://github.com/Direwolfesp) by starring their repository.

> [!Warning]
> In order to work, it must follow these constraints: exactly 1 Waybar process must be running with only 1 Bar. And Waybar **must** have been launched providing **full paths** for its config file. (ie. `waybar -c ~/.config/waybar/config`)

### Features
- **Auto-hide waybar**: Hides waybar when mouse moves away from top of screen
- **Multiple modes**: Hide all monitors, focused monitor only, or specific monitors
- **Mouse activation**: Shows waybar when mouse reaches top of screen
- **Workspace awareness**: Temporarily shows waybar on workspace changes
- **Crash protection**: Automatically restarts waybar if it crashes
- **Minimal dependencies**: Only requires fmt and jsoncpp

### Requirements
All deps except xmake are included with waybar.
```
xmake
waybar
g++    // C++ 20 or later
fmt     
jsoncpp 
``` 


### Usage Modes
- `autowaybar -m all`: Hide waybar on all monitors until mouse reaches top of any screen
- `autowaybar -m focused`: Hide waybar on focused monitor only
- `autowaybar -m mon:DP-2`: Hide waybar on specific monitor (DP-2)
- `autowaybar -m mon:DP-2,HDMI-1`: Hide waybar on multiple monitors

### Options
- `-t, --threshold`: Threshold in pixels (default: 50, range: 1-1000)
- `-v, --verbose`: Enable verbose output (-v for LOG, -vv for TRACE)
- `-h, --help`: Show help message 

### Installation

#### **Quick Start (Recommended)**
```bash
# Clone and build
git clone https://github.com/Trahloc/autowaybar-ai.git
cd autowaybar-ai

# Build and install
./build.sh build && ./build.sh install
```

#### **Build Script Options**
```bash
# Check dependencies
./build.sh deps

# Build release version (default)
./build.sh build

# Build debug version
./build.sh build debug

# Install to user directory (~/.local/bin)
./build.sh install

# Install to system (/usr/local/bin) - requires sudo
./build.sh install system

# Test the binary
./build.sh test

# Clean build artifacts
./build.sh clean

# Show all options
./build.sh help
```

#### **Arch Linux Package**
```bash
# Build and install package
git clone https://github.com/Trahloc/autowaybar-ai.git
cd autowaybar-ai
makepkg -s
sudo pacman -U autowaybar-ai-*.pkg.tar.zst
```

### Build Script Features
The included `build.sh` script provides:
- **Dependency checking** - Verifies all required tools and libraries
- **Multiple build modes** - Release and debug builds
- **Easy installation** - System or user installation
- **Testing** - Verify the binary works correctly
- **Cleaning** - Remove build artifacts
- **Cross-platform** - Works on Arch, Ubuntu, Fedora, etc.
- **Colored output** - Clear status messages
- **Error handling** - Stops on any error

#### **Verification**
After installation, verify it works:
```bash
# Test the binary
./build.sh test

# Or manually check
autowaybar --help
```

#### **Troubleshooting**
Common issues and solutions:

**Build script fails**
```bash
# Check dependencies
./build.sh deps

# Install missing dependencies
sudo pacman -S fmt jsoncpp xmake gcc
# Or on Ubuntu/Debian
sudo apt install libfmt-dev libjsoncpp-dev build-essential
```

**"autowaybar: command not found"**
```bash
# Reinstall to user directory
./build.sh install

# Or add to PATH
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.bashrc
source ~/.bashrc
```

**"Waybar is not running"**
```bash
# Start waybar first
waybar -c ~/.config/waybar/config &
# Then run autowaybar
autowaybar -m all
```

**"This tool ONLY supports Hyprland"**
```bash
# Make sure you're running Hyprland
echo $XDG_SESSION_DESKTOP
# Should show "Hyprland"
```

### Development
```bash
# Build and run
xmake f -m release && xmake
xmake run autowaybar -m all

# Install to system
xmake install --admin
```
### Sample bind config for waybar & autowaybar in hyprland.conf
```bash
# waybar start OR restart (using XDG config path)
bind=$mainMod, W, exec, if ! pgrep waybar; then waybar -c ~/.config/waybar/config & else killall -SIGUSR2 waybar & fi
#autohide with autowaybar
bind=$mainMod, A, exec, if ! pgrep autowaybar; then autowaybar -m all & fi
bind=$mainMod SHIFT, A, exec, killall -SIGTERM autowaybar
```
### Know your monitors and their names for multi-monitor
```bash
hyprctl monitors | grep Monitor
Monitor eDP-1 (ID 0):
```

### Credits
- **Original Author**: [@Direwolfesp](https://github.com/Direwolfesp) - [autowaybar](https://github.com/Direwolfesp/autowaybar)
- **Refactoring**: Code refactored to follow anti-kruft engineering principles
- **License**: MIT (inherited from original project)
