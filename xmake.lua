add_rules("mode.debug", "mode.release")
add_requires("fmt", "jsoncpp")

set_languages("c++20")

target("autowaybar")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("fmt", "jsoncpp")
    
    add_cxxflags("-Wall", "-Wextra")
    
    if is_mode("release") then
        add_cxxflags("-O3", "-DNDEBUG")
    end
    
    if is_mode("debug") then
        add_cxxflags("-g", "-O0", "-DDEBUG")
    end
