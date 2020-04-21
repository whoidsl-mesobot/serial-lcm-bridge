#ifndef _R2_SFD_H_
#define _R2_SFD_H_

#define R2_SFD_DEFAULT_DATA_SIZE 255

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

static const struct termios R2_SFD_DEFAULT_TIO = {
        .c_cflag = CS8 | CLOCAL | CREAD | B9600,
        .c_iflag = IGNBRK,
        .c_oflag = 0,
        .c_lflag = 0,
        .c_cc[VMIN] = 0,
        .c_cc[VTIME] = 1,
};


int r2_sfd_open( const char * device, const struct termios * opts ) {
    int fd = open( device, O_RDWR | O_NOCTTY );
    if( -1 == fd ) {
        perror( "open()" );
        fprintf( stderr, "could not open device %s\n", device );
        exit( EXIT_FAILURE );
    }
    if( -1 == tcsetattr( fd, TCSAFLUSH, opts ) ) {
        perror( "tcsetattr()" );
        fputs( "trouble setting termios attibutes", stderr );
        exit( EXIT_FAILURE );
    }
    return fd;
}


ssize_t r2_sfd_read_until( const int sfd, char * data, size_t data_size, const char terminator ) {
    ssize_t pos = 0;
    ssize_t bytes_read = 0;

    while( ( -1 != bytes_read )
            && ( data_size > pos )
            && ( terminator != data[pos-1] ) ) {
        bytes_read = read( sfd, &(data[pos]), data_size - pos );
        if( -1 == bytes_read ) {
            perror( "read()" );
            fprintf( stderr, " ...ran out of input searching for terminator (%c)\n", terminator );
        }
        pos += bytes_read;
    }
    if( terminator == data[pos-1] ) {
        return pos;
    } else {
        return -1;
    }
}

#endif // _R2_SFD_H_
