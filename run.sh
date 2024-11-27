output="auto_waybar.o"

src=(
    "main.cpp"
    "waybar.hpp"
    "waybar.cpp"
    "utils.hpp"
    "utils.cpp"
)

cd src/
g++ "${src[@]}" -o $output -std=c++23 -O3 -lfmt -ljsoncpp && ./$output

