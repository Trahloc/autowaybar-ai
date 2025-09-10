# Release Notes - autowaybar-ai v1.1.0

## 🚀 Major Improvements

### Single Waybar Enforcement
- **Enforces "one waybar only" policy** - Automatically detects and kills multiple waybar processes
- **Prevents conflicts** - Eliminates crashes and conflicts caused by multiple waybar instances
- **Clear warnings** - Shows informative messages when multiple waybars are detected

### Fail-Fast Error Handling
- **Hyprland detection** - Fails immediately with clear message when not running in Hyprland
- **Better error messages** - All error messages now tell users exactly what's wrong and how to fix it
- **Graceful cleanup** - Handles missing config paths during shutdown without cryptic errors

### Code Quality Improvements
- **Removed dead code** - Eliminated empty functions and unused code paths
- **Cleaner codebase** - Follows ANTI-KRUFT principles of simplicity over cleverness
- **Better maintainability** - Reduced complexity while maintaining full functionality

## 🔧 Technical Changes

### New Functions
- `enforceSingleWaybar()` - Detects and kills multiple waybar processes
- Enhanced `initPidOrRestart()` - Properly counts and handles waybar processes

### Removed Code
- `register_signals()` - Empty function that served no purpose
- Duplicate crash count initialization
- Test files and debug scripts

### Updated Defaults
- **Bar threshold**: Increased from 50px to 100px for better usability
- **Error handling**: All errors now provide actionable information

## 🐛 Bug Fixes
- Fixed PID parsing for multiple waybar processes
- Fixed config file write errors during cleanup
- Fixed waybar process detection and management
- Fixed signal handling for proper cleanup

## 📦 Installation
```bash
# Build from source
git clone https://github.com/Trahloc/autowaybar-ai.git
cd autowaybar-ai
xmake f -m release && xmake
xmake install

# Or install via AUR (when available)
yay -S autowaybar-ai
```

## 🔄 Migration from v1.0.0
No breaking changes - this is a drop-in replacement with enhanced reliability and error handling.

## 🙏 Acknowledgments
- Original work by [@Direwolfesp](https://github.com/Direwolfesp)
- Enhanced following ANTI-KRUFT engineering principles
- Focus on simplicity, reliability, and clear error messages
