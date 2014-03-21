#ifndef __SPE_WORKER_H
#define __SPE_WORKER_H

struct spe_object_s;

extern void
spe_worker_do(struct spe_object_s* obj);

extern void
spe_worker_run(void);

#endif
