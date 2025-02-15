### Description
Small program that allows various modes for waybar in [Hyprland](https://github.com/hyprwm/Hyprland). 

> [!Warning]
> In order to work, it must follow these constrains: exactly 1 Waybar process must be running with only 1 Bar. And Waybar **must** have been launched providing **full paths** for its config file. (ie. `waybar -c ~/waybar/myconfig`)

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
- `autowaybar -m focused`: Will hide waybar in the focused monitor only. 

### Build
In order to build it
```bash
xmake
xmake run autowaybar arguments [args]
```
or manually
```bash
g++ src/*.cpp src/*.hpp src/*.h -o autowaybar -lfmt -ljsoncpp -O3 -march=native
``` 
