CROSS_COMPILE=mipsel-openwrt-linux-
CC = ${CROSS_COMPILE}gcc
CXX = ${CROSS_COMPILE}g++
LIBS = -lpthread

CFLAGS = -O2 -fPIC

INC+=-I. -I /root/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/madplay-alsa/madplay-0.15.2b -I /root/openwrt_widora/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/libmad-0.15.1b -I /home/sanchrist/alsa-lib-1.0.25/include


OBJECTS = $(SOURCES:.c=.o)

all:libMadplay.so


libMadplay.so:audio_alsa.o audio.o player.o resample.o wplayer.o
	$(CXX) -fPIC -shared -o $@ audio_alsa.o audio.o player.o resample.o wplayer.o -L. -lmad -lasound -lpthread 
	mipsel-openwrt-linux-strip libMadplay.so
	
#madplay:main.o audio_alsa.o audio.o player.o resample.o wplayer.o 
#	$(CXX) -o $@ main.o audio_alsa.o audio.o player.o resample.o wplayer.o -L. -lmad -lasound -lpthread 

.c.o:
	$(CC)  $(INC) $(CFLAGS) -c -o $@ $<

.cpp.o:
	$(CXX)  $(INC) $(CFLAGS) -c -o $@ $<
clean:
	rm -rf $(OBJECTS) *.o libMadplay.so 

