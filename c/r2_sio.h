#ifndef _R2_SIO_H_
#define _R2_SIO_H_

#define R2_SIO_DEFAULT_DATA_SIZE 4096

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

static const struct termios R2_SIO_DEFAULT_TIO = {
        .c_cflag = CS8 | CLOCAL | CREAD | B9600,
        .c_iflag = IGNBRK,
        .c_oflag = 0,
        .c_lflag = 0,
        .c_cc[VMIN] = 0,
        .c_cc[VTIME] = 1,
};


FILE * r2_sio_open( const char * device, const struct termios * opts ) {
    FILE * fp = fopen( device, "w+b" );
    if( NULL == fp ) {
        perror( "fopen()" );
        fprintf( stderr, "could not open device %s\n", device );
        exit( EXIT_FAILURE );
    }
    if( -1 == tcsetattr( fileno( fp ), TCSAFLUSH, opts ) ) {
        perror( "tcsetattr()" );
        fputs( "trouble setting termios attibutes", stderr );
        exit( EXIT_FAILURE );
    }
    return fp;
}



#endif // _R2_SIO_H_
