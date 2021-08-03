# Makefile for libSVBony

CC = gcc
CFLAGS = -fpic -Wall -Wextra -O2 -g -DSB_LINUX_BUILD -I. -I./../../
CPPFLAGS = -fpic -Wall -Wextra -O2 -g -DSB_LINUX_BUILD -I. -I./../../
LDFLAGS = -shared -lstdc++
RM = rm -f
STRIP = strip
TARGET_LIB = libSVBony.so

SRCS = main.cpp SVBony.cpp x2camera.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^ static_libs/`arch`/libSVBCameraSDK.a
	$(STRIP) $@ >/dev/null 2>&1  || true

$(SRCS:.cpp=.d):%.d:%.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $< >$@

.PHONY: clean
clean:
	${RM} ${TARGET_LIB} ${OBJS} *.d
