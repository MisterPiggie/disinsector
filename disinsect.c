#define _GNU_SOURCE
#include "disinsect.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <errno.h>

/*
 * I need to redesing all of this to tired and stupid
 * Firstly tracer giving error responses is bad it doesnt work; introduces to many fail states
 * Secondly need to remake the whole tracer init logic 
 * init -> start tracer
 * if success set tracer_set = true
 * if something failed stop the whole logic at init and exit
 * then tracer works with threads, if something went wrong i either notify user that thread X wasnt stopped and continue working with debugger 
 * or outright exit full stop. 
 * full stop exit sound better at least at the initial state
 * if something goes wrong just stop the whole process and when project matures get more workarounds and logic
 * problem is communication between tracer and main 
 * if something went wrong 
 * so no 
 * wait 
 * i send break request to tracer
 * wait my whole logic is wrong 
 * DIS_break need to send code to tracer from main 
 * main listen for response if response doesnt come or error i stop the whole app
 * if repsonse is good i continue with REPL
 */
 
int dl_phdr_callback(struct dl_phdr_info *info, size_t size, void *data)
{
    if (info->dlpi_name[0] == '\0') {
        uintptr_t *bias = (uintptr_t *)data;
        *bias = info->dlpi_addr;
        return 1;
    }

    return 0;
}

int DIS_insector_init(void)
{
    dl_iterate_phdr(dl_phdr_callback, &debugger.main_load_bias);
    debugger.main_pid = getpid();

    if (pipe(debugger.main_to_tracer) == -1)
    {
        perror("Failed to init pipe main_to_tracer\n");
        return -1;
    }
    if (pipe(debugger.tracer_to_main) == -1)
    {
        perror("Failed to init pipe main_to_tracer\n");
        return -1;
    }

    pid_t pid = fork();
    
    if (pid == -1)
    {
        perror("Failed to create tracer; fork() call returned -1");
        return -1;
    }

    if (pid == 0) {
        close(debugger.main_to_tracer[1]);
        close(debugger.tracer_to_main[0]);

        debugger.role = ROLE_TRACER;
        debugger.tracer_pid = getpid();
        tracer_loop();
    }

    close(debugger.main_to_tracer[0]);
    close(debugger.tracer_to_main[1]);

    debugger.role = ROLE_MAIN;
    debugger.tracer_pid = pid;
    debugger.tids_count = 0;

    return 0;
}

int DIS_break(void)
{
    if (tracer_stop_threads() != 0)

    ucontext_t ctx;
    getcontext(&ctx);

    debugger.pc_static = ctx.uc_mcontext.gregs[REG_RIP] - debugger.main_load_bias;

    return 0;
}

int tracer_stop_threads(void)
{
    char path[64];

    snprintf(path, sizeof(path), "/proc/%d/task", debugger.main_pid);

    DIR *dir = opendir(path);

    if (!dir)
    {
        perror("No threads are found\n");
        return -1;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (debugger.tids_count >= THREAD_MAX)
        {
            //TODO change max threads to dynamic
            perror("Max threads amount exceeded\n");
            return -1;
        }

        if (entry->d_name[0] == '.')
            continue;

        pid_t tid = atoi(entry->d_name);
        if (tid == debugger.main_pid)
            continue;

        if (stop_thread(tid) != 0)
            return -1;

        debugger.tids_count++;
    }

    closedir(dir);

    return 0;
}

int stop_thread(pid_t tid)
{
    if (ptrace(PTRACE_ATTACH, tid, NULL, NULL) == -1) {
        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "PTRACE_ATTACH failed on thread with tid: %d", tid);

        perror(err_msg);
        return -1;
    }

    int status;
    pid_t result = waitpid(tid, &status, 0);

    if (result == -1) {
        if (errno != EINTR) {
            perror("waitpid failed");
            return -1;
        }

        bool is_success = false;
        
        for (int tries = 0; tries < 3; tries++) {
            result = waitpid(tid, &status, 0);
            if (result != -1) {
                is_success = true;
                break;
            }

            if (errno != EINTR) {
                perror("waitpid failed");
                return -1;
            }
        }

        if (!is_success) {
            perror("waitpid failed");
            return -1;
        }
    }

    if (!WIFSTOPPED(status)) {
        if (WIFEXITED(status)) {
            fprintf(stderr, "Thread %d exited before it could be stopped; exit code %d\n", tid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            fprintf(stderr, "Thread %d was killed by signal %d before it could be stopped\n", tid, WTERMSIG(status));
        } else {
            fprintf(stderr, "Unexpected waitpid status 0x%x on thread %d\n", status, tid);
        }
        return -1;
    }

    return 0;
}

void tracer_loop(void)
{
    uint8_t buffer;

    while(1) {
        ssize_t bytes_read = read(debugger.main_to_tracer[0], &buffer, sizeof(buffer));
        if (bytes_read == 0)
            tracer_exit();
        else if (bytes_read == -1) {
            if (errno == EINTR)
                continue;
            else {
                uint8_t err = TRACER_FAILED;
                write(debugger.tracer_to_main[1], &err, sizeof(err));
                if (errno == EBADF || errno == EINVAL) {  //test version might just return all of the time 
                    tracer_exit();
                    return;
                }
                continue; //might cahnge to return need to think about main loop
            }
        }

        tracer_code_handler(buffer);
    }
}

bool tracer_has_threads(void)
{
    char path[64];
    int count = 0;

    snprintf(path, sizeof(path), "/proc/%d/task", debugger.main_pid);

    DIR *dir = opendir(path);

    if (!dir)
        return false;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL && count < 2) {
        if (entry->d_name[0] == '.')
            continue;
        count++;
    }

    closedir(dir);

    return count >= 2;
}

void tracer_break_threads(void)
{
    uint8_t buffer;
    if(tracer_stop_threads() != 0)
        perror("Failed to stop threads\n");
}

int tracer_continue_threads(void)
{
    for (int i = 0; i < debugger.tids_count; i++) {
        pid_t tid = debugger.tids[i];
        if (continue_thread(tid) != 0)
            return -1;

    }

    return 0;
}

int continue_thread(pid_t tid)
{
    if (ptrace(PTRACE_DETACH, tid, NULL, 0) == -1) {
        if (errno == ESRCH) 
            return 0;

        char err_msg[128];
        snprintf(err_msg, sizeof(err_msg), "PTRACE_DETACH failed on thread with id: %d", tid);

        perror(err_msg);
        return -1;
    }

    return 0;
}



void tracer_code_handler(uint8_t code)
{
    //TODO this is where i need to count treads; Create this funcs
    bool has_threads = tracer_has_threads();
    switch(code)
    {
        case CODE_BREAK:
            if (!has_threads)
                break;
            tracer_break_threads();
            break;
        case CODE_CONTINUE:
            if (!has_threads)
                break;
            tracer_continue_threads();
            break;
        case CODE_CHECKPOINT:
            if (!has_threads)
            {
                perror("Checkpoint is not supported with multithreaded apps\n");
                break;
            }

            // tracer_checkpoint_make();
            break;
    }
}


void main_repl(void)
{
    char line[256];
    while (1) {
        printf("dis> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;

        line[strcspn(line, "\n")] = '\0';

        char *cmd = strtok(line, " ");
        if (!cmd)
            continue;

        if (strcmp(cmd, "continue"))
            //might need clean up
            break;
        else if (strcmp(cmd, "exit") == 0)
            //also migth need cleanup
            exit(0);
        else if (strcmp(cmd, "checkpoint"))
            //need arguments; look up how gdb does that
            continue;
        else if (strcmp(cmd, "print"))
            //need arguments; look up how gdb does that
            continue;
        else if (strcmp(cmd, "set"))
            //need arguments; also need to verify types
            continue;
        else if (strcmp(cmd, "help"))
            continue;
        else 
            //print about help command
            printf("Invalid command\n");


    }
}
 
void tracer_exit(void)
{
    bool exited_without_errors = true;
    for (int i = 0; i < debugger.tids_count; i++)
        if (continue_thread(debugger.tids[i]) != 0)
            exited_without_errors = false;

    if (exited_without_errors)
        exit(0);

    exit(1);
}

void main_break(void);
