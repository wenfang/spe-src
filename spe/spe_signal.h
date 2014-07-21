#ifndef __SPE_SIGNAL_H
#define __SPE_SIGNAL_H

typedef void SpeSignal_Handler(int sig);

extern void 
speSignalInit();

extern void 
SpeSignalRegister(int sig, SpeSignal_Handler handler);

extern void 
speSignalProcess();

#endif
