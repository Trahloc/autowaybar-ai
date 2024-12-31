### Description
Small program that allows various modes for waybar in [Hyprland](https://github.com/hyprwm/Hyprland). 
### Requirements
All deps except xmake are included with waybar.
```
xmake
waybar
g++    // C++ 17 or later
fmt     
jsoncpp 
``` 

> [!Warning]
> As this is just a personal use program I made. Some features such as `-m unfocused` are tighly attached to ML4W dotfiles structure (the one I use) so expect some bugs in that regard. Also, feel free to tweak this toy program to your like and contribute if you like:)

### Features
- `autowaybar -m all`: Will hide waybar in all monitors until your mouse reaches the top of any of the screens.
- `autowaybar -m unfocused`: Will hide waybar in the focused monitor only. (*check Warning*)

### Build
In order to build it
```bash
xmake
xmake run auto-waybar-cpp arguments [args]
```

### Todo:
As waybar doesn't allow to ask for the current loaded config path (as far as I know), it would be nice to default search for the `config.json` in the valid directories listed in the [waybar wiki](https://github.com/Alexays/Waybar/wiki/Configuration#config-file) and add and envar to check if we are in ML4W folder structure.
