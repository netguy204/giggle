all clean distclean:
	(cd engine ; make -f SDL.mk $@)
	(cd raw_resources ; make $@)
