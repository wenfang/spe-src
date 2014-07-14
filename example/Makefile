BUILD=../build
INCLUDE=../spe
BINS=pf iot rds_server rds_client client threadtest http_server

all:$(BINS)

$(BINS):%:%.c
	gcc -DDEBUG -pg -g -fPIC -std=gnu99 -lpthread -lm -Wall -I$(INCLUDE) $< $(BUILD)/libspe.a -o $@

clean:
	rm -rf $(BINS)
	rm -rf gmon.out

.PHONY: clean
