BUILD=../build
INCLUDE=../spe
OBJS=pf.o iot.o rds_client.o rdspool.o

.c.o:
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -pthread -Wall -I$(INCLUDE) -c $< -o $@

all:pf iot rdspool rds_client rdspool

pf:pf.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -lm -Wall pf.o $(BUILD)/libspe.a -o pf

iot:iot.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -lm -Wall iot.o $(BUILD)/libspe.a -o iot

rdspool:rdspool.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -Wall rdspool.o $(BUILD)/libspe.a -o rdspool

rds_client:rds_client.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -Wall rds_client.o $(BUILD)/libspe.a -o rds_client

clean:
	rm -rf $(OBJS)
	rm -rf gmon.out
	rm -rf pf
	rm -rf iot
	rm -rf rdspool
	rm -rf rds_client
