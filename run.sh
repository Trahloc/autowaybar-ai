program_name="hyprland_auto_bar_v2.cpp"
output="auto_waybar"

g++ $program_name -o $output -std=c++23 -O3 && ./$output 

