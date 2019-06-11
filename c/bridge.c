#include "config.h"

#define MAX_SERIAL_DATA_LENGTH 4096
#define INPUT_SUFFIX "i"
#define OUTPUT_SUFFIX "o"

#include <argp.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <lcm/lcm.h>

#include "raw_bytes_t.h"

static char output_channel[32] = "__serial-lcm-bridge__";

const char *argp_program_version = PACKAGE_STRING;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;

static char doc[] = "serial-lcm-bridge -- a bridge between serial device and LCM";
static char args_doc[] = "device";
static struct argp_option options[] = {
    { "verbose", 'v', 0, 0, "say more" },
    { "quiet", 'q', 0, 0, "say less" },
    { "baudrate", 'b', "baudrate", 0, "baudrate" },
    { "terminator", 't', "terminator", 0, "terminator" },
    { "initiator", 'i', "initiator", 0, "initiator" },
    { "preserve-termios", 'p', 0, 0, "preserve termios options" },
    { 0 }
};

struct args {
    char *arg[1]; // device
    int verbosity;
    speed_t baudrate;
    char terminator;
    char initiator;
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
        case 't':
            if( 1 != sscanf( arg, "%02hhx", &(args->terminator) ) ) {
                argp_usage( state );
            }
            break;
        case 'i':
            if( 1 != sscanf( arg, "%02hhx", &(args->initiator) ) ) {
                argp_usage( state );
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

static void sio_handle( FILE * sio, lcm_t * lio );

void sio_publish( char * b, size_t n, lcm_t * lio, const int64_t epoch_usec );


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
    opts.c_cc[VMIN] = 0;
    opts.c_cc[VTIME] = 1;

    opts.c_cflag |= baudrate;

    cfmakeraw( &opts );

    if( -1 == tcsetattr( fileno( fp ), TCSAFLUSH, &opts ) ) {
        perror( "tcsetattr()" );
        fputs( "trouble setting termios attributes", stderr );
        exit( EXIT_FAILURE );
    }

}


static void sio_handle( FILE * sio, lcm_t * lio ) {
    raw_bytes_t msg = {
        .utime = r2_epoch_usec_now(),
        .length = 1,
    };
    uint8_t tmp[MAX_SERIAL_DATA_LENGTH];

    // TODO: check fgetc for EOF, etc.
    tmp[msg.length-1] = fgetc( sio );

    // only search for initiator if it is different from terminator
    if( arguments.initiator != arguments.terminator ) {
        while( arguments.initiator != tmp[msg.length-1] ) {
            // TODO: check fgetc for EOF, etc.
            tmp[msg.length-1] = fgetc( sio );
            if( arguments.verbosity > 1 ) {
                printf( "%02hhx ", tmp[msg.length-1] );
            }
        }
        if( arguments.verbosity > 1 ) printf( "\n" );
    }

    // add all the data up to and including the terminator
    while( arguments.terminator != tmp[msg.length-1] ) {
        if( MAX_SERIAL_DATA_LENGTH == msg.length ) { // TODO or if there is no more data in the buffer or on the file descriptor
            fprintf( stderr, "terminator not found, sending %d bytes\n",
                    msg.length );
            msg.data = tmp;
            raw_bytes_t_publish( lio, output_channel, &msg );
            msg.length = 0;
        }
        // TODO: check return value of fgetc
        tmp[msg.length] = fgetc( sio );
        msg.length++;
    }

    msg.data = tmp;
    raw_bytes_t_publish( lio, output_channel, &msg );
}


static void raw_handler( const lcm_recv_buf_t *rbuf, const char * channel,
        const raw_bytes_t * msg, void * user ) {
    fwrite( msg->data, 1, msg->length, user );
    fflush( user );
}


void stream( FILE * sio, lcm_t * lio ) {
    int i = 0;
    struct pollfd pfd[2];
    pfd[0].fd = fileno( sio );
    pfd[0].events = POLLIN;
    pfd[1].fd = lcm_get_fileno( lio );
    pfd[1].events = POLLIN;
    while( i <3 ) {
        switch ( poll( pfd, 2, 1000 ) ) {
            case -1:
                switch errno {
                    case EINTR: // interrupted by a signal
                        break;
                    default:
                        perror( "poll()" );
                        i = 8;
                }
                break;
            case 0: // timed out
                if( arguments.verbosity > 1 ) {
                    printf( "poll timed out\n" );
                }
                break;
            default: // data available on serial or LCM -- always check both
                if( pfd[0].revents & POLLIN ) { // check for serial input
                    if( arguments.verbosity > 1 ) {
                        printf( "handling serial input on fd %d\n", pfd[0].fd );
                    }
                    sio_handle( sio, lio ); // handle the serial input...
                    if( arguments.verbosity > 1 ) {
                        printf( "handled serial input on fd %d\n", pfd[0].fd );
                    }
                }
                if( pfd[1].revents & POLLIN ) { // check for LCM input
                    if( arguments.verbosity > 1 ) {
                        printf( "handling LCM input on fd %d\n", pfd[1].fd );
                    }
                    lcm_handle( lio ); // handle all subscribed LCM input
                    if( arguments.verbosity > 1 ) {
                        printf( "handled LCM input on fd %d\n", pfd[1].fd );
                    }
                }
        }
    }
}


int main( int argc, char **argv ){

    arguments.verbosity = 0;
    arguments.baudrate = B115200;
    arguments.preserve_termios = 0;
    arguments.terminator = 0x0a;
    arguments.initiator = arguments.terminator;

    argp_parse( &argp, argc, argv, 0, 0, &arguments );
    char * device = arguments.arg[0];

    if( arguments.verbosity >= 0 ) {
        printf( "initiator: 0x%02hhx '%c'\n", arguments.initiator,
                arguments.initiator );
        printf( "terminator: 0x%02hhx '%c'\n", arguments.terminator,
                arguments.terminator );
    }

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
