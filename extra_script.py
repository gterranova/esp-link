Import("env", "projenv")

from os.path import isdir, join
import os

platform = env.PioPlatform()
FRAMEWORK_DIR = platform.get_package_dir("framework-esp8266-nonos-sdk")
assert isdir(FRAMEWORK_DIR)

env.Execute(" ".join([
    "python tools/makeld.py",
    "4", #"${__get_flash_size(__env__)}",
    "qio",
    "80", #"${__get_board_f_flash(__env__)}",
    "new",
    FRAMEWORK_DIR, 
    "$BUILD_DIR"
]))
"""
def copy_sections():

    return [env.VerboseAction(" ".join(["${OBJCOPY}", "--only-section", section, "-O binary", "$SOURCE", join("$BUILD_DIR", "eagle.app.v6.%s.bin" % section.replace('.',''))]),
        "Copy section {0} to eagle.app.v6.{1}.bin".format(section, section.replace('.',''))) 
        for section in [".irom0.text", ".text", ".data", ".rodata"]]

env.AddPostAction(
    "$BUILD_DIR/firmware.bin",
    copy_sections() 
)
"""
"""
env.AddPostAction(
    "$BUILD_DIR/firmware.bin",
    env.VerboseAction(" ".join([
        "python",
        join("tools", "gen_appbin_py3.py"),
        "$SOURCE",
        "2",
        "0", # BOARD_FLASH_MODE 
        "15", #"${__get_board_f_flash(__env__)}",
        "4", #"${__get_flash_size(__env__)}",
        "1"
    ]), "Generating user1.bin")
)

env.AddPostAction(
    "$BUILD_DIR/firmware.bin",
    env.VerboseAction(" ".join([
        "python",
        join("tools", "gen_appbin_py3.py"),
        "$SOURCE",
        "2",
        "0", # BOARD_FLASH_MODE 
        "15", #"${__get_board_f_flash(__env__)}",
        "4", #"${__get_flash_size(__env__)}",
        "2"
    ]), "Generating user2.bin")
)
"""
