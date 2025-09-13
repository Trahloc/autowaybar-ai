# Release Notes - autowaybar-ai v1.1.2

## Performance Improvements
- **Reduced workspace switch delay**: Changed from 3 seconds to 1 second for faster UI response
- **Fixed flickering on rapid workspace changes**: Eliminated visual glitches when switching workspaces quickly (e.g., Super+1, Super+2, Super+3 in succession)

## Signal Handling & Cleanup
- **Proper Ctrl-C handling**: Implemented graceful shutdown on SIGINT/SIGTERM signals
- **Clean waybar termination**: Waybar processes are now properly terminated on shutdown instead of being left running
- **Config restoration**: Original waybar configuration is restored on program exit
- **Resource cleanup**: PID files and waybar processes are handled correctly during shutdown

## Technical Changes
- Modified `WORKSPACE_SHOW_DURATION` constant from 3s to 1s (1000ms)
- Fixed workspace change timer logic to prevent multiple timers from interfering
- Added `shutdown()` method to Waybar class for proper process termination
- Improved signal handler to set interrupt flag instead of immediate exit
- Made `g_interrupt_request` globally accessible for proper signal handling
- Fixed lambda capture warning for static variable in workspace change handler

## Bug Fixes
- **Workspace change flickering**: Rapid workspace switches no longer cause waybar to flicker or show multiple times
- **Timer cancellation**: Older workspace change timers are properly cancelled when new changes occur
- **Signal handling**: Ctrl-C now triggers graceful shutdown instead of abrupt termination

## Impact
- Faster workspace switching experience with 1-second delay
- Smoother UI behavior during rapid workspace changes
- Clean shutdown process that properly restores system state
- No more orphaned waybar processes after program termination
