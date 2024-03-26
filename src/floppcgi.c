
/* vim: set ts=3 sw=3 sts=3 et nobackup : */

#include <fcgi_stdio.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "parse.h"
#include "retval.h"

#define VAR_FLOPPIES_ROOT "FLOPPIES_ROOT"

#define VAR_FLOPPIES_CONTAINER "FLOPPIES_CONTAINER"

#define VAR_FLOPPIES_ASSETS "FLOPPIES_ASSETS"

#define FLOPPIES_CONTAINER_SZ 10000000

#define FLOPPIES_FF_CFG "interface = ibmpc\nhost = unspecified\npin02 = auto\npin34 = auto\nwrite-protect = %s\nside-select-glitch-filter = 0\ntrack-change = instant\nindex-suppression = yes\nejected-on-startup = no\nimage-on-startup = %s\ndisplay-probe-ms = 3000\nautoselect-file-secs = 0\nautoselect-folder-secs = 0\nnav-mode = default\nnav-loop = yes\ntwobutton-action = rotary\nrotary = full\ndisplay-type = auto\nnav-scroll-rate = 80\nnav-scroll-pause = 300\nstep-volume = 20\nda-report-version = ""\nextend-image = yes\n"

void join_path( char* path_out, size_t path_out_sz_max, const char* path_in ) {
   size_t path_out_init_sz = 0;

   path_out_init_sz = strlen( path_out );
   if(
       0 < path_out_init_sz &&
       '/' != path_out[path_out_init_sz - 1] &&
       '"' != path_out[path_out_init_sz - 1] &&
       0 < strlen( path_in ) &&
       '/' != path_in[0] &&
       '"' != path_in[0]
   ) {
      strncat( path_out, "/", path_out_sz_max - strlen( path_out ) );
   }
   strncat( path_out, path_in, path_out_sz_max - strlen( path_out ) );
   if( 0 < strlen( path_out ) && '/' == path_out[strlen( path_out ) - 1] ) {
      path_out[strlen( path_out ) - 1] = '\0';
   }
}

int list_floppies( FCGX_Request* req, const char* floppy_dir ) {
   int retval = 0;
   struct stat ent_stat;
   char ent_path[PATH_MAX + 1];
   const char* floppy_root = NULL;
   char parent_path[PATH_MAX + 1];
   char* parent_path_slash = NULL;
   char floppy_dir_web[PATH_MAX + 1];
   int idx = 0;
   int floppy_container_txt_f = -1;
   char floppy_mounted[PATH_MAX + 1];
   char floppy_container_txt[PATH_MAX + 1];
   const char* floppy_container = NULL;
   struct dirent** dir_list;
   int dir_list_count = 0;
   int i = 0;

   floppy_container = FCGX_GetParam( VAR_FLOPPIES_CONTAINER, req->envp );
   if( NULL == floppy_container ) {
      retval = RETVAL_PARAMS;
      goto cleanup;
   }

   floppy_root = FCGX_GetParam( VAR_FLOPPIES_ROOT, req->envp );
   if( NULL == floppy_root ) {
      retval = RETVAL_PARAMS;
      goto cleanup;
   }

   if( strlen( floppy_root ) > strlen( floppy_dir ) ) {
      retval = RETVAL_DIR;
      goto cleanup;
   }

   /* Get a link-friendly version of the path with the root chopped off. */
   memset( floppy_dir_web, '\0', PATH_MAX + 1 );
   if( 0 < strlen( floppy_root ) ) {
      strncpy(
         floppy_dir_web, &(floppy_dir[strlen( floppy_root )]), PATH_MAX );
   }

   FCGX_FPrintF( req->out, "<!doctype HTML>\n<html>\n<head>\n" );
   FCGX_FPrintF( req->out, "<title>%s</title>\n", floppy_dir_web );
   FCGX_FPrintF( req->out,
      "<link rel=\"stylesheet\" href=\"/style.css\" />\n" );
   FCGX_FPrintF( req->out, "</head>\n<body>\n" );

   FCGX_FPrintF( req->out,
      "<h1><img src=\"/floppysrv.png\" />Floppy Disk Server</h1>\n" );

   /* Display currently mounted image. */

   FCGX_FPrintF( req->out, "<h2>Current Mounted</h2>\n" );

   memset( floppy_container_txt, '\0', PATH_MAX + 1 );
   snprintf( floppy_container_txt, PATH_MAX, "%s.txt", floppy_container );
   floppy_container_txt_f = open( floppy_container_txt, O_RDONLY );
   if( 0 <= floppy_container_txt_f ) {
      memset( floppy_mounted, '\0', PATH_MAX + 1 );
      read( floppy_container_txt_f, floppy_mounted, PATH_MAX );
      close( floppy_container_txt_f );

      FCGX_FPrintF( req->out,
         "<p class=\"current-mounted\">%s</a>\n", floppy_mounted );
   } else {
      FCGX_FPrintF( req->out,
         "<p class=\"current-mounted\">(No image mounted.)</a>\n" );
   }

   /* Display file browser. */

   FCGX_FPrintF( req->out,
      "<h2>Current Path</h2>\n<p class=\"current-path\">%s</p>\n",
      floppy_dir_web );

   FCGX_FPrintF( req->out,
      "<h2>Select Image</h2>\n"
      "<form class=\"select-image\" action=\"%s\" method=\"post\">\n",
      floppy_dir_web );

   /* TODO: Read-only checkbox. */

   FCGX_FPrintF( req->out, "<ul>\n" );

   /* Create a link to the parent path. */
   if( 0 < strlen( floppy_dir_web ) ) {
      memset( parent_path, '\0', PATH_MAX + 1 );
      strncpy( parent_path, floppy_dir_web, PATH_MAX );
      parent_path_slash = strrchr( parent_path, '/' );
      if( 1 < strlen( parent_path ) && NULL != parent_path_slash ) {
         parent_path_slash[0] = '\0';
      }
      if( 0 == strlen( parent_path ) ) {
         /* Add a slash at the end so we go to the root at minimum. */
         parent_path[0] = '/';
         parent_path[1] = '\0';
      }
      FCGX_FPrintF( req->out,
          "<li class=\"dir\"><a href=\"%s\">..</a></li>\n", parent_path );
   }

   dir_list_count = scandir( floppy_dir, &dir_list, NULL, alphasort );
   if( -1 == dir_list_count ) {
      retval = RETVAL_DIR;
      goto cleanup;
   }

   while( dir_list_count > i ) {
      if( '.' == dir_list[i]->d_name[0] ) {
         /* Skip hidden files. */
         goto dir_list_cleanup;
      }

      memset( ent_path, '\0', PATH_MAX + 1 );
      join_path( ent_path, PATH_MAX, floppy_dir );
      join_path( ent_path, PATH_MAX, dir_list[i]->d_name );

      if( stat( ent_path, &ent_stat ) ) {
         goto dir_list_cleanup;
      }

      /* TODO: URLencode paths. */

      if( S_IFDIR == (S_IFDIR & ent_stat.st_mode) ) {
         /* Link to directory. */
         FCGX_FPrintF( req->out,
            "<li class=\"dir\"><a href=\"%s/%s\">%s</a></li>\n",
            &(floppy_dir[strlen( floppy_root ) + 1]),
            dir_list[i]->d_name,
            dir_list[i]->d_name );
      } else {
         /* POST button for file. */
         FCGX_FPrintF( req->out,
            "<li class=\"floppy %s\">"
            "<input type=\"radio\" id=\"im-%d\" name=\"image\" value=\"%s\" />"
            "<label for=\"im-%d\">%s</label></li>\n",
            737280 == ent_stat.st_size ? "floppy-720" :
               1474560 == ent_stat.st_size ? "floppy-1440" :
                  1720320 == ent_stat.st_size ? "floppy-1680" :
                     2949120 == ent_stat.st_size ? "floppy-2880" :
                        "floppy-unknown",
            idx, dir_list[i]->d_name, idx,
            dir_list[i]->d_name );
         idx++;
      }

dir_list_cleanup:
      free( dir_list[i] );
      i++;
   }

   FCGX_FPrintF( req->out, "</ul>\n" );

   FCGX_FPrintF( req->out,
       "<input type=\"submit\" name=\"action\" value=\"Mount\" />" );

   FCGX_FPrintF( req->out,
       "<input type=\"submit\" name=\"action\" value=\"Eject\" />" );

   FCGX_FPrintF( req->out, "</form>\n" );

   FCGX_FPrintF( req->out, "</body>\n</html>\n" );

cleanup:

   if( NULL != dir_list ) {
      free( dir_list );
   }

   return retval;
}

int unmount_image( FCGX_Request* req ) {
   const char* floppy_container = NULL;
   char floppy_container_txt[PATH_MAX + 1];
   int retval = 0;
   struct stat ent_stat;

   /* TODO: Check command lengths against truncation. */

   floppy_container = FCGX_GetParam( VAR_FLOPPIES_CONTAINER, req->envp );
   if( NULL == floppy_container ) {
      retval = RETVAL_PARAMS;
      goto cleanup;
   }

   memset( floppy_container_txt, '\0', PATH_MAX + 1 );
   snprintf( floppy_container_txt, PATH_MAX, "%s.txt", floppy_container );

   system( "sudo rmmod g_mass_storage" );

   if( !stat( floppy_container_txt, &ent_stat ) ) {
      unlink( floppy_container_txt );
   }

   if( !stat( floppy_container, &ent_stat ) ) {
      unlink( floppy_container );
   }

cleanup:

   return retval;
}

int mount_image(
    FCGX_Request* req, const char* dir_path, const char* image_name
) {
   char path_buf[PATH_MAX + 1];
   char cmd_mount[PATH_MAX + 1];
   const char* floppy_root = NULL;
   const char* floppy_container = NULL;
   char floppy_container_txt[PATH_MAX + 1];
   int retval = 0;
   int floppy_c_f = -1;
   int floppy_container_txt_f = -1;
   struct stat ent_stat;
   int ff_cfg_f = -1;
   char ff_cfg_path[] = "/tmp/ffcfgXXXXXX";

   /* TODO: Check command lengths against truncation. */

   debug_printf( "mounting: %s\n", image_name );

   floppy_root = FCGX_GetParam( VAR_FLOPPIES_ROOT, req->envp );
   if( NULL == floppy_root ) {
      retval = RETVAL_PARAMS;
      goto cleanup;
   }

   floppy_container = FCGX_GetParam( VAR_FLOPPIES_CONTAINER, req->envp );
   if( NULL == floppy_container ) {
      retval = RETVAL_PARAMS;
      goto cleanup;
   }

   memset( floppy_container_txt, '\0', PATH_MAX + 1 );
   snprintf( floppy_container_txt, PATH_MAX, "%s.txt", floppy_container );

   memset( path_buf, '\0', PATH_MAX + 1 );

   /* Figure out path to floppy image. */
   join_path( path_buf, PATH_MAX, floppy_root );
   join_path( path_buf, PATH_MAX, dir_path );
   join_path( path_buf, PATH_MAX, image_name );

   /* Remove container if it exists. */
   if( !stat( floppy_container, &ent_stat ) ) {
      unlink( floppy_container );
   }

   /* Create image file. */
   floppy_c_f = open( floppy_container, O_WRONLY | O_CREAT,
      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
   if( 0 > floppy_c_f ) {
      retval = RETVAL_CONT;
      goto cleanup;
   }

   ftruncate( floppy_c_f, FLOPPIES_CONTAINER_SZ );
   close( floppy_c_f );

   memset( cmd_mount, '\0', PATH_MAX + 1 );
   snprintf( cmd_mount, PATH_MAX, "mkfs.vfat \"%s\"", floppy_container );
   if( system( cmd_mount ) ) {
      retval = RETVAL_CMD;
      goto cleanup;
   }

   memset( cmd_mount, '\0', PATH_MAX + 1 );
   snprintf( cmd_mount, PATH_MAX, "mcopy -i \"%s\" \"%s\" \"::/%s\"",
      floppy_container, path_buf, image_name );
   if( system( cmd_mount ) ) {
      retval = RETVAL_CMD;
      goto cleanup;
   }

   /* Write and copy FF.CFG. */
   ff_cfg_f = mkstemp( ff_cfg_path );
   if( 0 > ff_cfg_f ) {
      retval = RETVAL_TMP;
      goto cleanup;
   }
   dprintf( ff_cfg_f, FLOPPIES_FF_CFG, "no", image_name );
   memset( cmd_mount, '\0', PATH_MAX + 1 );
   snprintf( cmd_mount, PATH_MAX, "mcopy -i \"%s\" \"%s\" \"::/FF.CFG\"",
      floppy_container, ff_cfg_path );
   if( system( cmd_mount ) ) {
      retval = RETVAL_CMD;
      goto cleanup;
   }
   unlink( ff_cfg_path );
   close( ff_cfg_f );

   /* Figure out command with container image path. */
   memset( cmd_mount, '\0', PATH_MAX + 1 );
   snprintf( cmd_mount, PATH_MAX,
      "sudo modprobe g_mass_storage file=\"%s\" stall=0", floppy_container );

   /* Execute mount commands. */
   system( "sudo rmmod g_mass_storage" );
   if( system( cmd_mount ) ) {
      retval = RETVAL_CMD;
      goto cleanup;
   }

   if( !stat( floppy_container_txt, &ent_stat ) ) {
      unlink( floppy_container_txt );
   }

   floppy_container_txt_f = open( floppy_container_txt, O_WRONLY | O_CREAT,
      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
   if( 0 > floppy_container_txt_f ) {
      retval = RETVAL_TMP;
      goto cleanup;
   }
   dprintf( floppy_container_txt_f, "%s", image_name );
   close( floppy_container_txt_f );

cleanup:

   /* if( !stat( floppy_container, &ent_stat ) ) {
      unlink( floppy_container );
   } */

   return retval;
}

int send_file( FCGX_Request* req, const char* path, const char* mtype ) {
   int file_f = -1;
   int retval = 0;
   struct stat file_stat;
   char* file_buf = NULL;
   char asset_path[PATH_MAX + 1];
   const char* assets_root = NULL;

   /* Figure out the assets path. */
   assets_root = FCGX_GetParam( VAR_FLOPPIES_ASSETS, req->envp );
   if( NULL == assets_root ) {
      retval = RETVAL_PARAMS;
      goto cleanup;
   }

   memset( asset_path, '\0', PATH_MAX + 1 );
   join_path( asset_path, PATH_MAX, assets_root );
   join_path( asset_path, PATH_MAX, path );

   /* Make sure the file exists. */
   if( stat( asset_path, &file_stat ) ) {
      retval = RETVAL_FILE;
      goto cleanup;
   }

   file_buf = calloc( 1, file_stat.st_size );
   if( NULL == file_buf ) {
      retval = RETVAL_ALLOC;
      goto cleanup;
   }

   file_f = open( asset_path, O_RDONLY );
   if( 0 > file_f ) {
      retval = RETVAL_FILE;
      goto cleanup;
   }

   read( file_f, file_buf, file_stat.st_size );

   FCGX_FPrintF( req->out, "Content-type: %s\r\n", mtype );

   FCGX_FPrintF( req->out, "\r\n" );

   FCGX_PutStr( file_buf, file_stat.st_size, req->out );

cleanup:

   if( NULL != file_buf ) {
      free( file_buf );
   }

   if( 0 <= file_f ) {
      close( file_f );
   }

   return retval;
}

int handle_req( FCGX_Request* req ) {
   int retval = 0;
   const char* req_type = NULL;
   const char* req_uri_raw = NULL;
   char* req_uri = NULL;
   char floppy_dir[PATH_MAX + 1];
   struct stat ent_stat;
   char* post_buf = NULL;
   int post_buf_sz = 0;
   char* image_sel_raw = NULL;
   char* image_sel = NULL;
   const char* floppy_root = NULL;
   char* action_buf = NULL;
   char* req_ext = NULL;
   char* req_name = NULL;

   floppy_root = FCGX_GetParam( VAR_FLOPPIES_ROOT, req->envp );
   if( NULL == floppy_root ) {
      retval = RETVAL_PARAMS;
      goto cleanup;
   }

   /* Figure out our request type and consequent action. */
   req_type = FCGX_GetParam( "REQUEST_METHOD", req->envp );
   if( NULL == req_type ) {
      retval = RETVAL_PARAMS;
      goto cleanup;
   }

   /* TODO: URLdecode paths. */
   req_uri_raw = FCGX_GetParam( "DOCUMENT_URI", req->envp );
   if( NULL == req_uri_raw ) {
      retval = RETVAL_PARAMS;
      goto cleanup;
   }
   
   /* Fix path URL encoding. */
   retval = parse_decode( req_uri_raw, strlen( req_uri_raw ), &req_uri );
   if( retval ) {
      goto cleanup;
   }

   memset( floppy_dir, '\0', PATH_MAX + 1 );
   join_path( floppy_dir, PATH_MAX, floppy_root );
   join_path( floppy_dir, PATH_MAX, req_uri );

   /* Make sure path isn't dangerous or weird. */
   if(
      0 == strncmp( "..", req_uri, 3 ) ||
      0 == strncmp( ".", req_uri, 2 )
   ) {
      FCGX_FPrintF( req->out, "Status: 404 Bad Request\r\n\r\n" );
      goto cleanup;
   }

   req_ext = strrchr( req_uri, '.' );
   req_name = strrchr( req_uri, '/' );
   if( NULL == req_name ) {
      /* No directory separator present. */
      req_name = req_uri;
   } else {
      req_name = &(req_name[1]);
   }

   if( 0 == strncmp( "GET", req_type, 3 ) ) {
      if( 0 == strncmp( "/favicon.ico", req_uri, 13 ) ) {
         retval = send_file( req, "favicon.ico", "image/vnd.microsoft.icon" );

      } else if( 0 == strncmp( "/style.css", req_uri, 11 ) ) {
         retval = send_file( req, "style.css", "text/css" );

      } else if( NULL != req_ext && 0 == strncmp( ".png", req_ext, 5 ) ) {
         retval = send_file( req, req_name, "image/png" );

      } else {
         /* Make sure path exists. */
         if( stat( floppy_dir, &ent_stat ) ) {
            FCGX_FPrintF( req->out, "Status: 404 Bad Request\r\n\r\n" );
            goto cleanup;
         }

         /* Display the list of images. */
         FCGX_FPrintF( req->out, "Content-type: text/html\r\n" );

         FCGX_FPrintF( req->out, "\r\n" );

         retval = list_floppies( req, floppy_dir );
         if( retval ) {
            goto cleanup;
         }
      }
   } else if( 0 == strncmp( "POST", req_type, 4 ) ) {

      post_buf_sz = atoi( FCGX_GetParam( "CONTENT_LENGTH", req->envp ) );
      post_buf = calloc( post_buf_sz + 1, 1 );
      if( NULL == post_buf ) {
         retval = RETVAL_ALLOC;
         goto cleanup;
      }
      FCGX_GetStr( post_buf, post_buf_sz, req->in );

      retval = parse_post( post_buf, post_buf_sz, "action", &action_buf );
      if( retval || NULL == action_buf ) {
         /* Don't exit, just abort this request. */
         retval = 0;
         goto cleanup;
      }

      if( 0 == strncmp( "Mount", action_buf, 6 ) ) {
         /* Mount an image. */

         retval = parse_post( post_buf, post_buf_sz, "image", &image_sel_raw );
         if( retval ) {
            /* Don't exit, just abort this request. */
            retval = 0;
            goto cleanup;
         }

         retval = parse_decode( image_sel_raw, strlen( image_sel_raw ),
            &image_sel );
         if( retval || NULL == image_sel_raw ) {
            /* Don't exit, just abort this request. */
            retval = 0;
            goto cleanup;
         }
         debug_printf( "image_sel: %s\n", image_sel );

         retval = mount_image( req, req_uri, image_sel );
      } else if( 0 == strncmp( "Eject", action_buf, 6 ) ) {
         retval = unmount_image( req );
      }

      FCGX_FPrintF( req->out, "Status: 303 See Other\r\n" );
      FCGX_FPrintF( req->out, "Location: %s\r\n", req_uri_raw );
      FCGX_FPrintF( req->out, "Cache-Control: no-cache\r\n" );
      FCGX_FPrintF( req->out, "\r\n" );
   }

cleanup:

   if( NULL != image_sel ) {
      /* Free buffer allocated in parse_post(). */
      free( image_sel );
   }

   if( NULL != image_sel_raw ) {
      /* Free buffer allocated in parse_post(). */
      free( image_sel_raw );
   }

   if( NULL != action_buf ) {
      /* Free buffer allocated in parse_post(). */
      free( action_buf );
   }

   FCGX_Finish_r( req );

   return retval;
}

int main() {
   FCGX_Request req;
   int cgi_sock = -1;
   int retval = 0;

   FCGX_Init();
   memset( &req, 0, sizeof( FCGX_Request ) );

   cgi_sock = FCGX_OpenSocket( "127.0.0.1:9000", 100 );
   FCGX_InitRequest( &req, cgi_sock, 0 );
	while( 0 <= FCGX_Accept_r( &req ) ) {

      retval = handle_req( &req );
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

