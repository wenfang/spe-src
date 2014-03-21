BUILD = ../build

OBJS = spe_sock.o spe_rbtree.o spe_string.o spe_util.o spe_map.o spe_signal.o spe_log.o spe_tpool.o spe_io.o spe_opt.o spe_module.o spe_main.o spe_epoll.o spe_timer.o spe_task.o spe_server.o spe_conb.o spe_conn.o spe_object.o spe_worker.o spe_routine.o spe_objmap.o

.c.o:
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -pthread -Wall -c $< -o $@

all:$(OBJS)
	ar -r $(BUILD)/libspe.a $(OBJS)

clean:
	rm -rf $(OBJS)
	rm -rf $(BUILD)/libspe.a
	rm -rf gmon.out
