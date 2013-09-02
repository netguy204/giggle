#!/usr/bin/env python

import xml.dom.minidom
import os
import sys
import gzip
import util
import struct
import tsx2dat

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

def parse_tileset(tileset, basepath, outdir):
    source = util.attr(tileset, 'source', None)
    firstgid = int(util.attr(tileset, 'firstgid'))

    if source:
        source = os.path.join(basepath, source)
        return (firstgid, tsx2dat.tsx2resources(source, outdir))
    else:
        return (firstgid, tsx2dat.tileset2resources(tileset, basepath, outdir))

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
        return (tileid - match[0], match[1][0])

    if tileid == 0:
        # zero is the 'nothing' case and we'll generate a spec for it
        # automatically
        return None

    print 'couldnt file tileid %d in tilesets' % tileid
    print tilesets

if __name__ == '__main__':
    fname = sys.argv[1]
    outdir = sys.argv[2]

    basepath, mapname = os.path.split(fname)
    mapname, _ = os.path.splitext(mapname)

    with open(fname) as f:
        dom = xml.dom.minidom.parse(f)

    mapdom = element(dom, 'map')
    tilesets = elements(mapdom, 'tileset')
    for ii in range(len(tilesets)):
        tilesets[ii] = parse_tileset(tilesets[ii], basepath, outdir)

    layers = elements(mapdom, 'layer')
    tileid2entity = {}

    for ii in range(len(layers)):
        layer = layers[ii]

        data = element(layer, 'data')
        tiles = tmxdata(data)

        name = util.attr(layer, 'name', str(ii))
        width = int(util.attr(layer, 'width'))
        height = int(util.attr(layer, 'height'))

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
            # first write out the tilespecs, empty strings correspond
            # to empty tiles since the input data can be sparse but
            # the output data must be dense
            nspecs = len(tileid2entity)
            util.write_ushort(f, nspecs)
            for ii in range(nspecs):
                if ii in tileid2entity:
                    spec = tileid2entity[ii]
                    util.write_fstring(f, "%d" % spec[0])
                    util.write_fstring(f, spec[1].encode('ascii'))
                else:
                    # fill in a sparse gap
                    util.write_fstring(f, '')
                    util.write_fstring(f, '')

            # now write the map dimensions followed by the map data
            util.write_ushort(f, layer[1])
            util.write_ushort(f, layer[2])
            util.write_ushorts(f, layer[3])
