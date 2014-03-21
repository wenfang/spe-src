#ifndef __SPE_SIGNAL_H
#define __SPE_SIGNAL_H

typedef void spe_signal_Handler(int sig);

extern void 
spe_signal_init();

extern void 
spe_signal_register(int sig, spe_signal_Handler handler);

extern void 
spe_signal_process();

#endif
