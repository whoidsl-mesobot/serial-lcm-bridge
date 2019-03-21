/*  bridge.c
 */

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/time.h>
#include <unistd.h>

#include <lcm/lcm.h>

#include "raw_bytes_t.h"

#define INPUT_SUFFIX "<"
#define OUTPUT_SUFFIX ">"
#define MAX_SERIAL_DATA_LENGTH 4096


//TODO copied for now to eliminate external dependency
int64_t r2_epoch_usec_now( void );


unsigned char terminator = '\n';
unsigned char initiator = '\n';
char output_channel[32] = "__serial-lcm-bridge__";


FILE * sio_open( const char * device );


static void sio_handle( FILE * sio, lcm_t * lio );


int64_t r2_epoch_usec_now( void )
{
    struct timeval utc;
    gettimeofday(&utc, NULL);
    return (int64_t)utc.tv_sec * 1000000 + (int64_t)utc.tv_usec;
}


FILE * sio_open( const char * device ) {

    struct termios opts = {
        .c_iflag = 0,
        .c_oflag = 0,
        .c_cflag = B115200 | CS8 | CLOCAL | CREAD,
        .c_lflag = 0,
        .c_cc[VMIN] = 0,
        .c_cc[VTIME] = 0
    };
    cfmakeraw( &opts );

    FILE * fp = fopen( device, "r+" );
    if( NULL == fp ) {
        perror( "fopen()" );
        fprintf( stderr, "could not open device %s\n", device );
        exit( EXIT_FAILURE );
    }
    if( -1 == tcsetattr( fileno( fp ), TCSAFLUSH, &opts ) ) {
        perror( "tcsetattr()" );
        fputs( "trouble setting termios attributes", stderr );
        exit( EXIT_FAILURE );
    }

    return fp;
}


void sio_close( FILE * sio ) {
    if( EOF == fclose( sio ) ) {
        fprintf( stderr, "fclose(): %s\n", strerror( ferror( sio ) ) );
        exit( EXIT_FAILURE );
    }
}


static void sio_handle( FILE * sio, lcm_t * lio ) {
    raw_bytes_t msg = {
        .utime = r2_epoch_usec_now(),
        .length = 1,
    };
    uint8_t tmp[MAX_SERIAL_DATA_LENGTH];

    tmp[msg.length-1] = fgetc( sio );

    // only search for initiator if it is different from terminator
    if( initiator != terminator ) {
        fprintf( stderr, "searching for initiator %02hhX... ", initiator );
        while( initiator != tmp[msg.length-1] ) {
            tmp[msg.length-1] = fgetc( sio ); // want fgetc w/ timeout in case initiator is not there... maybe just go back to read(1)
            fprintf( stderr, " %02hhX", tmp[msg.length-1] );
        }
        fputs( " ...found initiator\n", stderr );
    }

    // add all the data up to and including the terminator
    while( terminator != tmp[msg.length-1] ) {
        fprintf( stderr, " %02hhX", tmp[msg.length-1] );
        if( MAX_SERIAL_DATA_LENGTH == msg.length ) { // TODO or if there is no more data in the buffer or on the file descriptor
            fprintf( stderr, "terminator not found, sending %d bytes\n",
                    msg.length );
            msg.data = tmp;
            raw_bytes_t_publish( lio, output_channel, &msg );
            msg.length = 0;
        }
        tmp[msg.length] = fgetc( sio );
        msg.length++;
    }

    msg.data = tmp;
    raw_bytes_t_publish( lio, output_channel, &msg );
}


void sio_publish( char * b, size_t n, lcm_t * lio, const int64_t epoch_usec ) {
}


lcm_t * lio_open( void ) {
    lcm_t * lio = lcm_create( NULL );
    if( NULL == lio ) {
        fputs( "could not create LCM instance", stderr );
        exit( EXIT_FAILURE );
    }
    return lio;
}


void lio_close( lcm_t * lio ) {
    lcm_destroy( lio );
}


static void raw_handler( const lcm_recv_buf_t *rbuf, const char * channel,
        const raw_bytes_t * msg, void * user ) {
    printf( "rx %d-byte message:\t", msg->length );
    for( int j = 0; j < msg->length; j++ ) {
        fputc( msg->data[j], user );
        printf( " %02hhX", msg->data[j] );
    }
    fflush( user );
    printf( "\n" );
}


void stream( FILE * sio, lcm_t * lio ) {

    const int sfd = fileno( sio );
    const int lfd = lcm_get_fileno( lio );
    const int maxfd = sfd > lfd ? sfd + 1 : lfd + 1;
    fd_set rfds; // file descriptors to check for readability with select

    while( true ) {

        // add both i/o file descriptors to the fdset for select
        FD_ZERO(&rfds);
        FD_SET(sfd, &rfds);
        FD_SET(lfd, &rfds);

        if( -1 != pselect( maxfd, &rfds, NULL, NULL, NULL, NULL ) ) {
            if( FD_ISSET( sfd, &rfds ) ) { // check for serial input first
                puts( "handling serial input\n" );
                sio_handle( sio, lio ); // handle the serial input...
                puts( "\nserial input handled\n" );
            }
            if( FD_ISSET( lfd, &rfds ) ) { // then check for LCM input
                puts( "handling LCM input\n" );
                lcm_handle( lio ); // handle all subscribed LCM input
                puts( "LCM input handled\n" );
            }
        }
    }
}


int main( int argc, char* argv[] ){

    char device[16] = "/dev/ttyUSB0";

    if( argc >= 2 ) strcpy( device, argv[1] );
    if( argc >= 3 ) {
        terminator = argv[2][0];
        fprintf( stdout, "terminator:\t0x%02hhX\t`%c'\n",
                terminator, terminator );
    }
    if( argc >= 4 ) {
        initiator = argv[3][0];
        fprintf( stdout, "initiator:\t0x%02hhX\t`%c'\n",
                initiator, initiator );
    }
    if( argc >= 5 ) {
        fprintf( stderr, "Usage: %s [device] [terminator] [initiator]\n",
                argv[0] );
        exit( EXIT_FAILURE );
    }


    FILE * sio = sio_open( device );
    lcm_t * lio = lio_open( );

    char input_channel[strlen( device ) + strlen( INPUT_SUFFIX )];
    strcpy( input_channel, device );
    strcat( input_channel, INPUT_SUFFIX );

    strcpy( output_channel, device );
    strcat( output_channel, OUTPUT_SUFFIX );

    raw_bytes_t_subscribe( lio, input_channel, &raw_handler, (void *) sio );

    printf( "input channel: %s\n", input_channel );
    printf( "output channel: %s\n", output_channel );

    stream( sio, lio );

    sio_close( sio );
    lio_close( lio );

    exit( EXIT_SUCCESS );
}
