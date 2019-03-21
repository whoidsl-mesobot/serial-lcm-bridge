/*  bridge.c
 */

#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#include <sys/time.h>
#include <lcm/lcm.h>

#include "raw_bytes_t.h"

#define INPUT_SUFFIX "<"
#define OUTPUT_SUFFIX ">"
#define MAX_LENGTH 255


//TODO copied for now to eliminate external dependency
int64_t r2_epoch_usec_now( void );

char output_channel[32] = "__serial-lcm-bridge__";

static FILE * sio_open( const char * device );

static void sio_handle( FILE * sio, lcm_t * lio );

static lcm_t * lio_open( void );

static void raw_handler( const lcm_recv_buf_t *rbuf, const char * channel,
        const raw_bytes_t * msg, void * user );

static void stream( FILE * sio, lcm_t * lio );

int64_t r2_epoch_usec_now( void )
{
    struct timeval utc;
    gettimeofday(&utc, NULL);
    return (int64_t)utc.tv_sec * 1000000 + (int64_t)utc.tv_usec;
}


static FILE * sio_open( const char * device ) {

    struct termios opts = {
        .c_iflag = 0,
        .c_oflag = 0,
        .c_cflag = B115200 | CS8 | CLOCAL | CREAD,
        .c_lflag = 0,
        .c_cc[VMIN] = 255,
        .c_cc[VTIME] = 1
    };

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


static void sio_handle( FILE * sio, lcm_t * lio ) {
    uint8_t data[MAX_LENGTH] = { 0 };
    raw_bytes_t msg = {
        .utime = r2_epoch_usec_now(),
        .length = 0,
        .data = data,
    };
    msg.length = read( fileno( sio ), msg.data, MAX_LENGTH );
    raw_bytes_t_publish( lio, output_channel, &msg );
}


static lcm_t * lio_open( void ) {
    lcm_t * lio = lcm_create( NULL );
    if( NULL == lio ) {
        fputs( "could not create LCM instance", stderr );
        exit( EXIT_FAILURE );
    }
    return lio;
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


static void stream( FILE * sio, lcm_t * lio ) {

    const int sfd = fileno( sio );
    const int lfd = lcm_get_fileno( lio );
    const int maxfd = sfd > lfd ? sfd + 1 : lfd + 1;
    fd_set rfds; // file descriptors to check for readability with select

    while( 1 <3 ) {

        // add both i/o file descriptors to the fdset for select
        FD_ZERO(&rfds);
        FD_SET(sfd, &rfds);
        FD_SET(lfd, &rfds);

        if( -1 != pselect( maxfd, &rfds, NULL, NULL, NULL, NULL ) ) {
            if( FD_ISSET( sfd, &rfds ) ) { // check for serial input first
                sio_handle( sio, lio ); // handle the serial input...
            }
            if( FD_ISSET( lfd, &rfds ) ) { // then check for LCM input
                lcm_handle( lio ); // handle all subscribed LCM input
            }
        }
    }
}


int main( int argc, char* argv[] ){

    char device[16] = "/dev/ttyUSB0";

    if( argc >= 2 ) strcpy( device, argv[1] );
    if( argc > 3 ) {
        fprintf( stderr, "Usage: %s [device]\n", argv[0] );
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
}
