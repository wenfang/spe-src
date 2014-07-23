BUILD=../build
INCLUDE=../spe
BINS=pf iot rdspool rds_client client threadtest

$(BINS):%:%.c
	gcc -O2 -fPIC -std=gnu99 -lpthread -lm -Wall -I$(INCLUDE) $< $(BUILD)/libspe.a -o $@

clean:
	rm -rf $(BINS)
	rm -rf gmon.out

.PHONY: clean
