SCR_PATH:=$(PWD)/scr-1.1.8/install

all: clean
	mpicc -g -O0 -o fti.o -c api.c -I$(SCR_PATH)/include
	ar cr libfti.a fti.o
	mpicc -g -O0 -o hd.exe heatdis.c -I. -L$(SCR_PATH)/lib -Wl,-rpath,$(SCR_PATH)/lib -lscr libfti.a -lm

clean:
	rm -rf fti.o libfti.a hd.exe
