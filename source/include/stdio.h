/* yaosp C library
 *
 * Copyright (c) 2009 Zoltan Kovacs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _STDIO_H_
#define _STDIO_H_

#include <stddef.h>
#include <stdarg.h>

#define _IONBF 0
#define _IOLBF 1
#define _IOFBF 2

#define __FILE_CAN_READ    0x01
#define __FILE_CAN_WRITE   0x02
#define __FILE_ERROR       0x04
#define __FILE_EOF         0x08
#define __FILE_BUFLINEWISE 0x10
#define __FILE_DONTFREEBUF 0x20
#define __FILE_NOBUF       0x40
#define __FILE_BUFINPUT    0x80

#define EOF -1

#define _IO_BUFSIZE 2048

#ifndef BUFSIZ
#define BUFSIZ _IO_BUFSIZE
#endif // BUFSIZ

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct FILE {
    int fd;
    int flags;
    char* buffer;
    unsigned int buffer_pos;
    unsigned int buffer_size;
    unsigned int buffer_data_size;
    int has_ungotten;
    unsigned char unget_buffer;
} FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

FILE* fopen( const char* path, const char* mode );
FILE* fdopen( int fd, const char* mode );
FILE* freopen( const char* path, const char* mode, FILE* stream );

int fclose( FILE* stream );
int feof( FILE* stream );
int ferror( FILE* stream );
int fileno( FILE* stream );
int fflush( FILE* stream );
int fseek( FILE* stream, long offset, int whence );
long ftell( FILE* stream );
size_t fread( void* ptr, size_t size, size_t nmemb, FILE* stream );
size_t fwrite( const void* ptr, size_t size, size_t nmemb, FILE* stream );
void rewind( FILE* stream );

int ungetc( int c, FILE* stream );
void clearerr( FILE* stream );

int printf( const char* format, ... ) __attribute__(( __format__( __printf__, 1, 2 ) ));
int fprintf( FILE* stream, const char* format, ... ) __attribute__(( __format__( __printf__, 2, 3 ) ));
int sprintf( char* str, const char* format, ... ) __attribute__(( __format__( __printf__, 2, 3 ) ));
int snprintf( char* str, size_t size, const char* format, ... ) __attribute__(( __format__( __printf__, 3, 4 ) ));

int vprintf( const char* format, va_list ap ) __attribute__(( __format__( __printf__, 1, 0 ) ));
int vfprintf( FILE* stream, const char* format, va_list ap ) __attribute__(( __format__( __printf__, 2, 0 ) ));
int vsprintf( char *str, const char *format, va_list ap ) __attribute__(( __format__( __printf__, 2, 0 ) ));
int vsnprintf( char* str, size_t size, const char* format, va_list ap ) __attribute__(( __format__( __printf__, 3, 0 ) ));

int scanf( const char* format, ... );
int fscanf( FILE* stream, const char* format, ... );
int sscanf( const char* str, const char* format, ... );

int fgetc( FILE* stream );
int getc( FILE* stream );
char* fgets( char* s, int size, FILE* stream );
#define getchar(c) getc(stdin)

int fputc( int c, FILE* stream );
int putc( int c, FILE* stream );
int fputs( const char* s, FILE* stream );
int puts( const char* s );
int putchar( int c );

#define setbuf(stream,buf) setvbuf(stream,buf,buf?_IOFBF:_IONBF,BUFSIZ)
int setvbuf( FILE* stream, char* buf, int flags, size_t size );

int rename( const char* oldpath, const char* newpath );
int remove( const char* path );

void perror( const char* s );

#endif // _STDIO_H_
