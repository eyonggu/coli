##Install following packages
#sudo apt install libeditline-dev
#sudo apt install libedit-dev

CFLAGS = -I ../../
LDFLAGS = -lreadline -ledit

all: rwscolish

.PHONY: clean

OBJS = rwscolish_main.o rwscolish_cmd.o
OBJS += ../../rws_args.o ../../rws_coli.o ../../rws_uds.o

rwscolish: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -rf $(OBJS) rwscolish
