
/* vim: set ts=3 sw=3 sts=3 et nobackup : */

#include <fcgi_stdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>

#define RETVAL_PARAMS 1
#define RETVAL_DIR 2
#define RETVAL_OVERFLOW 3
#define RETVAL_ALLOC 4

#define CONFIG_ROOT "./Floppies"

#define PARSE_TOKEN_SZ_MAX 255
#define PARSE_MODE_KEY 0
#define PARSE_MODE_VAL 1

#define MODE_DECODE 0
#define MODE_ENCODE 1

void join_path( char* path_out, size_t path_out_sz_max, const char* path_in ) {
   size_t path_out_init_sz = 0;

   path_out_init_sz = strlen( path_out );
   if(
       0 < path_out_init_sz &&
       '/' != path_out[path_out_init_sz - 1] &&
       0 < strlen( path_in ) &&
       '/' != path_in[0]
   ) {
      strncat( path_out, "/", path_out_sz_max - strlen( path_out ) );
   }
   strncat( path_out, path_in, path_out_sz_max - strlen( path_out ) );
}

int list_floppies( FCGX_Request* req, const char* floppy_dir ) {
   int retval = 0;
   DIR* dir = NULL;
   struct dirent* dir_ent = NULL;
   struct stat ent_stat;
   char ent_path[PATH_MAX + 1];

   /* TODO: Print to intermediate buffer than can optionally be shown if no 404. */

   dir = opendir( floppy_dir );

   if( strlen( CONFIG_ROOT ) >= strlen( floppy_dir ) ) {
      retval = RETVAL_DIR;
      goto cleanup;
   }

   if( NULL == dir ) {
      retval = RETVAL_DIR;
      goto cleanup;
   }

   FCGX_FPrintF( req->out,
      "<form action=\"%s\" method=\"post\">\n",
      &(floppy_dir[strlen( CONFIG_ROOT )]) );

   FCGX_FPrintF( req->out,
      "<input type=\"hidden\" name=\"xxx\" value=\"yyy\" />\n" );

   FCGX_FPrintF( req->out, "<ul>\n" );

   while( NULL != (dir_ent = readdir( dir )) ) {
      if( '.' == dir_ent->d_name[0] ) {
         /* Skip hidden files. */
         continue;
      }

      memset( ent_path, '\0', PATH_MAX + 1 );
      join_path( ent_path, PATH_MAX, floppy_dir );
      join_path( ent_path, PATH_MAX, dir_ent->d_name );

      if( stat( ent_path, &ent_stat ) ) {
         continue;
      }

      /* TODO: URLencode paths. */

      if( S_IFDIR == (S_IFDIR & ent_stat.st_mode) ) {
         /* Link to directory. */
         FCGX_FPrintF( req->out,
            "<li><a href=\"%s/%s\">%s</a></li>\n",
            &(floppy_dir[strlen( CONFIG_ROOT ) + 1]), dir_ent->d_name,
            dir_ent->d_name );
      } else {
         /* POST button for file. */
         FCGX_FPrintF( req->out,
            "<li><input type=\"submit\" name=\"image\" value=\"%s\" /></li>\n",
            dir_ent->d_name );
      }
   }

   FCGX_FPrintF( req->out, "</ul>\n</form>" );

cleanup:

   if( NULL != dir ) {
      closedir( dir );
   }

   return retval;
}

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

int mount_image(
    FCGX_Request* req, const char* dir_path, const char* image_name
 ) {
   char path_buf[PATH_MAX + 1];

   memset( path_buf, '\0', PATH_MAX + 1 );

   join_path( path_buf, PATH_MAX, CONFIG_ROOT );
   join_path( path_buf, PATH_MAX, dir_path );
   join_path( path_buf, PATH_MAX, image_name );

   system( "sudo rmmod g_mass_storage" );
   system( "sudo modprobe g_mass_storage file=\"floppies.img\" stall=0" );

   return 0;
}

int encode_data( const char* data_in, char** p_data_out, int mode ) {
   int retval = 0;
   size_t i = 0;

   *p_data_out = calloc( 1, strlen( data_in ) + 1 );
   if( NULL == *p_data_out ) {
      retval = RETVAL_ALLOC;
      goto cleanup;
   }

   for( i = 0 ; strlen( data_in ) > i ; i++ ) {
      if( MODE_DECODE == mode && '+' == data_in[i] ) {
         (*p_data_out)[i] = ' ';

      } else if( MODE_ENCODE == mode && ' ' == data_in[i] ) {
         (*p_data_out)[i] = '+';
      } else {
         (*p_data_out)[i] = data_in[i];
      }
   }

cleanup:

   return retval;
}

int main() {
   FCGX_Request req;
   int cgi_sock = -1;
   int retval = 0;
   const char* req_type = NULL;
   const char* req_uri_raw = NULL;
   char* req_uri = NULL;
   char floppy_dir[PATH_MAX + 1];
   struct stat ent_stat;
   char* post_buf = NULL;
   int post_buf_sz = 0;
   char* image_sel = NULL;

   FCGX_Init();
   memset( &req, 0, sizeof( FCGX_Request ) );

   cgi_sock = FCGX_OpenSocket( "127.0.0.1:9000", 100 );
   FCGX_InitRequest( &req, cgi_sock, 0 );
	while( 0 <= FCGX_Accept_r( &req ) ) {

      /* Figure out our request type and consequent action. */
      req_type = FCGX_GetParam( "REQUEST_METHOD", req.envp );
      if( NULL == req_type ) {
         retval = RETVAL_PARAMS;
         goto req_cleanup;
      }

      /* TODO: URLdecode paths. */
      req_uri_raw = FCGX_GetParam( "DOCUMENT_URI", req.envp );
      if( NULL == req_uri_raw ) {
         retval = RETVAL_PARAMS;
         goto req_cleanup;
      }
      
      /* Fix path URL encoding. */
      retval = encode_data( req_uri_raw, &req_uri, MODE_DECODE );
      if( retval ) {
         goto req_cleanup;
      }

      memset( floppy_dir, '\0', PATH_MAX + 1 );
      join_path( floppy_dir, PATH_MAX, CONFIG_ROOT );
      join_path( floppy_dir, PATH_MAX, req_uri );

      /* Make sure path isn't dangerous or weird. */
      if(
         0 == strncmp( "..", req_uri, 3 ) ||
         0 == strncmp( ".", req_uri, 2 )
      ) {
         FCGX_FPrintF( req.out, "Status: 404 Bad Request\r\n\r\n" );
         goto req_cleanup;
      }

      /* Make sure path exists. */
      if( stat( floppy_dir, &ent_stat ) ) {
         FCGX_FPrintF( req.out, "Status: 404 Bad Request\r\n\r\n" );
         goto req_cleanup;
      }

      if( 0 == strncmp( "GET", req_type, 3 ) ) {
         /* Display the list of images. */
         FCGX_FPrintF( req.out, "Content-type: text/html\r\n" );

         FCGX_FPrintF( req.out, "\r\n" );

         FCGX_FPrintF( req.out, floppy_dir );

         retval = list_floppies( &req, floppy_dir );
         if( retval ) {
            goto req_cleanup;
         }
      } else if( 0 == strncmp( "POST", req_type, 4 ) ) {

         post_buf_sz = atoi( FCGX_GetParam( "CONTENT_LENGTH", req.envp ) );
         post_buf = calloc( post_buf_sz + 1, 1 );
         if( NULL == post_buf ) {
            retval = RETVAL_ALLOC;
            goto cleanup;
         }
         FCGX_GetStr( post_buf, post_buf_sz, req.in );

         retval = parse_post( post_buf, post_buf_sz, "image", &image_sel );
         if( retval ) {
            /* Don't exit, just abort this request. */
            retval = 0;
            goto req_cleanup;
         }

         /* TODO: Mount the image. */
         retval = mount_image( &req, req_uri, image_sel );

         if( NULL != image_sel ) {
            /* Free buffer allocated in parse_post(). */
            free( image_sel );
            image_sel = NULL;
         }

         FCGX_FPrintF( req.out, "Status: 303 See Other\r\n" );
         FCGX_FPrintF( req.out, "Location: %s\r\n", req_uri_raw );
         FCGX_FPrintF( req.out, "Cache-Control: no-cache\r\n" );
         FCGX_FPrintF( req.out, "\r\n" );
      }

req_cleanup:
      FCGX_Finish_r( &req );
      if( retval ) {
         goto cleanup;
      }
   }

cleanup:

   if( 0 <= cgi_sock ) {
      FCGX_Free( &req, cgi_sock );
   }

   return retval;
}
