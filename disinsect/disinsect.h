#include <elfutils/libdw.h>
#include <link.h>

#define TIMEOUT_MS 1000
#define THREAD_MAX 1024


typedef enum
{
    TIMEOUT_EXIT,
    COMMUNICATION_FAIL,
    THREAD_FAIL,
    EXIT_CLEAN,
    EXIT_FAIL,
} EXIT_CODE;

typedef struct 
{

} DIS_variable;

typedef struct
{
    DIS_variable *variables;
}DIS_stack;

typedef enum
{
    ROLE_MAIN = 0,
    ROLE_TRACER,
    ROLE_CHECKPOINT,
} DIS_role;

typedef struct 
{
    DIS_stack   *stacks;
    DIS_role    role;

    //Check if you actually need to know both
    pid_t       main_pid;
    pid_t       tracer_pid;

    pid_t       caller_tid;

    pid_t       tids[THREAD_MAX];
    int         tids_count;

    uintptr_t   main_load_bias;

    uint64_t    pc_static;

    int         main_to_tracer[2]; 
    int         tracer_to_main[2]; 
}DIS_insector;

extern DIS_insector debugger;

typedef enum
{
    DIS_OK = 0,
    DIS_READLINK_FAIL,
    DIS_OPEN_FD_FAIL,
    DIS_DWARF_BEGIN_FAIL,
    DIS_FORK_FAILED,
} DIS_ERROR_CODES;

int DIS_insector_init(void);
int DIS_insector_close(void);
void DIS_break(void);
int DIS_print_all(void);
void main_continue(void);
void main_exit(void);

void send_to_tracer(uint8_t code);
void send_to_main(uint8_t code);


int dl_phdr_callback(struct dl_phdr_info *info, size_t size, void *data);

