#ifndef _konf_query_h
#define _konf_query_h

#include <lub/types.h>

typedef enum
{
  KONF_QUERY_OP_NONE,
  KONF_QUERY_OP_OK,
  KONF_QUERY_OP_ERROR,
  KONF_QUERY_OP_SET,
  KONF_QUERY_OP_UNSET,
  KONF_QUERY_OP_STREAM,
  KONF_QUERY_OP_DUMP
} konf_query_op_e;

typedef struct konf_query_s konf_query_t;

konf_query_t *konf_query_new(void);
void konf_query_free(konf_query_t *instance);
int konf_query_parse(konf_query_t *instance, int argc, char **argv);
int konf_query_parse_str(konf_query_t *instance, char *str);
void konf_query_dump(konf_query_t *instance);

char *konf_query__get_pwd(konf_query_t *instance, unsigned index);
int konf_query__get_pwdc(konf_query_t *instance);
konf_query_op_e konf_query__get_op(konf_query_t *instance);
char * konf_query__get_path(konf_query_t *instance);
const char * konf_query__get_pattern(konf_query_t *instance);
const char * konf_query__get_line(konf_query_t *instance);
unsigned short konf_query__get_priority(konf_query_t *instance);
bool_t konf_query__get_splitter(konf_query_t *instance);
bool_t konf_query__get_seq(konf_query_t *instance);
unsigned short konf_query__get_seq_num(konf_query_t *instance);
bool_t konf_query__get_unique(konf_query_t *instance);
int konf_query__get_depth(konf_query_t *instance);

#endif
