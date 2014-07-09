BUILD=../build
INCLUDE=../spe
#OBJS=pf.o rdspool.o rds_client.o iot.o
OBJS=pf.o

.c.o:
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -pthread -Wall -I$(INCLUDE) -c $< -o $@

all:pf iot rdspool rds_client

pf:pf.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -lm -Wall pf.o $(BUILD)/libspe.a -o pf

json:json.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -lm -Wall json.o $(BUILD)/libspe.a -o json

iot:iot.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -Wall iot.o $(BUILD)/libspe.a -o iot

rdspool:rdspool.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -Wall rdspool.o $(BUILD)/libspe.a -o rdspool

rds_client:rdsclient.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -Wall rds_client.o $(BUILD)/libspe.a -o rds_client

clean:
	rm -rf $(OBJS)
	rm -rf gmon.out
	rm -rf pf
	rm -rf json
	rm -rf iot
	rm -rf rdspool
	rm -rf rds_client
