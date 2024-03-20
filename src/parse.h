
#ifndef PARSE_H
#define PARSE_H

#include <stddef.h>

#define PARSE_TOKEN_SZ_MAX 255
#define PARSE_MODE_KEY 0
#define PARSE_MODE_VAL 1

#define PARSE_DECODE 0
#define PARSE_ENCODE 1

int parse_keyval(
    const char* key, const char* key_ck, const char* val_try, char** val_p );

int parse_post(
   const char* post, size_t post_sz, const char* key, char** val_p );

int parse_encode( const char* data_in, char** p_data_out, int mode );

#endif /* !PARSE_H */

