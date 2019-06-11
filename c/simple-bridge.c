#include "config.h"

#define INPUT_SUFFIX "i"
#define OUTPUT_SUFFIX "o"
#define MAX_LENGTH 255

#include <argp.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <lcm/lcm.h>

#include "raw_bytes_t.h"

static char output_channel[32] = "__serial-lcm-bridge__";

const char *argp_program_version = "simple-serial-lcm-bridge in " PACKAGE_STRING;

const char *argp_program_bug_address = PACKAGE_BUGREPORT;

static char doc[] = "serial-lcm-bridge -- a bridge between serial device and LCM";
static char args_doc[] = "device";
static struct argp_option options[] = {
    { "verbose", 'v', 0, 0, "say more" },
    { "quiet", 'q', 0, 0, "say less" },
    { "baudrate", 'b', "baudrate", 0, "baudrate" },
    { "preserve-termios", 'p', 0, 0, "preserve termios options" },
    { 0 }
};

struct args {
    char *arg[1]; // device
    int verbosity;
    speed_t baudrate;
    int preserve_termios;
};

struct args arguments; // just leave it as a global for now

static error_t parse_opt( int key, char *arg, struct argp_state *state ) {
    struct args *args = state->input;
    switch( key ){
        case 'q':
            args->verbosity = -1;
            break;
        case 'v':
            args->verbosity += 1;
            break;
        case 'b':
            switch( atoi( arg ) ){
                case 9600:
                    args->baudrate = B9600;
                    break;
                case 19200:
                    args->baudrate = B19200;
                    break;
                case 38400:
                    args->baudrate = B38400;
                    break;
                case 57600:
                    args->baudrate = B57600;
                    break;
                case 115200:
                    args->baudrate = B115200;
                    break;
                case 230400:
                    args->baudrate = B230400;
                    break;
                default:
                    printf( "%s baud not supported\n", arg );
                    exit( EXIT_FAILURE );
            }
            break;
        case 'p':
            args->preserve_termios = 1;
            break;
        case ARGP_KEY_ARG:
            if( state->arg_num >= 1 ) argp_usage( state );
            args->arg[state->arg_num] = arg;
            break;
        case ARGP_KEY_END:
            if( state->arg_num < 1 ) argp_usage( state );
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };


//TODO copied for now to eliminate external dependency
int64_t r2_epoch_usec_now( void );

static FILE * sio_open( const char * device );

static void sio_handle( FILE * sio, lcm_t * lio );

static void raw_handler( const lcm_recv_buf_t *rbuf, const char * channel,
        const raw_bytes_t * msg, void * user );

static void stream( FILE * sio, lcm_t * lio );

int64_t r2_epoch_usec_now( void )
{
    struct timespec t;
    clock_gettime( CLOCK_REALTIME, &t );
    return (int64_t)t.tv_sec * 1000000 + (int64_t)t.tv_nsec / 1000;
}


FILE * sio_open( const char * device ) {

    FILE * fp = fopen( device, "w+b" );
    if( NULL == fp ) {
        perror( "fopen()" );
        fprintf( stderr, "could not open device %s\n", device );
        exit( EXIT_FAILURE );
    }
    return fp;
}

void sio_configure( FILE * fp, speed_t baudrate ) {
    struct termios opts = { 0 };
    opts.c_iflag = IGNBRK;
    opts.c_oflag = 0;
    opts.c_cflag = CS8 | CLOCAL | CREAD;
    opts.c_lflag = 0;
    opts.c_cc[VMIN] = 255;
    opts.c_cc[VTIME] = 1;

    opts.c_cflag |= baudrate;

    if( -1 == tcsetattr( fileno( fp ), TCSAFLUSH, &opts ) ) {
        perror( "tcsetattr()" );
        fputs( "trouble setting termios attributes", stderr );
        exit( EXIT_FAILURE );
    }

}


static void sio_handle( FILE * sio, lcm_t * lio ) {
    uint8_t data[MAX_LENGTH] = { 0 };
    raw_bytes_t msg = {
        .utime = r2_epoch_usec_now(),
        .length = 0,
        .data = data,
    };
    usleep( 10 );
    msg.length = read( fileno( sio ), msg.data, MAX_LENGTH );
    if( arguments.verbosity > 1 ) {
        for( int j = 0; j < msg.length; j++ ) {
            putchar( msg.data[j] );
        }
        putchar( '\n' );
    }    
    raw_bytes_t_publish( lio, output_channel, &msg );
}


static void raw_handler( const lcm_recv_buf_t *rbuf, const char * channel,
        const raw_bytes_t * msg, void * user ) {
    write( fileno( user ), msg->data, msg->length );
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


int main( int argc, char **argv ){

    arguments.verbosity = 0;
    arguments.baudrate = B38400;
    arguments.preserve_termios = 0;

    argp_parse( &argp, argc, argv, 0, 0, &arguments );
    char * device = arguments.arg[0];

    if( arguments.verbosity > 0 ) {
        printf( "opening device %s\n", arguments.arg[0] );
    }
    FILE * sio = sio_open( arguments.arg[0] );
    if( arguments.verbosity > 0 ) {
        printf( "opened device %s\n", arguments.arg[0] );
    }
    if( arguments.preserve_termios == 0 ) {
        sio_configure( sio, arguments.baudrate );
    }
    if( arguments.verbosity > 0 ) {
        printf( "starting LCM\n" );
    }
    lcm_t * lio = lcm_create( NULL );
    if( NULL == lio ) {
        fputs( "could not create LCM instance", stderr );
        exit( EXIT_FAILURE );
    }
    if( arguments.verbosity > 0 ) {
        printf( "started LCM\n" );
    }

    char tty[8] = { 0 };
    sscanf( device, "%*4c/%s", tty );
    char input_channel[strlen( tty ) + strlen( INPUT_SUFFIX )];
    strcpy( input_channel, tty );
    strcat( input_channel, INPUT_SUFFIX );
    // char output_channel[strlen( tty ) + strlen( OUTPUT_SUFFIX )];
    strcpy( output_channel, tty );
    strcat( output_channel, OUTPUT_SUFFIX );
    if( arguments.verbosity >= 0 ) {
        printf( "input channel: %s\n", input_channel );
        printf( "output channel: %s\n", output_channel );
    }

    raw_bytes_t_subscribe( lio, input_channel, &raw_handler, (void *) sio );

    stream( sio, lio );

    if( EOF == fclose( sio ) ) {
        fprintf( stderr, "fclose(): %s\n", strerror( ferror( sio ) ) );
        exit( EXIT_FAILURE );
    }
    lcm_destroy( lio );

    exit( EXIT_SUCCESS );
}
