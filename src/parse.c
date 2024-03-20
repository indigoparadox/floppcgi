
#include <string.h>
#include <stdlib.h>

#include "parse.h"
#include "retval.h"

int parse_keyval(
    const char* key, const char* key_ck, const char* val_try, char** val_p
 ) {
   int retval = 0;

   if( 0 != strncmp( key_ck, key, strlen( key_ck ) + 1 ) ) {
      goto cleanup;
   }

   /* Copy the prospective value to the new value pointer. */
   *val_p = malloc( strlen( val_try ) + 1 );
   if( NULL == *val_p ) {
      retval = RETVAL_ALLOC;
      goto cleanup;
   }
   strcpy( *val_p, val_try );
   (*val_p)[strlen( val_try )] = '\0';

cleanup:

   return retval;
}

int parse_post(
   const char* post, size_t post_sz, const char* key, char** val_p
) {
   int retval = 0;
   int mode = PARSE_MODE_KEY;
   size_t i = 0;
   char token[PARSE_TOKEN_SZ_MAX + 1];
   char key_iter[PARSE_TOKEN_SZ_MAX + 1];

   token[0] = '\0';
   memset( key_iter, '\0', PARSE_TOKEN_SZ_MAX + 1 );

   for( i = 0 ; post_sz > i ; i++ ) {
      switch( post[i] ) {
      case '=':
         /* Save key to compare. */
         strncpy( key_iter, token, PARSE_TOKEN_SZ_MAX );
         mode = PARSE_MODE_VAL;
         token[0] = '\0';
         break;

      case '&':
         /* Check for key match and copy if so. */
         retval = parse_keyval( key_iter, key, token, val_p );
         if( retval || NULL != *val_p ) {
            goto cleanup;
         }
         mode = PARSE_MODE_KEY;
         token[0] = '\0';
         break;

      default:
         if( strlen( token ) + 1 >= PARSE_TOKEN_SZ_MAX ) {
            retval = RETVAL_OVERFLOW;
            goto cleanup;
         }
         strncat( token, &(post[i]), 1 );
         break;
      }
   }

   /* Try the final key/value pair. */
   retval = parse_keyval( key_iter, key, token, val_p );
   if( retval ) {
      goto cleanup;
   }

cleanup:
   
   return retval;
}

int parse_encode( const char* data_in, char** p_data_out, int mode ) {
   int retval = 0;
   size_t i = 0;

   *p_data_out = calloc( 1, strlen( data_in ) + 1 );
   if( NULL == *p_data_out ) {
      retval = RETVAL_ALLOC;
      goto cleanup;
   }

   for( i = 0 ; strlen( data_in ) > i ; i++ ) {
      if( PARSE_DECODE == mode && '+' == data_in[i] ) {
         (*p_data_out)[i] = ' ';

      } else if( PARSE_ENCODE == mode && ' ' == data_in[i] ) {
         (*p_data_out)[i] = '+';
      } else {
         (*p_data_out)[i] = data_in[i];
      }
   }

cleanup:

   return retval;
}


