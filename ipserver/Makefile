TARGET=aoipserver
SRCS=main.c tablet.c udp.c wavefile.c mixer.c volume_gain_table.c process_loop.c
SRCS+=audio.c

ifeq (${V},)						   
NOECHO	= @
endif

OBJDIR=obj
ifeq ($(shell test -d $(OBJDIR) || mkdir -p $(OBJDIR)),)
endif
OBJS=$(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
LIBS = -lpthread -lasound -lm -lsndfile
CFLAGS=-g -Wall -Wno-unused-function -O0 $(INCLUDES) 


all: $(TARGET)

$(TARGET): $(OBJS)
	@echo " $(notdir ->$@)";
	$(NOECHO)$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJDIR)/%.o: %.c
	@echo " $(notdir $<)";
	$(NOECHO)$(CC) $(CFLAGS) -MD -c $< -o $@

clean:
	rm -rf *.o *.a  $(TARGET) $(OBJDIR) 


-include $(wildcard $(OBJDIR)/*.d)
