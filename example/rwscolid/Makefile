#This is an example of Makefile

CFLAGS = -I ../../

all: rwscolid

.PHONY: clean

OBJS = rwscolid_main.o
OBJS += ../../rws_args.o ../../rws_coli.o ../../rws_uds.o

rwscolid: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJS) rwscolid
