#include <unistd.h>
#include <stdint.h>

void tracer_loop(void);
int tracer_stop_threads(void);
void tracer_exit(void);
void tracer_code_handler(uint8_t code);
int stop_thread(pid_t tid);
int continue_thread(pid_t tid);
