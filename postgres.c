#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <libpq-fe.h>
#include <libpq/libpq-fs.h>
#include <syslog.h>

#include "htools.h"

static PGconn   *conn;
static PGresult *res_query = NULL;
static long      res_query_tuples = 0;
static int       res_query_nfields;
static long      res_query_cnt = 0;
static int       connected = FALSE;


int sql_postgres_connect(char *database, char *host, char *port, char *username, char *password)
{
  conn = PQsetdbLogin(host, port == NULL || port[0] == 0 ? "5432" : port, NULL, NULL, database, username, password);
  if (PQstatus(conn) != CONNECTION_OK) {
    syslog(LOG_ERR, "Connection to database '%s' failed; %s", database, PQerrorMessage(conn));
    PQfinish(conn);
    connected = FALSE;
    return FALSE;
  }
  connected = TRUE;

  return TRUE;
}


int sql_postgres_is_connected()
{
  return connected;
}



void sql_postgres_disconnect()
{
  if (connected == FALSE)
    return;

  PQfinish(conn);
  connected = FALSE;
}



void sql_postgres_begin()
{
  PGresult *res;
  
  if (connected == FALSE)
    return;

  res = PQexec(conn, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    syslog(LOG_ERR, "postgres 'BEGIN' command failed");
    PQclear(res);
    PQfinish(conn);
    connected = FALSE;
    return;
  }
  PQclear(res);
}

void sql_postgres_end()
{
  PGresult *res;
  
  if (connected == FALSE)
    return;

  res = PQexec(conn, "END");
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    syslog(LOG_ERR, "postgres 'END' command failed");
    PQclear(res);
    PQfinish(conn);
    connected = FALSE;
    return;
  }
  PQclear(res);
}



char **sql_postgres_query_first(char *fmt, ...)
{
  char *query, **list = NULL;
  va_list ap;
  int j;
  int n, size = 10240;
  
  query = malloc(size);
  while (1) {
    va_start(ap, fmt);
    n = vsnprintf (query, size, fmt, ap);
    va_end(ap);
    if (n > -1 && n < size)
      break;
    if (n > -1)     /* glibc 2.1 */
      size = n + 1; /* precisely what is needed */
    else            /* glibc 2.0 */
      size *= 2;    /* twice the old size */
    query = realloc (query, size);
  }
  
  res_query = PQexec(conn, query);
  if (PQresultStatus(res_query) != PGRES_COMMAND_OK && PQresultStatus(res_query) != PGRES_EMPTY_QUERY &&
      PQresultStatus(res_query) != PGRES_TUPLES_OK) {
    syslog(LOG_ERR, "Postgres '%s' command failed; %s", query, PQerrorMessage(conn));
    PQclear(res_query);
    free(query);
    return NULL;
  }
  free(query);
  res_query_tuples = PQntuples(res_query);
  
  if (res_query_tuples < 1) {
    PQclear(res_query);
    return NULL;
  }
  res_query_nfields = PQnfields(res_query);
  list = malloc(sizeof(char *) * (res_query_nfields + 1));
  for (j = 0 ; j < res_query_nfields ; j++)
    list[j] = strdup(PQgetvalue(res_query, 0, j));
  list[j] = NULL;

  if (res_query_tuples == 1) {
    PQclear(res_query);
    res_query = NULL;
    res_query_tuples = 0;
    res_query_cnt = 0;
    return list;
  }
  res_query_cnt = 1;
  
  return list;
}


char **sql_postgres_query_next()
{
  char **list = NULL;
  long j;
  
  if (res_query == NULL)
    return NULL;

  if (res_query_cnt + 1 > res_query_tuples) {
    PQclear(res_query);
    res_query = NULL;
    res_query_tuples = 0;
    res_query_cnt = 0;
    return NULL;
  }
  list = malloc(sizeof(char *) * (res_query_nfields + 1));
  for (j = 0 ; j < res_query_nfields ; j++)
    list[j] = strdup(PQgetvalue(res_query, res_query_cnt, j));
  list[j] = NULL;
  res_query_cnt++;

  return list;
}


char **sql_postgres_execute(char *fmt, ...)
{
  char *query, **list = NULL;
  va_list ap;
  int j;
  int n, size = 10240, nfields;
  
  query = malloc(size);
  while (1) {
    va_start(ap, fmt);
    n = vsnprintf (query, size, fmt, ap);
    va_end(ap);
    if (n > -1 && n < size)
      break;
    if (n > -1)     /* glibc 2.1 */
      size = n + 1; /* precisely what is needed */
    else            /* glibc 2.0 */
      size *= 2;    /* twice the old size */
    query = realloc (query, size);
  }
  
  res_query = PQexec(conn, query);
  if (PQresultStatus(res_query) != PGRES_COMMAND_OK && PQresultStatus(res_query) != PGRES_EMPTY_QUERY &&
      PQresultStatus(res_query) != PGRES_TUPLES_OK) {
    syslog(LOG_ERR, "Postgres '%s' command failed; %s", query, PQerrorMessage(conn));
    PQclear(res_query);
    free(query);
    return NULL;
  }
  free(query);
  if (PQntuples(res_query) < 1) {
    PQclear(res_query);
    return NULL;
  }
  nfields = PQnfields(res_query);
  list = malloc(sizeof(char *) * (nfields + 1));
  for (j = 0 ; j < nfields ; j++)
    list[j] = strdup(PQgetvalue(res_query, 0, j));
  list[j] = NULL;
  PQclear(res_query);
  
  return list;
}


char *sql_where_in(char **list)
{
  int i, len = 0;
  char *s;
  
  if (list == NULL)
    return NULL;
  for (i = 0 ; list[i] != NULL ; i++)
    len += strlen(list[i]) + 10;
  s = malloc(len + 1);
  strcpy(s, "(");
  for (i = 0 ; list[i] != NULL ; i++) {
    strcat(s, "'");
    strcat(s, list[i]);
    strcat(s, "'");
    if (list[i + 1] != NULL)
      strcat(s, ",");
  }
  strcat(s, ")");

  return s;
}

char *sql_postgres_escape_string(char *s)
{
  char *n = malloc(strlen(s) * 2);
  char *p = n;
  
  *n = 0;
  while(*s) {
    if (*s == '\'') {
      *n = '\'';
      n++;
    }
    *n++ = *s++;
  }
  *n = 0;

  return p;
}
