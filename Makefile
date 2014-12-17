#Makefile for sim-eclipse.c
COMPILER 	= gcc
CFLAGS		= -g -O0 -pthread -Wall #-Wextra
LDFLAGS		= -z muldefs #-lmcheck
LIBS		=  -lcrypto
INCLUDE		= -I./
TARGET		= ./$(shell basename `readlink -f .`)
OBJDIR		= ./obj
ifeq "$(strip $(OBJDIR))" ""
	OBJDIR = .
endif
SOURCES		= $(wildcard *.c)
OBJECTS 	= $(addprefix $(OBJDIR)/, $(SOURCES:.c=.o))
DEPENDS		= $(OBJECTS:.o=.d)

#sim-eclipse: sim-eclipse.c
#	gcc -Wall -O2 -pthread sim-eclipse.c -o simulator

$(TARGET): $(OBJECTS) $(LIBS)
	$(COMPILER) -o $@ $^ $(LDFLAGS)

$(OBJDIR)/%.o: %.c
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(COMPILER) $(CFLAGS) $(INCLUDE) -o $@ -c $<

all: clean $(TARGET)

clean:
	rm -f $(OBJECTS) $(DEPENDS) $(TARGET)
	@rmdir --ignore-fail-on-non-empty `readlink -f $(OBJDIR)`

-include $(DEPENDS)
