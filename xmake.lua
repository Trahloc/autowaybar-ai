add_rules("mode.debug", "mode.release")
add_requires("fmt", "jsoncpp")

target("autowaybar")
    set_kind("binary")
    add_files("src/*.cpp")
    add_packages("fmt", "jsoncpp")

includes("@builtin/xpack")
xpack("autowaybar")
    set_version("0.0.1")
    set_homepage("https://github.com/Direwolfesp/auto-waybar-cpp/")
    add_targets("autowaybar")
    set_formats("zip", "targz")
