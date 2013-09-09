#!/usr/bin/env python

from PIL import Image
import freetype
import sys
import os

def draw_bitmap(image, bitmap, x, y):
    x_max = x + bitmap.width
    y_max = y + bitmap.rows
    p = 0
    for p,i in enumerate(range(x,x_max)):
        for q,j in enumerate(range(y,y_max)):
            ch = int(bitmap.buffer[q * bitmap.width + p])
            pixel = (255, 255, 255, ch)
            image.putpixel((i,j), pixel)

def char_data(face, ch):
    face.load_char(ch)
    bitmap = face.glyph.bitmap
    im = Image.new('RGBA', (bitmap.width, bitmap.rows))
    draw_bitmap(im, bitmap, 0, 0)
    return {'image': im,
            'xadvance': face.glyph.advance.x/64,
            'yadvance': face.glyph.advance.y/64,
            'xlead': face.glyph.bitmap_left,
            'ylead': -face.glyph.bitmap.rows + face.glyph.bitmap_top}


if __name__ == '__main__':
    fname = sys.argv[1]
    ptsz = int(sys.argv[2])
    outprefix = sys.argv[3]
    outfont = sys.argv[4]

    face = freetype.Face(fname)
    face.set_char_size(ptsz * 64)
    height = face.height / 64

    chars = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.!?,\'"'

    sheet_path, font_name = os.path.split(outprefix)
    _, sheet_name = os.path.split(sheet_path)
    sheet_name, _ = os.path.splitext(sheet_name)

    print font_name, sheet_name

    f = open(outfont, 'w')
    f.write("""
local function %s_font(scale)
   scale = scale or 1
   local characters = %s
   local font = world:create_object('Font')
   font:load(world:atlas('resources/%s'), '%s', characters)
   font:scale(scale)
   font:line_separation(%d)
    """ % (font_name, repr(chars), sheet_name, font_name, height))

    for ii, ch in enumerate(chars):
        data = char_data(face, ch)
        data['image'].save('%s%d.png' % (outprefix, ii+1))
        f.write("""
   font:set_char_xadvance(%s, %d)
   font:set_char_yadvance(%s, %d)
   font:set_char_xlead(%s, %d)
   font:set_char_ylead(%s, %d)
""" % (repr(ch), data['xadvance'], repr(ch), data['yadvance'],
       repr(ch), data['xlead'], repr(ch), data['ylead']))

    f.write("""
   return font
end

return %s_font
""" % font_name)
