#include "spe_signal.h"
#include <string.h>
#include <signal.h>

#define MAX_SIGNAL 256

struct spe_signal_s {
  unsigned            count;
  SpeSignal_Handler*  handler;
};
typedef struct spe_signal_s spe_signal_t;

static int          spe_signal_queue_len;
static int          spe_signal_queue[MAX_SIGNAL];
static spe_signal_t spe_signal_state[MAX_SIGNAL];
static sigset_t     spe_blocked_sig;

/*
===================================================================================================
speSignalInit
===================================================================================================
*/
void 
speSignalInit() {
  spe_signal_queue_len = 0;
  memset(spe_signal_queue, 0, sizeof(spe_signal_queue));
  memset(spe_signal_state, 0, sizeof(spe_signal_state));
  sigfillset(&spe_blocked_sig);
}

/*
===================================================================================================
signal_handler
===================================================================================================
*/
static void 
signal_handler(int sig) {
	// not handler this signal, ignore
  if (sig < 0 || sig > MAX_SIGNAL || !spe_signal_state[sig].handler) {
    signal(sig, SIG_IGN);
    return;
  }
	// put signal in queue
  if (!spe_signal_state[sig].count && (spe_signal_queue_len<MAX_SIGNAL)) {
    spe_signal_queue[spe_signal_queue_len++] = sig;
  }
	// add signal count
  spe_signal_state[sig].count++;
  signal(sig, signal_handler);
}

/*
===================================================================================================
SpeSignalRegister
===================================================================================================
*/
void 
SpeSignalRegister(int sig, SpeSignal_Handler handler) {
	if (sig < 0 || sig > MAX_SIGNAL) return;
	spe_signal_state[sig].count = 0;
 	if (handler == NULL) handler = SIG_IGN; 
	// set signal handler
	if (handler != SIG_IGN && handler != SIG_DFL) {
		spe_signal_state[sig].handler = handler;
 		signal(sig, signal_handler);
 	} else {                        
   	spe_signal_state[sig].handler = NULL;
 		signal(sig, handler);
	}
}

/*
===================================================================================================
speSignalProcess
===================================================================================================
*/
void 
speSignalProcess() {
	sigset_t old_sig;
	sigprocmask(SIG_SETMASK, &spe_blocked_sig, &old_sig);
 	// check signal queue	
	for (int i=0; i<spe_signal_queue_len; i++) {
		int sig = spe_signal_queue[i];
 		spe_signal_t* desc = &spe_signal_state[sig];
		if (desc->count) {
   		if (desc->handler) desc->handler(sig);
   		desc->count = 0;
 		}
 	}
	spe_signal_queue_len = 0;
	sigprocmask(SIG_SETMASK, &old_sig, NULL);  
}
