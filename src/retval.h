
#ifndef RETVAL_H
#define RETVAL_H

#define RETVAL_PARAMS 	0x01
#define RETVAL_DIR 	0x02
#define RETVAL_OVERFLOW 0x04
#define RETVAL_ALLOC  	0x08
#define RETVAL_CONT	0x10
#define RETVAL_TMP	0x20
#define RETVAL_FILE	0x40
#define RETVAL_CMD	0x80

#ifdef DEBUG
#   define debug_printf( ... ) dprintf( 1, __VA_ARGS__ )
#endif /* DEBUG */

#endif /* !RETVAL_H */

