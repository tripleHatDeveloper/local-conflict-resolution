# Auteur : Omar Doukari

CC = gcc
CFLAGS = -g -O2 -Wall -fcommon -DSTACK_ALLOC -DNTRACE $(INCLUDEDIRS) -D_GNU_SOURCE

LD = gcc
LDFLAGS = -lm 

SRCS = Principal.c clset.c gstructure.c LectureAdja.c timer.c stack.c utils.c arbre-diag.c nodetable.c dp.c 
OBJS = Principal.o clset.o gstructure.o LectureAdja.o timer.o stack.o utils.o arbre-diag.o nodetable.o dp.o 

APP = espace

all: $(APP)

$(APP): $(OBJS)
	$(LD) -o $(APP) $(OBJS) $(LDFLAGS)

.c.o: 
	$(CC) -c $(CFLAGS) $<

depend: 
	makedepend -- $(CFLAGS) -- $(SRCS)

clean:
	rm -f $(OBJS) $(APP) 
