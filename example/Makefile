BUILD=../build
INCLUDE=../spe
OBJS=pf.o

.c.o:
	gcc -pg -g -fPIC -std=gnu99 -pthread -Wall -I$(INCLUDE) -c $< -o $@

all:$(OBJS)
	gcc -pg -g -fPIC -std=gnu99 -lpthread -Wall $(OBJS) $(BUILD)/libspe.a -o pf

clean:
	rm -rf $(OBJS)
	rm -rf gmon.out
	rm -rf pf
