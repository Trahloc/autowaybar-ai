program_name="main.cpp"
output="auto_waybar.o"

g++ $program_name -ljsoncpp -o $output -std=c++23 -O3 && ./$output 

