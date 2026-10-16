#!/usr/bin/env python3
"""Render DATA arcade screens with firmware code and native bundled fonts.

Requires g++ and Pillow. Outputs 480x272 software reference images, not device
captures. The host primitive rasterizer can differ slightly from Arduino_GFX.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('/tmp/pipboy-games-preview'))
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='pipboy-games-') as temporary:
        temporary = Path(temporary)
        executable = temporary/'arcade-preview'
        subprocess.run([os.environ.get('CXX','g++'), '-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-I'+str(ROOT/'include'), '-I'+str(ROOT/'lib/Arcade/src'),
                        str(ROOT/'test/host/arcade_preview.cpp'), str(ROOT/'lib/Arcade/src/Arcade.cpp'),
                        str(ROOT/'lib/Arcade/src/ArcadeRender.cpp'), '-o',str(executable)],check=True)
        subprocess.run([str(executable),str(temporary)],check=True)
        sheet = Image.new('RGB',(960,272*5))
        for row, phase in enumerate(('menu','ready','play','paused','over')):
            for column, game in enumerate(('breaker','patrol')):
                name = game+'-'+phase
                image = Image.open(temporary/(name+'.ppm'))
                image.save(output/(name+'.png'))
                sheet.paste(image,(column*480,row*272))
        sheet.save(output/'contact-sheet.png')
    print('Native game previews written to '+str(output))
