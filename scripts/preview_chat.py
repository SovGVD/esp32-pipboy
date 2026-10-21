#!/usr/bin/env python3
"""Render RADIO chat screens with firmware code and native bundled fonts.

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
SCREENS = ('chat-typing', 'chat-shift', 'chat-all', 'chat-empty', 'chat-full')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=Path('/tmp/pipboy-chat-preview'))
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='pipboy-chat-') as temporary:
        temporary = Path(temporary)
        executable = temporary/'chat-preview'
        subprocess.run([os.environ.get('CXX','g++'), '-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror',
                        '-I'+str(ROOT/'include'), '-I'+str(ROOT/'lib/Chat/src'),
                        str(ROOT/'test/host/chat_test.cpp'), str(ROOT/'lib/Chat/src/Chat.cpp'),
                        str(ROOT/'lib/Chat/src/ChatRender.cpp'), '-o',str(executable)],check=True)
        subprocess.run([str(executable),str(temporary)],check=True)
        sheet = Image.new('RGB',(960,272*3))
        for index, name in enumerate(SCREENS):
            image = Image.open(temporary/(name+'.ppm'))
            image.save(output/(name+'.png'))
            sheet.paste(image,((index%2)*480,(index//2)*272))
        sheet.save(output/'contact-sheet.png')
    print('Native chat previews written to '+str(output))
