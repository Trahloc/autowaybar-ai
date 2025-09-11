# Release Notes - autowaybar-ai v1.1.1

## Bug Fixes
- **Fixed double delay issue**: Resolved the problem where waybar would show for 3 seconds, flicker, then show again for another 3 seconds when switching workspaces with Super+1/2/3/4/5 keys
- **Improved workspace change handling**: Replaced atomic flag-based approach with timestamp-based idempotent handling to prevent race conditions
- **Enhanced stability**: Workspace change detection is now completely idempotent and handles rapid workspace changes gracefully

## Technical Changes
- Replaced `g_handling_workspace_change` atomic flag with `g_workspace_show_start` timestamp tracking
- Made `handleWorkspaceChange()` idempotent - multiple calls are now safe and won't cause double delays
- Simplified workspace change logic by removing complex atomic flag management
- Added time-based prevention to ignore additional workspace changes within 3 seconds of the last show

## Impact
- Eliminates the frustrating double delay behavior when switching workspaces
- Provides smoother user experience with consistent 3-second waybar display
- Maintains all existing functionality while fixing the core timing issue
