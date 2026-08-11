#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
    CODE_BREAK,
    CODE_CONTINUE,
    CODE_EXIT,
    CODE_CHECKPOINT,
} TRACER_CODE;

typedef enum
{
    TRACER_OK,
    TRACER_EXIT_RESUME_CLEAN,
    TRACER_EXIT_RESUME_FAIL,
} tracer_exit_codes;

void tracer_loop(void);
int tracer_stop_threads(void);
void tracer_exit(void);
void tracer_code_handler(uint8_t code);
bool tracer_has_threads(void);
int tracer_continue_threads(void);
int stop_thread(pid_t tid);
int continue_thread(pid_t tid);
