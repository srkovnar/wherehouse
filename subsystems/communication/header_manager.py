"""
This is the most basic way I could think of to keep all of my header
files in one place while simultaneously having them in every folder
that has Arduino code in it.

This script copies every file in ./inc/ to every folder listed in the
"destinations" list.
"""


import os
import shutil
import sys

destinations = {
    "./esp8266_firmware/esp8266_release/",
    "./esp8266_firmware/esp8266_release_1mb/",
    "./esp8266_firmware/esp8266_setup/"
}

inc = "./inc/"


def copy_to_folders(source_directory, destination_list):
    header_files = os.listdir(source_directory)
    for f in header_files:
        for d in destination_list:
            if os.path.isdir(d):
                print("Copying", inc+f, "to", d+f)
                shutil.copyfile(inc+f, d+f)

if __name__ == "__main__":
    copy_to_folders(inc, destinations)
    sys.exit(0);
    

