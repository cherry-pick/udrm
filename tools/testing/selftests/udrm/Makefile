TEST_PROGS := udrm-test

OBJS =				\
	test.o			\
	test-api.o

CFLAGS += -Wall -I../../../../usr/include/

all: $(TEST_PROGS)

include ../lib.mk

clean:
	$(RM) $(TEST_PROGS)

%.o: %.c test.h ../../../../usr/include/linux/udrm.h
	$(CC) $(CFLAGS) -c $< -o $@

udrm-test: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LDLIBS) -o $@
