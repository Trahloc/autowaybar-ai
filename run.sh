program_name="main.cpp"
output="auto_waybar.o"

cd src/
g++ main.cpp waybar.cpp waybar.hpp utils.hpp -o $output -std=c++23 -lfmt -ljsoncpp && ./$output
# g++ $program_name -ljsoncpp -o $output -std=c++23 -O3 && ./$output 

