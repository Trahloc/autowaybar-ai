output="autowaybar"

src=(
    "main.cpp"
    "waybar.hpp"
    "waybar.cpp"
    "utils.hpp"
    "utils.cpp"
)

mkdir -p build

cd src/ || exit
g++ "${src[@]}" -o $output -std=c++23 -O3 -lfmt -ljsoncpp && mv $output ../build/ 

echo "Build complete."
