exe := nvram
lib:= libnvram.so
libobjs := crc32.o flash.o nvram.o
exeobjs := main.o
CFLAGS += -Wall

ifdef CONFIG_NVRAM_MTD_NAME
CFLAGS += -DNVRAM_MTD_NAME=\"$(CONFIG_NVRAM_MTD_NAME)\"
endif

all: $(lib) $(exe)

$(lib) : $(libobjs) 
	$(CC) $(CFLAGS) -shared -o $@ $^

$(exe) : $(exeobjs) $(lib)
	$(CC) $(CFLAGS) -o $@ $^

$(exeobjs): %.o : %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(libobjs): %.o : %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

clean:
	-rm *.o $(exe) $(lib)
