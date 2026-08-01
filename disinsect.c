#define _GNU_SOURCE
#include "disinsect.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <ucontext.h>
#include <errno.h>
 
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

    return 0;
}

int DIS_break(void)
{
    ucontext_t ctx;
    getcontext(&ctx);

    debugger.pc_static = ctx.uc_mcontext.gregs[REG_RIP] - debugger.main_load_bias;

    return 0;
}

void tracer_stop_threads(void)
{
    char path[64];

    snprintf(path, sizeof(path), "/proc/%d/task", debugger.main_pid);

    DIR *dir = opendir(path);

    if (!dir)
        return;

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        pid_t tid = atoi(entry->d_name);
        if (tid == debugger.main_pid)
            continue;

        if (stop_thread(tid) != 0)
            return;

    }

    closedir(dir);

    return;

}

int stop_thread(pid_t tid)
{
    if (ptrace(PTRACE_ATTACH, tid, NULL, NULL) == -1) {
        perror("PTRACE_ATTACH failed");
        return -1;
    }

    int status;
    waitpid(tid, &status, 0);

    if (!WIFSTOPPED(status)) {
        char err_msg[128];
        sprintf(err_msg, "Unexpected waitpid status code %d", status);

        perror(err_msg);
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

        tracer_code_to_action(buffer);
    }

}

