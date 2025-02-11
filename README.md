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


### Features
- `autowaybar -m all`: Will hide waybar in all monitors until your mouse reaches the top of any of the screens.
- `autowaybar -m unfocused`: Will hide waybar in the focused monitor only. (*check Warning*)

### Build
In order to build it
```bash
xmake
xmake run autowaybar arguments [args]
```
