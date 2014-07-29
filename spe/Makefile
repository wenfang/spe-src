TARGET = ../build/libspe.a

OBJS = spe_sock.o spe_rbtree.o spe_string.o spe_util.o spe_map.o spe_signal.o spe_log.o spe_tpool.o spe_io.o spe_opt.o spe_main.o spe_task.o spe_epoll.o spe_conn.o spe_server.o spe_conb.o spe_redis.o spe_shm.o cJSON.o http_parser.o spe_monitor.o

.c.o:
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -pthread -Wall -c $< -o $@

all:$(OBJS)
	ar -r $(TARGET) $(OBJS)

clean:
	rm -rf $(OBJS)
	rm -rf $(TARGET)
	rm -rf gmon.out

.PHONY: all clean
