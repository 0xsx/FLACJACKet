
bin_PROGRAMS	= flacjacket

flacjacket_SOURCES = \
  src/flacjacket.c \
  src/server.c \
  src/logging.c \
  src/http_sends.c \
  src/sddp_sends.c

flacjacket_CPPFLAGS	= -I./src

flacjacket_LDFLAGS	= @LDFLAGS@
flacjacket_LDADD	= -lm -luuid -ljack -lpthread -lFLAC


