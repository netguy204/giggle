#!/usr/bin/env python

import xml.dom.minidom
import os
import sys
import gzip
import util
import struct
import tsx2dat

# bitmask properties. This must be kept in sync with the enum in
# tiles.h. This is a bitmask so the values must always be powers of
# two
bitmask_properties = {
    'collidable': 1,
    'visible': 2,
    'passable': 4
}

def elements(dom, name):
    return dom.getElementsByTagName(name)

def element(dom, name):
    return elements(dom, name)[0]

def text(dom):
    tchunks = []

    for child in dom.childNodes:
        if child.nodeType == child.TEXT_NODE:
            tchunks.append(child.data.strip().rstrip())
    return ''.join(tchunks)

def tmxdata(data):
    encoding = util.attr(data, 'encoding', None)
    compression = util.attr(data, 'compression', None)

    textdata = text(data)
    if encoding: textdata = textdata.decode(encoding)
    if compression: textdata = textdata.decode(compression)
    n = len(textdata) / 4
    return struct.unpack('<' + 'I'*n, textdata)

def parse_tileset(tileset, basepath, projroot, outdir):
    source = util.attr(tileset, 'source', None)
    firstgid = int(util.attr(tileset, 'firstgid'))

    if source:
        source = os.path.join(basepath, source)
        return (firstgid, tsx2dat.tsx2resources(source, projroot, outdir))
    else:
        return (firstgid, tsx2dat.tileset2resources(tileset, basepath, projroot, outdir))

def tileid2tileset(tilesets, tileid):
    # find the greatest global id that is still less than or equal to
    # tileid
    match = None
    for tileset in tilesets:
        firstid = tileset[0]
        if firstid <= tileid:
            if (not match) or firstid > match[0]:
                match = tileset

    if match:
        tid = tileid - match[0]
        name = match[1][0]
        props = match[1][2][tid]
        return (tid, name, props)

    if tileid == 0:
        # zero is the 'nothing' case and we'll generate a spec for it
        # automatically
        return None

    print 'couldnt file tileid %d in tilesets' % tileid
    print tilesets

if __name__ == '__main__':
    fname = sys.argv[1]
    projroot = sys.argv[2]
    outdir = sys.argv[3]

    basepath, mapname = os.path.split(fname)
    mapname, _ = os.path.splitext(mapname)

    with open(fname) as f:
        dom = xml.dom.minidom.parse(f)

    mapdom = element(dom, 'map')
    tilesets = elements(mapdom, 'tileset')
    for ii in range(len(tilesets)):
        tilesets[ii] = parse_tileset(tilesets[ii], basepath, projroot, outdir)

    width = int(util.attr(mapdom, 'width'))
    height = int(util.attr(mapdom, 'height'))
    tile_width = int(util.attr(mapdom, 'tilewidth'))
    tile_height = int(util.attr(mapdom, 'tileheight'))

    layers = elements(mapdom, 'layer')
    tileid2entity = {}

    for ii in range(len(layers)):
        layer = layers[ii]

        data = element(layer, 'data')


        flipped_tiles = tmxdata(data)

        # flip the map vertically to fit the shape that we want to be
        # loading
        tiles = []
        for jj in range(height):
            row = height - jj - 1
            start = row * width
            tiles.extend(flipped_tiles[start:(start + width)])

        name = util.attr(layer, 'name', str(ii))

        layers[ii] = (name, width, height, tiles)

        for tileid in tiles:
            if not (tileid in tileid2entity):
                tileset = tileid2tileset(tilesets, tileid)
                if tileset: tileid2entity[tileid] = tileset


    for ii in range(len(layers)):
        layer = layers[ii]
        layername = os.path.join(outdir, "%s_%s.tdt" % (mapname, layer[0]))
        print 'writing %s' % layername

        with open(layername, "wb") as f:
            # first, write out the map dimensions
            util.write_ushort(f, layer[1])
            util.write_ushort(f, layer[2])
            util.write_ushort(f, tile_width)
            util.write_ushort(f, tile_height)

            # write out the tilespecs, empty strings correspond to
            # empty tiles since the input data can be sparse but the
            # output data must be dense. Also we generate the bitmask
            # for the spec here based on the properties encoded in the
            # tilemap
            def get_properties(idx):
                if idx in tileid2entity:
                    return tileid2entity[idx][2]

            def get_property(idx, name):
                props = get_properties(idx)
                if props and (name in props): return props[name]
                return None

            def is_property_set(idx, name):
                prop = get_property(idx, name)
                return prop and prop == 'true'

            nspecs = max(tileid2entity.keys()) + 1
            util.write_ushort(f, nspecs)

            for ii in range(nspecs):
                #print ii, get_properties(ii)

                bitmask = 0
                if ii in tileid2entity:
                    spec = tileid2entity[ii]
                    util.write_fstring(f, spec[1].encode('ascii'))
                    util.write_fstring(f, "%d" % spec[0])

                    # seems like an obvious default
                    bitmask = bitmask_properties['visible']
                    for k,v in bitmask_properties.items():
                        if is_property_set(ii, k):
                            #print 'setting %s on tile %d' % (k, ii)
                            bitmask |= v
                else:
                    # fill in a sparse gap
                    util.write_fstring(f, '')
                    util.write_fstring(f, '')

                # write the bitmask last
                util.write_ushort(f, bitmask)

            # finally, write out the map data
            util.write_ushorts(f, layer[3])
