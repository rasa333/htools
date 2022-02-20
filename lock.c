#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>

#include "htools.h"


static char *pid_file = NULL;
static int pid_fd = -1;

/* void r_locks_remove(char *dir) */
/* { */
/*   char tmp[1024]; */
/*   DI *d; */
/*   int i; */

/*   d = getdir(dir); */
/*   if (d == NULL) { */
/*     psyslog(LOG_ERR, "%s: %s", dir, strerror(errno)); */
/*     exit(1); */
/*   } */

/*   for (i = 0 ; i < d->no_of_files ; i++) { */
/*     if (!strcmp(d->fl[i].name, ".") || !strcmp(d->fl[i].name, "..")) */
/*       continue; */

/*     strcpy(tmp, dir); */
/*     strcat(tmp, "/"); */
/*     strcat(tmp, d->fl[i].name); */
/*     if ((d->fl[i].stat.st_mode & S_IFMT) & S_IFDIR) { */
/*       r_locks_remove(tmp); */
/*     } else { */
/*       if (match(tmp, "*.lock")) { */
/* 	if (unlink(tmp) == -1) { */
/* 	  psyslog(LOG_ERR, "%s: %s", tmp, strerror(errno)); */
/* 	  exit(1); */
/* 	} */
/*       } */
/*       if (match(tmp, "*\/.lock*")) { */
/* 	if (unlink(tmp) == -1) { */
/* 	  psyslog(LOG_ERR, "%s: %s", tmp, strerror(errno)); */
/* 	  exit(1); */
/* 	} */
/*       } */
/*     } */
/*   } */
/*   freedir(d); */
/* } */



void pid_write_file(char *argv0, char *arg, char *basedir)
{
    char *n = basename(argv0);
    char file[1024], path[1024], pid[256];
    int fd, val;
    struct flock lock;

    if (arg == NULL) {
        strcpy(file, n);
    } else {
        snprintf(file, sizeof(file), "%s:%s", n, arg);
    }

    snprintf(path, sizeof(path), "%s/lock/%s.pid", basedir, file);
    fd = open(path, O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        syslog(LOG_ERR, "%s: %s", path, strerror(errno));
        exit(1);
    }

    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_whence = SEEK_SET;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            syslog(LOG_ERR, "%s seems to be already running - exit", file);
            exit(1);
        } else {
            syslog(LOG_ERR, "%s: %s", path, strerror(errno));
            exit(1);
        }
    }
    if (ftruncate(fd, 0) < 0) {
        syslog(LOG_ERR, "%s: %s", path, strerror(errno));
        exit(1);
    }
    snprintf(pid, sizeof(pid), "%d\n", getpid());
    if (write(fd, pid, strlen(pid)) != strlen(pid)) {
        syslog(LOG_ERR, "%s: %s", path, strerror(errno));
        exit(1);
    }

    /* set close-on-exec flag */

    val = fcntl(fd, F_GETFD, 0);
    val |= FD_CLOEXEC;
    fcntl(fd, F_SETFD, val);

    pid_file = strdup(path);
    pid_fd = fd;
}


void pid_test_file(char *argv0, char *arg, char *basedir)
{
    char *n = basename(argv0);
    char file[1024];
    char path[1024];
    int fd;
    struct flock lock;

    if (arg == NULL) {
        strcpy(file, n);
    } else {
        snprintf(file, sizeof(file), "%s:%s", n, arg);
    }

    snprintf(path, sizeof(path), "%s/lock/%s.pid", basedir, file);
    fd = open(path, O_RDONLY);
    if (fd < 0)
        return;

    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_whence = SEEK_SET;

    if (fcntl(fd, F_GETLK, &lock) < 0) {
        fprintf(stderr, "fcntl: %s: %s", path, strerror(errno));
        exit(1);
    }
    close(fd);

    if (lock.l_type == F_UNLCK)
        return;

    fprintf(stderr, "!!! %s seems to be already running (process id %d) - exit\n", file, lock.l_pid);
    exit(1);
}

int pid_test_file_bool(char *argv0, char *arg, char *basedir)
{
    char *n = basename(argv0);
    char file[1024];
    char path[1024];
    int fd;
    struct flock lock;

    if (arg == NULL) {
        strcpy(file, n);
    } else {
        snprintf(file, sizeof(file), "%s:%s", n, arg);
    }

    snprintf(path, sizeof(path), "%s/lock/%s.pid", basedir, file);
    fd = open(path, O_RDONLY);
    if (fd < 0)
        return FALSE;

    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_whence = SEEK_SET;

    if (fcntl(fd, F_GETLK, &lock) < 0) {
        syslog(LOG_ERR, "fcntl: %s: %s", path, strerror(errno));
        return FALSE;
    }
    close(fd);

    if (lock.l_type == F_UNLCK)
        return FALSE;

    return TRUE;
}


void reset_pid_vars(void)
{
    if (pid_file != NULL)
        free(pid_file);
    pid_file = NULL;
    pid_fd = -1;
}


//
// file-locking functions
//

static char **filelist = NULL;


void lock_free()
{
    if (filelist == NULL)
        return;
    free(filelist);
}

static void lock_save(char *file, int fd)
{
    int i;

    if (filelist == NULL) {
        filelist = malloc(sizeof(char **) * getdtablesize());
        for (i = 0; i < getdtablesize(); i++)
            filelist[i] = NULL;
    }

    if (filelist[fd] != NULL)
        free(filelist[fd]);

    filelist[fd] = strdup(file);
}

static int lock_getfd(char *file)
{
    int i;

    for (i = 0; i < getdtablesize(); i++)
        if (filelist[i] != NULL && !strcmp(filelist[i], file))
            return i;

    return INVALID;
}

inline static char *lock_getfile(int fd)
{
    if (filelist == NULL)
        return NULL;

    return filelist[fd];
}


// create lock for a given file
//
// mode = TRUE    -- retry until we get the lock
// mode = FALSE   -- exit(1) if the file is already locked
// mode = INVALID -- return -1 if the file is already locked
//
// returns the file-descriptor of the lock-file or
// returns -1 in case of an error

int lock_make(char *file, int mode)
{
    struct flock lock;
    int val, lockfd, pid_test;
    char pid[20], lockfile[1024];

    snprintf(lockfile, sizeof(lockfile), "%s.lock", file);

    val = umask(0);
    lockfd = open(lockfile, O_CREAT | O_WRONLY, 0666);
    umask(val);
    if (lockfd == -1)
        return -1;

    while (3) {
        memset(&lock, '\0', sizeof(struct flock));
        lock.l_type = F_WRLCK;
        lock.l_start = 0;
        lock.l_len = 0;
        lock.l_whence = SEEK_SET;
        lock.l_pid = getpid();
        if (fcntl(lockfd, F_SETLK, &lock) < 0) {
            if (errno == EAGAIN || errno == EACCES) {
                pid_test = lock_test(file);
                if (pid_test == -1 || pid_test == FALSE) {
                    sleep(1);
                    continue;
                }
                switch (mode) {
                    case TRUE:
                        syslog(LOG_INFO, "%s: Resource is locked by process-id %d. Waiting for unlock...",
                               file, pid_test);
                        sleep(3);
                        break;
                    case FALSE:
                        syslog(LOG_ERR, "%s: Resource is locked by process-id %d. Exiting...", file, pid_test);
                        exit(1);
                    case INVALID:
                        syslog(LOG_ERR, "%s: Resource is locked by process-id %d. Returning...", file, pid_test);
                        close(lockfd);
                        return -1;
                }
                continue;
            } else {
                syslog(LOG_ERR, "lock: %s: %s (errno=%d)", file, strerror(errno), errno);
                exit(1);
            }
        }
        break;
    }

    snprintf(pid, sizeof(pid), "%d", getpid());
    write(lockfd, pid, strlen(pid));

    /* set close-on-exec flag */

    val = fcntl(lockfd, F_GETFD, 0);
    val |= FD_CLOEXEC;
    fcntl(lockfd, F_SETFD, val);

    lock_save(lockfile, lockfd);

    return lockfd;
}


// unlock a file by file-descriptor

void lock_unmake(int lockfd)
{
    struct flock lock;

    if (lockfd < 0) {
        syslog(LOG_ERR, "r_lock_unmake(): %s: %s", filelist[lockfd], strerror(errno));
        exit(1);
    }

    lock.l_type = F_UNLCK;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_whence = SEEK_SET;

    if (fcntl(lockfd, F_SETLK, &lock) < 0) {
        syslog(LOG_ERR, "unlock: %s: %s", filelist[lockfd], strerror(errno));
        exit(1);
    }
    close(lockfd);
    unlink(filelist[lockfd]);
    free(filelist[lockfd]);
    filelist[lockfd] = NULL;
}


// test if a file is locked
// returns -1 if an error occured or
// returns FALSE if the file is NOT locked or
// returns PID of process locked the file

int lock_test(char *file)
{
    int fd;
    struct flock lock;
    char lockfile[1024];

    snprintf(lockfile, sizeof(lockfile), "%s.lock", file);

    fd = open(lockfile, O_RDONLY);
    if (fd == -1)
        return -1;

    lock.l_type = F_WRLCK;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_whence = SEEK_SET;
    if (fcntl(fd, F_GETLK, &lock) < 0) {
        close(fd);
        return -1;
    }
    close(fd);
    if (lock.l_type == F_UNLCK)
        return FALSE;

    return lock.l_pid;
}


// Only used by r_exit() -- it should output nothing when all works
// well :-) -- otherwise a list of all locked files are listed.

void lock_verify()
{
    int i;

    if (filelist == NULL)
        return;

    for (i = 0; i < getdtablesize(); i++)
        if (filelist[i] != NULL)
            printf("Resource '%s' not freed\n", filelist[i]);
}


//
// file-stream
//

FILE *lock_fopen(char *file, char *mode)
{
    int fdlock;
    FILE *f;

    fdlock = lock_make(file, TRUE);
    if (fdlock == -1)
        return NULL;

    f = fopen(file, mode);
    if (f == NULL) {
        lock_unmake(fdlock);
        return NULL;
    }

    return f;
}

FILE *lock_fopen_exit(char *file, char *mode)
{
    int fdlock;
    FILE *f;

    fdlock = lock_make(file, FALSE);
    if (fdlock == -1)
        return NULL;

    f = fopen(file, mode);
    if (f == NULL) {
        lock_unmake(fdlock);
        return NULL;
    }

    return f;
}

int lock_fclose(FILE *f, char *file)
{
    int fdlock;
    char lockfile[1024];

    snprintf(lockfile, sizeof(lockfile), "%s.lock", file);
    fdlock = lock_getfd(lockfile);
    lock_unmake(fdlock);

    return fclose(f);
}


/*
//
// z-stream
//

gzFile lock_gzopen(char *file, char *mode)
{
  int fdlock;
  gzFile f;

  fdlock = r_lock_make(file, TRUE);
  if (fdlock == -1)
    return NULL;

  f = gzopen(file, mode);
  if (f == NULL) {
    r_lock_unmake(fdlock);
    return NULL;
  }

  return f;
}

gzFile lock_gzopen_exit(char *file, char *mode)
{
  int fdlock;
  gzFile f;

  fdlock = r_lock_make(file, FALSE);
  if (fdlock == -1)
    return NULL;

  f = gzopen(file, mode);
  if (f == NULL) {
    r_lock_unmake(fdlock);
    return NULL;
  }

  return f;
}

int lock_gzclose(gzFile f, char *file)
{
  int fdlock;
  char lockfile[1024];

  snprintf(lockfile, sizeof(lockfile), "%s.lock", file);
  fdlock = r_lock_getfd(lockfile);
  r_lock_unmake(fdlock);

  return gzclose(f);
}
*/
