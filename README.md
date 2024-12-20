### Description
Small program that allows various modes for waybar in Hyprland. 
### Requirements
```
Hyprland
waybar
g++    // C++ 17 or later
fmt     
jsoncpp 
``` 

> [!Warning]
> As this is just a personal use program I made. Some features such as `-m unfocused` are tighly attached to ML4W dotfiles, which are the ones I use. So feel free to tweak this toy program to your like :)

### Features
- `autowaybar -m all`: Will hide waybar in all monitors until your mouse reaches the top of any of the screens.
- `autowaybar -m unfocused`: Will hide waybar in the focused monitor only. (*check Warning*)

### Build
In order to build it
```bash
# You can do: 
make build
./build/autowaybar -m [mode]

# or 
make run -m [mode]
```
