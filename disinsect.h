#include <elfutils/libdw.h>


typedef struct 
{

} DIS_variable;

typedef struct
{
    DIS_variable *variables;
}DIS_stack;

typedef enum
{
    main = 0,
    tracer,
} DIS_role;

typedef struct 
{
    DIS_stack   *stacks;
    DIS_role    role;

    int         main_to_tracer[2]; 
    int         tracer_to_main[2]; 
}DIS_insector;

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
int DIS_print(void *args...);
int DIS_continue(void);


