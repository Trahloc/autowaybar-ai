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
xmake run autowaybar arguments [args]
```

## Todo:

### HIDE ALL ###
Necesito:
- saber la posicion del mouse (hyprctl)

**Accion**:
- SIGUSR1 (implementada en waybar, toggle visibility)

**Ventajas**:
- funciona en cualquier waybar

**Problema**:
- Depende de Hyprland (not really a problem)

---

### HIDE FOCUSED ###
**Necesito**:
- Saber la posicion del mouse (hyprctl)
- Saber los monitores que tienes mediante (hyprctl),
- saber donde esta el config.json y el style.css (ahora mismo ML4W).

**Accion**:
- Cambiar los outputs en tiempo real del config.jsonc
- SIGUSR2 (reload hyprland with new config)

**Problemas**:
- No tengo manera de preguntar a waybar por su config.jsonc
  por eso dependo de una busqueda cutre y no portable del config
  de ML4W. 

---
### Solucion 1
Cambiar el src de waybar para que haya un comando que exponga el
path de config.

**Desventajas**:
- Tengo que encontrar que modificar en el src
- Tengo que crear pull request y ser aceptada (improbable porque
un comando es demasiado intrusivo)
- Ahora dependo de auto-waybar y de la PR

**Ventajas**
- Ya no dependo de que se haya podido encontrar la config.json y
podria funcionar con cualquier configuracion de waybar

### Solucion 2
que SIGUSR2 (reload) printee al stdout la config y asi
poder pillarla.

**Ventajas**
- no tan intrusivo

