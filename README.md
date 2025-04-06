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
- `autowaybar -m mon:DP-2`: Will hide waybar in the DP-2 monitor only. 

### Build
In order to build it:
```bash
xmake f -m release && xmake
```
or manually:
```bash
g++ src/*.cpp src/*.hpp src/*.h -o autowaybar -lfmt -ljsoncpp -O3 -march=native
```
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
# waybar start OR restart 
bind=$mainMod, W, exec, if ! pgrep waybar; then waybar & else killall -SIGUSR2 waybar & fi
#autohide with autowaybar
bind=$mainMod, A, exec, if ! pgrep autowaybar; then autowaybar -m all & fi
bind=$mainMod SHIFT, A, exec, killall -SIGTERM autowaybar
```
### Know your monitors and their names for multi-monitor
```bash
hyprctl monitors | grep Monitor
Monitor eDP-1 (ID 0):
```
