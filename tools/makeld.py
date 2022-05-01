#!/bin/python3

import sys
import re, shutil, tempfile
from os.path import join, basename, sep

def freqdiv(spi_speed):
    if spi_speed == "26.7":
        return 1
    elif spi_speed == "20":
        return 2
    elif spi_speed == "80":
        return 15
    return 0

def mode(spi_mode):
    if spi_mode.upper() == "QOUT":
        return 1
    elif spi_mode.upper() == "DIO":
        return 2
    elif spi_mode.upper() == "DOUT":
        return 3
    return 0

def config(spi_size_map, spi_mode, spi_speed, app, boot, LD_DIR) -> dict:
    defaults = { 
        "size_map": 0, 
        "flash": 512, 
        "addr": "0x01000" if app != 2 else "0x41000" , 
        "app": app, 
        "LD_FILE": LD_DIR+"/eagle.app.v6.ld",
        "mode": spi_mode,
        "freqdiv": spi_speed }

    if (spi_size_map == 0 or spi_size_map == 7 or spi_size_map > 9):
        return defaults

    ret = { "size_map": spi_size_map, 'addr': "0x01000", "app": app }
    ret["flash"] = [512, 256, 1024, 2048, 4096, 2048, 4096, 0, 8192, 16384][spi_size_map]
    
    if app == 2:
        if spi_size_map < 5:
            ret["addr"] = "0x81000"
        elif spi_size_map >= 5:
            ret["addr"] = "0x101000"

    if boot is not None and app != 0:
        if spi_size_map >= 5:
            ret["LD_FILE"] = "{LD_DIR}/eagle.app.v6.{boot}.2048.ld".format(boot=boot, LD_DIR=LD_DIR)
        elif spi_size_map >= 2:
            ret["LD_FILE"] = "{LD_DIR}/eagle.app.v6.{boot}.1024.app{app}.ld".format(boot=boot, LD_DIR=LD_DIR, app=app)
        elif spi_size_map == 0:
            ret["LD_FILE"] = "{LD_DIR}/eagle.app.v6.{boot}.512.app{app}.ld".format(boot=boot, LD_DIR=LD_DIR, app=app)
        ret["BIN_NAME"] = "user{app}.{flash}.{boot}.{size_map}".format(boot=boot, **ret)
    defaults.update(ret)
    return defaults

def sed_inplace(filename, patterns, outfilename):
    '''
    Perform the pure-Python equivalent of in-place `sed` substitution: e.g.,
    `sed -i -e 's/'${pattern}'/'${repl}' "${filename}"`.
    '''
    # For efficiency, precompile the passed regular expression.
    patterns_compiled = [re.compile(pattern[0], re.I) for pattern in patterns]

    # For portability, NamedTemporaryFile() defaults to mode "w+b" (i.e., binary
    # writing with updating). This is usually a good thing. In this case,
    # however, binary writing imposes non-trivial encoding constraints trivially
    # resolved by switching to text writing. Let's do that.
    with tempfile.NamedTemporaryFile(mode='w', delete=False) as tmp_file:
        with open(filename) as src_file:
            for line in src_file:
                for (idx, pattern) in enumerate(patterns_compiled):
                    if (pattern.search(line)):
                        #print(line.strip(), pattern.sub(patterns[idx][1], line))
                        line = pattern.sub(patterns[idx][1], line)
                tmp_file.write(line)

    # Overwrite the original file with the munged temporary file in a
    # manner preserving file attributes (e.g., permissions).
    shutil.copystat(filename, tmp_file.name)
    shutil.move(tmp_file.name, outfilename)

def mklds(spi_size_map, spi_mode, spi_speed, app, boot, ld_dir, output_dir):
    appconfig = config(spi_size_map, spi_mode, spi_speed, app, boot, ld_dir)
    if app==0:
        outfile = join(output_dir, basename(appconfig["LD_FILE"])).replace('/', sep)
        patterns = []
    else:
        outfile = join(output_dir, basename(appconfig["LD_FILE"])).replace('/', sep)

        # Do it for Johnny.
        patterns = [
            (r"len\s*=\s*0x6B000", 'len = 0x7C000'), 
            (r"_irom0_text_end = ABSOLUTE\(\.\);", '. = ALIGN (4);\n    *(.espfs)\n    _irom0_text_end = ABSOLUTE(.);')
        ]
    sed_inplace(appconfig["LD_FILE"].replace('/', sep), patterns, outfile)

    return outfile

if __name__=='__main__':
    print(len(sys.argv), sys.argv)
    if len(sys.argv) != 8:
        print('Usage: makeld.py spi_size_map spi_mode spi_speed boot app sdk_dir output_dir')
        sys.exit(0)

        """LD_DIR = 'sdk/ESP8266_NONOS_SDK/ld'
        output_dir = 'build'
        SPI_SIZE_MAP = 4
        SPI_MODE = "DIO"
        SPI_SPEED = "80"
        BOOT = 'new'
        APP = 1"""

    boot = sys.argv[1]
    spi_mode = sys.argv[2]
    spi_speed = sys.argv[3]
    spi_size_map = int(sys.argv[4])
    app = int(sys.argv[5])
    ld_dir = join(sys.argv[6], "ld")
    output_dir = sys.argv[7]
    print(mklds(spi_size_map, spi_mode, spi_speed, app, boot, ld_dir, output_dir))
