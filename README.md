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
Just run `./build.sh` and go to ./build directory and launch `./autowaybar`.

### Bugs
This might be a skill issue but I dont know how to prevent waybar from spawning child processes whenever a SIGUSR1/2 signal is made. Any help would be apreciated.
