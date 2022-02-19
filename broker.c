#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>

#include "htools.h"

extern int h_errno;


int broker(char *bindtohost, int port, void (*func)(int fd), int blocking_flag, int max_childs)
{
  struct sockaddr_in mysocket;
  struct sockaddr_in child;
  struct hostent *hp;
  int addrlen, connfd, sockd;
  int len = sizeof(mysocket);
  int one = 1;
  int val = 1;
  
  // init server-process-socket
  bzero((char *)&mysocket, sizeof(mysocket));
  
  mysocket.sin_port        = htons(port);
  if (bindtohost == NULL) {
    mysocket.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    hp = gethostbyname(bindtohost);              
    if (hp == NULL) {
#ifdef sun
      syslog(LOG_ERR, "gethostbyname: '%s' faild", bindtohost);
#else
      syslog(LOG_ERR, "gethostbyname: '%s' %s", bindtohost, hstrerror(h_errno));
#endif
      exit(1);
    }
    mysocket.sin_addr   = (*(struct in_addr *)(hp->h_addr_list[0]));
  }
  mysocket.sin_family      = AF_INET;
  
  if ((sockd = socket(AF_INET, SOCK_STREAM, 0)) < 0)  {
    syslog(LOG_ERR, "socket: %s", strerror(errno));
    exit(1);
  }

  // keepalive
  /*if (setsockopt(sockd, SOL_SOCKET, SO_KEEPALIVE, (char *)&one, sizeof(one)) < 0) {
    psyslog(LOG_ERR, "setsockopt: %s", strerror(errno));
    exit(1);
    }*/

  if (setsockopt(sockd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) {
    syslog(LOG_ERR, "setsockopt: %s", strerror(errno));
    exit(1);
  }
  
  if (bind(sockd, (struct sockaddr *)&mysocket, sizeof(mysocket)) < 0)  {
    syslog(LOG_ERR, "bind: %s", strerror(errno));
    exit(1);
  }
  
  if (listen(sockd, 128) == -1) {
    syslog(LOG_ERR, "listen: %s", strerror(errno));
    exit(1);
  }
  if (getsockname(sockd, (struct sockaddr *)&mysocket, &len) < 0)  {
    syslog(LOG_ERR, "getsockname: %s", strerror(errno));
    exit(1);
  }
  
  for( ; ; ) {
    signal_unblock(SIGALRM);
    signal_unblock(SIGCHLD);
    addrlen = sizeof(struct sockaddr_in);
    if ((connfd = accept(sockd, (struct sockaddr *) &child, &addrlen)) < 0) {
      if (errno != EINTR) 
	syslog(LOG_ERR, "accept: %s", strerror(errno));
      continue;
    }
    signal_block(SIGCHLD);
    signal_block(SIGALRM);


    // maximum number of childs reached?
    if (child_cnt(1) > max_childs) {
      syslog(LOG_ERR, "Maximum number of childs (%d) reached.", max_childs);
      continue;
    }
    
    // fork the child
    if (fork() == 0) {
      signal_action_flags(SIGCHLD, SIG_DFL, 0);
      signal_action(SIGALRM, SIG_IGN);
      signal_action(SIGHUP, SIG_IGN);
      signal_action(SIGTERM, SIG_IGN);
      signal_action(SIGINT, SIG_IGN);
      signal_action(SIGQUIT, SIG_IGN);
      signal_action(SIGABRT, SIG_IGN);
      signal_action(SIGPIPE, SIG_IGN);
      signal_action(SIGSEGV, exit);
      signal_action(SIGILL, exit);
      close(sockd);
      alarm(0);
      //      reset_pid_vars();
      umask(077);
      
      // nonblocking?
      if (blocking_flag == FALSE) {
	val = fcntl(connfd, F_GETFL, 0);
	fcntl(connfd, F_SETFL, val | O_NONBLOCK);
      }
      
      (*func)(connfd);
      _exit(0);
    } else {
      child_cnt(0);
    }
    close(connfd);
  }
}
