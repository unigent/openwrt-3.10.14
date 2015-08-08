#############################################################
# nfc handover daemon Makefile 
#############################################################
EXEC = nfchod

OBJS = nfcho/porting/mtk_nfchod_main.o nfcho/module/mtk_app_service.o \
       nfcho/module/mtk_app_ndef.o nfcho/module/mtk_app_handover.o

LIBS = lib/libnfc.a

CFLAGS += -O2 -Wall -Infcho/porting -Infcho/module
	
all: $(EXEC)

$(EXEC):  $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) -lpthread -lm -lrt

clean:
	rm -rf $(OBJS) $(EXEC)

romfs:
	$(ROMFSINST) $(EXEC) /bin/$(EXEC)

