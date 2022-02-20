#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>


char *signal_text(int sig)
{
    char *cptr;
    static char sigunknown[15];

    sig &= 0x7F;
    switch (sig) {
        case SIGHUP:
            cptr = "SIGHUP";
            break;
        case SIGINT:
            cptr = "SIGINT";
            break;
        case SIGQUIT:
            cptr = "SIGQUIT";
            break;
        case SIGILL:
            cptr = "SIGILL";
            break;
        case SIGTRAP:
            cptr = "SIGTRAP";
            break;
        case SIGIOT:
            cptr = "SIGIOT";
            break;
        case SIGFPE:
            cptr = "SIGFPE";
            break;
        case SIGKILL:
            cptr = "SIGKILL";
            break;
        case SIGBUS:
            cptr = "SIGBUS";
            break;
        case SIGSEGV:
            cptr = "SIGSEGV";
            break;
        case SIGPIPE:
            cptr = "SIGPIPE";
            break;
        case SIGALRM:
            cptr = "SIGALRM";
            break;
        case SIGTERM:
            cptr = "SIGTERM";
            break;
        case SIGUSR1:
            cptr = "SIGUSR1";
            break;
        case SIGUSR2:
            cptr = "SIGUSR2";
            break;
        case SIGCHLD:
            cptr = "SIGCHLD";
            break;
        case SIGWINCH:
            cptr = "SIGWINCH";
            break;
        default:
            snprintf(sigunknown, sizeof(sigunknown), "SIGNAL %u", sig);
            return sigunknown;
    }
    return cptr;
}

void signal_hdl__exit(int sig)
{
    syslog(LOG_NOTICE, "%s caught", signal_text(sig));
    exit(0);
}

void signal_hdl__pipe(int sig)
{
    syslog(LOG_NOTICE, "%s caught", signal_text(sig));
    exit(0);
}

pid_t *signal_childs_died(pid_t pid)
{
    static pid_t *list = NULL;
    int i = 0;

    if (pid == -1)
        return list;
    if (pid == -2) {
        free(list);
        list = NULL;
        return NULL;
    }
    if (list == NULL) {
        list = malloc(sizeof(pid_t *) * 2);
    } else {
        for (; list[i] != -1; i++);
        list = realloc(list, sizeof(pid_t *) * (i + 2));
    }
    list[i] = pid;
    list[i + 1] = -1;

    return list;
}

int child_cnt(int state)
{
    static int cnt = 0;

    switch (state) {
        case 0:
            return cnt;
        case -1:
            if (cnt == 0)
                return 0;
            return --cnt;
        case 1:
            return ++cnt;
    }

    return cnt;
}


void signal_hdl__child(int sig)
{
    int pid;
    int status;

    while (3) {
        pid = waitpid(-1, &status, WNOHANG);
        if (pid == -1) {
            if (errno == EINTR)
                continue;
            break;
        } else if (pid == 0) {
            break;
        } else {
            child_cnt(-1);
            continue;
        }
    }
}


void signal_init()
{
    struct sigaction sact;

    sigfillset(&sact.sa_mask);
    sact.sa_handler = signal_hdl__child;
    sact.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sact, 0);
    sact.sa_handler = signal_hdl__pipe;
    sigaction(SIGPIPE, &sact, 0);
    sact.sa_handler = signal_hdl__exit;
    sigaction(SIGINT, &sact, 0);
    sact.sa_handler = signal_hdl__exit;
    sigaction(SIGTERM, &sact, 0);
    sact.sa_handler = signal_hdl__exit;
    sigaction(SIGQUIT, &sact, 0);
    sact.sa_handler = signal_hdl__exit;
    sigaction(SIGHUP, &sact, 0);
}

static sigset_t sigset_mask;

void signal_block_all()
{
    sigemptyset(&sigset_mask);
    sigaddset(&sigset_mask, SIGALRM);
    sigaddset(&sigset_mask, SIGINT);
    sigaddset(&sigset_mask, SIGTERM);
    sigaddset(&sigset_mask, SIGCHLD);
    sigaddset(&sigset_mask, SIGQUIT);
    sigaddset(&sigset_mask, SIGHUP);
    sigaddset(&sigset_mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigset_mask, NULL);
}

void signal_unblock_all()
{
    sigprocmask(SIG_UNBLOCK, &sigset_mask, NULL);
}

void signal_block_reset()
{
    sigemptyset(&sigset_mask);
    sigprocmask(SIG_UNBLOCK, &sigset_mask, NULL);
}

void signal_block(int sig)
{
    sigaddset(&sigset_mask, sig);
    sigprocmask(SIG_BLOCK, &sigset_mask, NULL);
}

void signal_unblock(int sig)
{
    static sigset_t tmp_mask;

    sigemptyset(&tmp_mask);
    sigaddset(&tmp_mask, sig);
    sigprocmask(SIG_UNBLOCK, &tmp_mask, NULL);
    sigdelset(&sigset_mask, sig);
}


void signal_action(int sig, void (*handler)(int))
{
    struct sigaction sact;

    memset(&sact, 0, sizeof(sact));
    sact.sa_handler = handler;
    sact.sa_flags = SA_RESTART; // | SA_NOCLDWAIT | SA_NOCLDSTOP;
    sigaction(sig, &sact, NULL);
}

void signal_action_flags(int sig, void (*handler)(int), int flags)
{
    struct sigaction sact;

    memset(&sact, 0, sizeof(sact));
    sact.sa_handler = handler;
    sact.sa_flags = flags;
    sigaction(sig, &sact, NULL);
}