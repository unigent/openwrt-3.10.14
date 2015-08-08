EXEC = longloopd
OBJS = long_loop.o
#CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)
#CFLAGS += -I$(ROOTDIR)/$(LINUXDIR)/drivers/net/raeth

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o

