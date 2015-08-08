#############################################################
# nfc daemon Makefile 
#############################################################
EXEC = nfcsd
CFLAGS += -O2 -Wall -Infcd/inc

OBJS = nfcd/src/mtk_nfc_sys.o nfcd/src/mtk_nfc_adp_sys.o nfcd/src/linux_nfc_main.o
LIBS = lib/libnfc.a

all: $(EXEC)

$(EXEC): $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) -lpthread -lm -lrt


clean:
	rm -rf $(OBJS) $(EXEC)

romfs:
	$(ROMFSINST) $(EXEC) /bin/$(EXEC)

