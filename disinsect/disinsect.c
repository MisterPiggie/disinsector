#define _GNU_SOURCE
#include "disinsect.h"
#include "communication.h"
#include "tracer.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>

/* 
 * Dont think about tracer and its state or how it works inside here you just thik about main logic
 * after sending data to tracer you wait for response and act accordingly dont overthink it 
 * you can do it
 */

//Callback for getting bias of main app 
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
    //getting bias
    dl_iterate_phdr(dl_phdr_callback, &debugger.main_load_bias);
    debugger.main_pid = gettid();

    //initing pipes 
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

    //forking an app
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("Failed to create tracer; fork() call returned -1");
        return -1;
    }

    //splitting logic depending on child and parent roles
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



//REPL loop used as command interface
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
        {
            main_continue();
            break;
        }
        else if (strcmp(cmd, "exit") == 0)
            main_exit();
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
 
void DIS_break(void)
{
    int code = CODE_BREAK;
    io_status status = io_send_data(debugger.main_to_tracer[1], &code, sizeof(code));
    if (status != IO_OK)
    {
        fprintf(stderr, "Communication between main and tracer failed\nExit debbuger");
        return;
    }

    status = io_read_data_timeout(debugger.tracer_to_main[0], &code, sizeof(code), TIMEOUT_MS);
    if (status == IO_TIMEOUT)
    {
        kill(debugger.tracer_pid, SIGKILL);

        int wstatus;
        waitpid(debugger.tracer_pid, &wstatus, 0);   

        fprintf(stderr, "Communication between main and tracer timed out\nExiting app");
        _exit(TIMEOUT_EXIT);
    }

    if (status != IO_OK)
    {
        int wstatus;
        pid_t result = waitpid(debugger.tracer_pid, &wstatus, 0);
        if (result == debugger.tracer_pid && WIFEXITED(wstatus))
        {
            int exit_code = WEXITSTATUS(wstatus);
            if (exit_code == TRACER_EXIT_RESUME_CLEAN) 
                return;
        }

        _exit(THREAD_FAIL);
    }

    if (code == TRACER_EXIT_RESUME_CLEAN)
        return;
    else if (code == TRACER_EXIT_RESUME_FAIL)
        _exit(THREAD_FAIL);

    main_repl();
}

void main_continue(void)
{
    int code = CODE_CONTINUE;
    io_status status = io_send_data(debugger.main_to_tracer[1], &code, sizeof(code));
    if (status != IO_OK)
    {
        kill(debugger.tracer_pid, SIGKILL);

        int wstatus;
        waitpid(debugger.tracer_pid, &wstatus, 0);   

        fprintf(stderr, "Communication between main and tracer failed\nExiting app");
        _exit(COMMUNICATION_FAIL);
    }

    status = io_read_data_timeout(debugger.tracer_to_main[0], &code, sizeof(code), TIMEOUT_MS);
    if (status == IO_TIMEOUT)
    {
        kill(debugger.tracer_pid, SIGKILL);

        int wstatus;
        waitpid(debugger.tracer_pid, &wstatus, 0);   

        fprintf(stderr, "Communication between main and tracer timed out\nExiting app");
        _exit(TIMEOUT_EXIT);
    }

    //TODO: CHECK RESUME CLEAN LOGIC AND HOW TO HANDLE IT
    if (status != IO_OK)
    {
        int wstatus;
        pid_t result = waitpid(debugger.tracer_pid, &wstatus, 0);
        if (result == debugger.tracer_pid && WIFEXITED(wstatus))
        {
            int exit_code = WEXITSTATUS(wstatus);
            if (exit_code == TRACER_EXIT_RESUME_CLEAN) 
                return;
        }

        _exit(THREAD_FAIL);
    }

    if (code == TRACER_EXIT_RESUME_CLEAN)
        return;
    else if (code == TRACER_EXIT_RESUME_FAIL)
        _exit(THREAD_FAIL);
}

void main_exit(void)
{
    int code = CODE_EXIT;
    io_status status = io_send_data(debugger.main_to_tracer[1], &code, sizeof(code));
    if (status != IO_OK)
    {
        kill(debugger.tracer_pid, SIGKILL);

        int wstatus;
        waitpid(debugger.tracer_pid, &wstatus, 0);   

        fprintf(stderr, "Communication between main and tracer failed\nExiting app");
        _exit(COMMUNICATION_FAIL);
    }

    status = io_read_data_timeout(debugger.tracer_to_main[0], &code, sizeof(code), TIMEOUT_MS);
    if (status == IO_TIMEOUT)
    {
        kill(debugger.tracer_pid, SIGKILL);

        int wstatus;
        waitpid(debugger.tracer_pid, &wstatus, 0);   

        fprintf(stderr, "Communication between main and tracer timed out\nExiting app");
        _exit(TIMEOUT_EXIT);
    }

    if (status != IO_OK)
    {
        int wstatus;
        pid_t result = waitpid(debugger.tracer_pid, &wstatus, 0);
        if (result == debugger.tracer_pid && WIFSIGNALED(wstatus))
            fprintf(stderr, "tracer crashed during exit (signal %d)\n", WTERMSIG(wstatus));
        _exit(EXIT_FAIL);
    }

    if (code == TRACER_EXIT_RESUME_CLEAN)
        _exit(EXIT_CLEAN);
    else 
        _exit(EXIT_FAIL);
}
