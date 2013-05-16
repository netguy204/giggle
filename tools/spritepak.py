#!/usr/bin/env python
#
# Format:
# num_entrys
# [ entry: W, H, u0, v0, u1, v1, name ]
#
# all big-endian, floating point numbers are converted to fixed point
# numbers by multiplying by coord_scale

from PIL import Image
import glob
import sys
import os
import struct
import util
from optparse import OptionParser

packing = '>HHHHHH32s'

def find_visible_bounds(image, pad=0):
    data = list(image.getdata())
    w, h = image.size

    def getpx(x, y):
        v = data[x + (y * w)]
        if len(v) == 4: return v
        return (v[0], v[1], v[2], 255)

    def first_hit(gen):
        first_value = None
        for v in gen:
            if not first_value: first_value = v
            x, y = v
            r, g, b, a = getpx(x, y)
            if a > 0:
                return v
        return first_value

    def top_gen():
        for y in xrange(h):
            for x in xrange(w):
                yield (x, y)

    def bottom_gen():
        for ty in xrange(h):
            for x in xrange(w):
                yield (x, (h - 1 - ty))

    def left_gen():
        for x in xrange(w):
            for y in xrange(h):
                yield (x, y)

    def right_gen():
        for tx in xrange(w):
            for y in xrange(h):
                yield ((w - 1 - tx), y)

    left, junk = first_hit(left_gen())
    right, junk = first_hit(right_gen())
    junk, top = first_hit(top_gen())
    junk, bottom = first_hit(bottom_gen())

    if left >= pad: left -= pad
    if right <= (w - 1 - pad): right += pad
    if top >= pad: top -= pad
    if bottom <= (h - 1 - pad): bottom += pad

    return (left, top, right, bottom)


def mk_sheet(filenames, outbase, tgt_dims, trim, sort, system):
    print tgt_dims
    outimagename = outbase + '.png'
    outdatname = outbase + '.dat'
    outdat = open(outdatname, 'wb')

    tgt_w, tgt_h = tgt_dims
    tgt = Image.new("RGBA", tgt_dims)
    metadata = []

    current_x = 0
    current_y = 0
    pad = 1

    max_row_h = 0
    nentries = len(filenames) * 2 # for flips
    util.write_short(outdat, nentries)

    if sort:
        # sort by height
        print 'sorting tiles for more efficient packing'
        objects = []
        for fname in filenames:
            img = Image.open(fname)
            img_w, img_h = img.size
            objects.append({'fname': fname, 'height': img_h})
        objects.sort(key=lambda x: x['height'])
        filenames = [ obj['fname'] for obj in objects ]
    else:
        print 'not sorting tiles'

    def system_for_file(fname):
        path, basename = os.path.split(fname)
        base, ext = os.path.splitext(basename)
        base, p, sys = base.partition('@')
        if p:
            return (os.path.join(path, base) + ext, sys)
        else:
            return fname, None

    # if we got a system override command then remove all
    # basefilenames that have an override
    overrides = []
    for fname in filenames:
        base, sys = system_for_file(fname)
        if system and sys == system:
            overrides.append(base)

    # filter the filenames
    temp = []
    for fname in filenames:
        if not (fname in overrides):
            temp.append(fname)
    filenames = temp

    for fname in filenames:
        img = Image.open(fname)

        fname, sys = system_for_file(fname)
        junk, basename = os.path.split(fname)

        if sys and sys != system:
            continue # skip

        if basename in trim:
            img = img.crop(find_visible_bounds(img))

        img_w, img_h = img.size

        def state_str():
            return '"%s" (%d, %d)' % (fname, img_w, img_h)

        if current_x + img_w > tgt_w:
            if max_row_h == 0:
                raise Exception('%s: nothing fits' % state_str())

            current_y += max_row_h + pad
            current_x = 0

            max_row_h = 0

        if current_y + img_h > tgt_h:
            raise Exception('%s: out of vertical space' % state_str())

        max_row_h = max(max_row_h, img_h)

        # finally, insert the image
        tgt.paste(img, (current_x, current_y))

        u0 = util.normfloat2short(float(current_x) / tgt_w)
        v1 = util.normfloat2short(float(current_y) / tgt_h)
        u1 = util.normfloat2short(float(current_x + img_w) / tgt_w)
        v0 = util.normfloat2short(float(current_y + img_h) / tgt_h)
        pakname, _ = os.path.splitext(basename)

        struct_tuple = (img_w, img_h, u0, v0, u1, v1, pakname)
        packed = struct.pack(packing, *struct_tuple)
        outdat.write(packed)

        # flipped in the x direction
        struct_tuple = (img_w, img_h, u1, v0, u0, v1, "/x" + pakname)
        packed = struct.pack(packing, *struct_tuple)
        outdat.write(packed)

        current_x += img_w + pad

    tgt.save(outimagename)
    outdat.close()
    print "%d entries" % nentries
    print "size per entry: %d" % struct.calcsize(packing)

if __name__ == '__main__':
    if len(sys.argv) < 5:
        print 'usage: %s trimfile sort outprefix width height [input files]' % sys.argv[0]
        exit(1)

    parser = OptionParser()
    parser.add_option("-s", "--system", dest="system")
    (options, args) = parser.parse_args()

    trimfile = args[0]
    sort = args[1]
    outfile = args[2]
    width = int(args[3])
    height = int(args[4])
    infiles = args[5:]

    with open(trimfile) as f:
        trim = [ name.rstrip() for name in f ]

    mk_sheet(infiles, outfile, (width, height), trim, sort == 'true', options.system)
