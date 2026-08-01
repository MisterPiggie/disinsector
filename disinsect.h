#include <elfutils/libdw.h>
#include <link.h>

#define CODE_BREAK 0x13
#define CODE_CONTINUE 0x31
#define CODE_CHECKPOINT 0x33

#define TRACER_FAILED 0x99


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
int DIS_break(void);
int DIS_print_all(void);
int DIS_continue(void);


void tracer_loop(void);
void tracer_stop_threads(void);
void tracer_exit(void);
void tracer_code_handler(uint8_t code);
int stop_thread(pid_t tid);

int dl_phdr_callback(struct dl_phdr_info *info, size_t size, void *data);

