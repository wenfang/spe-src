BUILD=../build
INCLUDE=../spe
OBJS=pf.o rdspool.o rds_client.o iot.o

.c.o:
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -pthread -Wall -I$(INCLUDE) -c $< -o $@

all:$(OBJS)
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -Wall pf.o $(BUILD)/libspe.a -o pf
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -Wall iot.o $(BUILD)/libspe.a -o iot
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -Wall rdspool.o $(BUILD)/libspe.a -o rdspool
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -Wall rds_client.o $(BUILD)/libspe.a -o rds_client

clean:
	rm -rf $(OBJS)
	rm -rf gmon.out
	rm -rf pf
	rm -rf iot
	rm -rf rdspool
	rm -rf rds_client
