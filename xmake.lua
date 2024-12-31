add_rules("mode.debug", "mode.release")
add_requires("fmt", "jsoncpp")

target("auto-waybar-cpp")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("fmt", "jsoncpp")
