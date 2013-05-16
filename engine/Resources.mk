# plain (non-tile) sprites whose ordering and dimensions don't matter
SPRITE_PSDS=$(wildcard sprites/*.psd)
SPRITE_PNGS_FROM_PSD=$(patsubst %.psd, %.png, $(SPRITE_PSDS))
SPRITE_PNGS_FROM_PNG=$(wildcard sprites/*.png)
SPRITE_PNGS=$(sort $(SPRITE_PNGS_FROM_PNG) $(SPRITE_PNGS_FROM_PSD))

RESOURCE_FILES=resources/images_default.png resources/images_default.dat

testtest:
	echo $(SPRITE_PSDS)

# comment me out when building for Ouya
#PAK_O=-s mac

$(RESOURCE_FILES): $(SPRITE_PNGS)
	python tools/spritepak.py $(PAK_O) sprites/trim.txt true resources/images_default 1024 1024 $(SPRITE_PNGS)

# compiled spriter
SPRITER_SCML=$(wildcard sprites/*.scml)
SPRITER_CS=$(patsubst sprites/%.scml, resources/%.cs, $(SPRITER_SCML))

resources/%.cs: sprites/%.scml
	python tools/spriterpak.py $< $@

# tiles (must all be the same dimension and must maintain order or
# tiled will get unhappy
TILE_NAMEFILE=resources/tile_order.txt
TILE_NAMES=$(shell cat $(TILE_NAMEFILE))
TILE_PSDS=$(patsubst %, tiles/%.psd, $(TILE_NAMES))
TILE_PNGS=$(patsubst %.psd, %.png, $(TILE_PSDS))

pngs: $(SPRITE_PNGS) $(TILE_PNGS)

TILE_RESOURCES=resources/tiles.png resources/tiles.dat
$(TILE_RESOURCES): $(TILE_NAMEFILE) $(TILE_PNGS)
	python tools/spritepak.py $(PAK_O) sprites/trim.txt false resources/tiles 1024 512 $(TILE_PNGS)

pngs: $(SPRITE_PNGS)

resources: $(RESOURCE_FILES) $(TILE_RESOURCES) $(SPRITER_CS)

BUILT_RESOURCES=$(RESOURCE_FILES) $(TILE_RESOURCES)
