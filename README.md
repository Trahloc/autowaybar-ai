### Description
AI-enhanced version of [autowaybar](https://github.com/Direwolfesp/autowaybar) - a program that allows various modes for waybar in [Hyprland](https://github.com/hyprwm/Hyprland). 

**Original work by [@Direwolfesp](https://github.com/Direwolfesp)** - all credit for the core functionality belongs to them.

This fork adds modern C++20 RAII patterns, comprehensive input validation, security hardening, and XDG Base Directory Specification compliance through AI-assisted code improvements.

> [!Warning]
> In order to work, it must follow these constraints: exactly 1 Waybar process must be running with only 1 Bar. And Waybar **must** have been launched providing **full paths** for its config file. (ie. `waybar -c ~/.config/waybar/config`)

### Features
- **XDG Compliant**: Follows XDG Base Directory Specification for config file discovery
- **RAII Resource Management**: Automatic cleanup of file handles and process pipes
- **Security Hardened**: Input validation and command injection prevention
- **Exception Safe**: Proper error handling and resource cleanup on exceptions
- **Modern C++20**: Uses latest C++ features for better performance and safety

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
- `autowaybar -m all`: Will hide waybar in all monitors until your mouse reaches the top of any of the screens.
- `autowaybar -m focused`: Will hide waybar in the focused monitor only. 
- `autowaybar -m mon:DP-2`: Will hide waybar in the DP-2 monitor only.

### Configuration Discovery
The program follows XDG Base Directory Specification and searches for waybar config in this order:
1. Command line argument path (if waybar was started with `-c` flag)
2. `$HOME/.config/waybar/config` (XDG_CONFIG_HOME)
3. `$HOME/waybar/config` (fallback)
4. `/etc/xdg/waybar/config` (system-wide)

### Security Features
- **Input Validation**: Comprehensive validation of command arguments and thresholds
- **Command Injection Prevention**: Sanitized command execution with character filtering
- **Resource Management**: RAII patterns prevent resource leaks
- **Exception Safety**: Proper cleanup even when exceptions occur 

### Build
In order to build it:
```bash
xmake f -m release && xmake
```
or manually:
```bash
g++ -std=c++20 src/*.cpp src/*.hpp -o autowaybar -lfmt -ljsoncpp -O3 -march=native
```

### Development
The codebase uses modern C++20 features and RAII patterns:
- **RAII Wrappers**: `FileWrapper`, `OutputFileWrapper`, `ProcessPipe`, `ConfigManager`
- **Exception Safety**: All resource management is exception-safe
- **Input Validation**: Comprehensive validation prevents security issues
- **XDG Compliance**: Follows standard configuration discovery patterns
### Run
Now you can run it by doing:
```bash
xmake run autowaybar arguments [args]
```
or you can install it to your path by doing:
```bash
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

### XDG Compliance
This project follows the XDG Base Directory Specification:
- **Config Discovery**: Automatically finds waybar config in standard XDG locations
- **Environment Variables**: Respects `$HOME` and `$XDG_CONFIG_HOME`
- **Fallback Paths**: Graceful fallback to system-wide configs
- **No Hardcoded Paths**: All paths are dynamically resolved

### Security & Quality
- **Input Validation**: All user inputs are validated and sanitized
- **Resource Management**: RAII patterns ensure no resource leaks
- **Exception Safety**: Proper cleanup even when errors occur
- **Modern C++20**: Uses latest language features for better safety and performance

### AI Enhancements
This fork includes AI-assisted improvements:
- **Security Hardening**: Fixed critical vulnerabilities and added input validation
- **RAII Implementation**: Modern C++ resource management patterns
- **XDG Compliance**: Proper configuration discovery following Linux standards
- **Code Quality**: Extracted magic numbers, improved error handling, fixed typos
- **Documentation**: Enhanced README with comprehensive feature descriptions

### Credits
- **Original Author**: [@Direwolfesp](https://github.com/Direwolfesp) - [autowaybar](https://github.com/Direwolfesp/autowaybar)
- **AI Assistant**: Claude Sonnet 4 (Anthropic) - Code improvements and security enhancements
- **License**: MIT (inherited from original project)
