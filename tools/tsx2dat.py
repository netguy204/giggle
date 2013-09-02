#!/usr/bin/env python
#
# Convert Tiled TSX files into our internal sprite map "dat"
# format. The original image that Tiled is using can then be used
# directly by giggle
#

import xml.dom.minidom
import os
import sys
import util
import shutil

def tileset2resources(tileset, basedir, projroot, outdir):
    tile_width = int(tileset.getAttribute('tilewidth'))
    tile_height = int(tileset.getAttribute('tileheight'))
    tile_spacing = int(util.attr(tileset, 'spacing', 0))
    margin = int(util.attr(tileset, 'margin', 0))

    image = tileset.getElementsByTagName('image')[0]

    image_rel_name = image.getAttribute('source')
    _, image_name = os.path.split(image_rel_name)
    shutil.copyfile(os.path.join(basedir, image_rel_name), os.path.join(outdir, image_name))

    image_name, _ = os.path.splitext(image_name)

    base_outname = os.path.join(outdir, image_name)
    dat_outname = '%s.dat' % base_outname
    order_outname = '%s.order' % base_outname

    img_width = int(image.getAttribute('width'))
    img_height = int(image.getAttribute('height'))

    width_in_tiles = int(round((img_width - 2*margin) / (tile_width)))
    height_in_tiles = int(round((img_height - 2*margin) / (tile_height)))

    nentries = width_in_tiles * height_in_tiles

    with open(dat_outname, "wb") as outdat:
        util.write_short(outdat, nentries)

        for tnum in range(nentries):
            row = int(tnum / width_in_tiles)
            col = tnum % width_in_tiles

            # compute the margin impact
            xpad = margin + col * tile_spacing
            ypad = margin + row * tile_spacing

            xstart = xpad + col * tile_width
            ystart = ypad + row * tile_height
            xend = xstart + tile_width
            yend = ystart + tile_height

            u0 = float(xstart) / img_width
            v1 = float(ystart) / img_height
            u1 = float(xend) / img_width
            v0 = float(yend) / img_height

            parms = (tile_width, tile_height, u0, v0, u1, v1, "%d" % tnum)
            #print(row, col, (xstart, ystart, xend, yend), parms)
            outdat.write(util.pack_sprite(*parms))

    with open(order_outname, "w") as outnames:
        for ii in range(nentries):
            outnames.write("%d\n" % ii)

    # revise base_outname such that it's relative to what we hope to
    # the project root
    base_outname = os.path.relpath(base_outname, projroot)

    return (base_outname, nentries)

def tsx2resources(fname, projroot, outdir):
    with open(fname) as f:
        dom = xml.dom.minidom.parse(f)

    basedir, _ = os.path.split(fname)

    tileset = dom.getElementsByTagName('tileset')[0]
    return tileset2resources(tileset, basedir, projroot, outdir)

if __name__ == '__main__':
    if len(sys.argv) != 4:
        print 'usage: %s input projroot resources_out' % (sys.argv[0])
        exit(1)

    fname = sys.argv[1]
    projroot = sys.argv[2]
    outdir = sys.argv[3]

    tsx2resources(fname, projroot, outdir)
