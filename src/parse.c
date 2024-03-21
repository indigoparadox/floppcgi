
/* vim: set ts=3 sw=3 sts=3 et nobackup : */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

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

int parse_append( char* buf, size_t buf_sz, char c ) {
   int retval = 0;
   size_t buf_len = 0;

   buf_len = strlen( buf );
   if( buf_len >= buf_sz ) {
      retval = RETVAL_OVERFLOW;
      goto cleanup;
   }

   buf[buf_len] = c;
   buf[buf_len + 1] = '\0';

cleanup:

   return retval;
}

int parse_decode( const char* data_in, size_t data_in_sz, char** p_data_out ) {
   int retval = 0;
   size_t in_i = 0,
      entity_i = 0;
   int decode_mode = 0;
   char entity[PARSE_ENTITY_SZ_MAX + 1];

   *p_data_out = calloc( 1, data_in_sz + 1 );
   if( NULL == *p_data_out ) {
      retval = RETVAL_ALLOC;
      goto cleanup;
   }

   for( in_i = 0 ; data_in_sz > in_i ; in_i++ ) {
      switch( data_in[in_i] ) {
      case '%':
         if( PARSE_DECODE_ENT == decode_mode ) {
            retval = parse_append( *p_data_out, data_in_sz, '%' );
            decode_mode = PARSE_DECODE_NONE;
            entity_i = 0;
            if( retval ) {
               goto cleanup;
            }
         } else {
            decode_mode = PARSE_DECODE_ENT;
         }
         break;

      default:
         if(
             PARSE_DECODE_ENT == decode_mode &&
             (isalpha( data_in[in_i] ) || isdigit( data_in[in_i] )) &&
             entity_i < PARSE_ENTITY_SZ_MAX
         ) {
            /* Build entity. */
            entity[entity_i++] = data_in[in_i];
            entity[entity_i] = '\0';
            debug_printf( "entity iter: %s (%d)\n", entity, entity_i );

         } else {

            if( 0 < entity_i ) {
               /* Decode build entity. */
               debug_printf( "entity: %s (%c)\n", entity,
                   (char)strtol( entity, NULL, 16 )  );
               retval = parse_append( *p_data_out, data_in_sz,
                   (char)strtol( entity, NULL, 16 )  );
               decode_mode = PARSE_DECODE_NONE;
               entity_i = 0;
            }

            if( '+' == data_in[in_i] ) {
               /* Rough decode + to space. */
               retval = parse_append( *p_data_out, data_in_sz, ' ' );
            } else {
               /* Normal character. */
               retval = parse_append( *p_data_out, data_in_sz, data_in[in_i] );
            }
            if( retval ) {
               goto cleanup;
            }
         }
         break;
      }
   }

cleanup:

   return retval;
}

int parse_encode( const char* data_in, char** p_data_out ) {
   int retval = 0;
   size_t i = 0;

   *p_data_out = calloc( 1, strlen( data_in ) + 1 );
   if( NULL == *p_data_out ) {
      retval = RETVAL_ALLOC;
      goto cleanup;
   }

   for( i = 0 ; strlen( data_in ) > i ; i++ ) {
      if( ' ' == data_in[i] ) {
         (*p_data_out)[i] = '+';
      } else {
         (*p_data_out)[i] = data_in[i];
      }
   }

cleanup:

   return retval;
}

