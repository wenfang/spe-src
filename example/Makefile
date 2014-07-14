BUILD=../build
INCLUDE=../spe
BINS=pf iot rdspool rds_client client
OBJS=pf.o iot.o rds_client.o rdspool.o client.o

.c.o:
	gcc -O2 -fPIC -std=gnu99 -pthread -Wall -I$(INCLUDE) -c $< -o $@

all:pf iot rdspool rds_client rdspool client

pf:pf.o
	gcc -O2 -fPIC -std=gnu99 -lpthread -lm -Wall $< $(BUILD)/libspe.a -o $@

iot:iot.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -lm -Wall $< $(BUILD)/libspe.a -o $@

rdspool:rdspool.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -Wall $< $(BUILD)/libspe.a -o $@

rds_client:rds_client.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -Wall $< $(BUILD)/libspe.a -o $@

client:client.o
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -Wall $< $(BUILD)/libspe.a -o $@

clean:
	rm -rf $(OBJS)
	rm -rf $(BINS)
	rm -rf gmon.out
