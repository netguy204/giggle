import struct

norm_coord_scale = 2 ** 15
coord_scale = 2 ** 15

short_packing = '>h'
ushort_packing = '>H'
long_packing = '>i'
fstring_packing = '32s'

def normfloat2short(fl):
    return fl * norm_coord_scale

def float2long(fl):
    return fl * norm_coord_scale

def write_short(f, sh):
    f.write(struct.pack(short_packing, sh))

def write_ushort(f, sh):
    f.write(struct.pack(ushort_packing, sh))

def write_ushorts(f, shs):
    f.write(struct.pack('>' + 'H' * len(shs), *shs))

def write_long(f, l):
    f.write(struct.pack(long_packing, l))

def write_norm_float(f, fl):
    write_short(f, normfloat2short(fl))

def write_float(f, fl):
    write_long(f, float2long(fl))

def write_fstring(f, s):
    f.write(struct.pack(fstring_packing, s.encode("utf8")))

sprite_packing = '>HHHHHH32s'

def pack_sprite(w, h, u0, v0, u1, v1, name):
    u0 = normfloat2short(u0)
    v0 = normfloat2short(v0)
    u1 = normfloat2short(u1)
    v1 = normfloat2short(v1)

    return struct.pack(sprite_packing, w, h, u0, v0, u1, v1, name)

def attr(dom, name, default=None):
    if dom.hasAttribute(name):
        return dom.getAttribute(name)
    else:
        return default
