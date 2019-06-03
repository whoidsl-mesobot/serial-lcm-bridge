#include <argp.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <lcm/lcm.h>

#include "raw_bytes_t.h"

#define INPUT_SUFFIX "i"
#define OUTPUT_SUFFIX "o"
#define MAX_LENGTH 255

const char *argp_program_version = "serial-lcm-source 0.x";
const char *argp_program_bug_address = "m.j.stanway@alum.mit.edu";

static char doc[] = "serial-lcm-source -- a source of messages to test the bridge between serial device and LCM";
static char args_doc[] = "device";
static struct argp_option options[] = {
    { "verbose", 'v', 0, 0, "say more" },
    { "quiet", 'q', 0, 0, "say less" },
    { 0 }
};

struct args {
    char *arg[1]; // device
    int verbosity;
};

struct args arguments; // just leave it as a global for now

static error_t parse_opt( int key, char *arg, struct argp_state *state ) {
    struct args *args = state->input;
    switch( key ){
        case 'q':
            args->verbosity -= 1;
            break;
        case 'v':
            args->verbosity += 1;
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


static void raw_publisher( uint8_t * payload, size_t n, const char * channel,
        lcm_t * lio );

static void raw_handler( const lcm_recv_buf_t *rbuf, const char * channel,
        const raw_bytes_t * msg, void * user );


int64_t r2_epoch_usec_now( void )
{
    struct timespec t;
    clock_gettime( CLOCK_REALTIME, &t );
    return (int64_t)t.tv_sec * 1000000 + (int64_t)t.tv_nsec / 1000;
}


// TODO: This can be streamlined with r2
static void raw_publisher( uint8_t * payload, size_t n, const char * channel,
        lcm_t * lio ) {
    raw_bytes_t msg = {
        .utime = r2_epoch_usec_now(),
        .length = n,
        .data = payload,
    };
    raw_bytes_t_publish( lio, channel, &msg );
    fprintf( stdout, "tx %d-byte message:\t", msg.length );
    for( int j = 0; j < msg.length; j++ ) {
        fprintf( stdout, " %02hhx", msg.data[j] );
    }
    fprintf( stdout, "\n" );
    fflush( stdout );
}


static void raw_handler( const lcm_recv_buf_t *rbuf, const char * channel,
        const raw_bytes_t * msg, void * user ) {
    fprintf( stdout, "rx %d-byte message:\t", msg->length );
    for( int j = 0; j < msg->length; j++ ) {
        fprintf( stdout, " %02hhx", msg->data[j] );
    }
    fprintf( stdout, "\n" );
    fflush( stdout );
}


int main( int argc, char **argv){

    arguments.verbosity = 0;

    argp_parse( &argp, argc, argv, 0, 0, &arguments );
    char * device = arguments.arg[0];

    char input_channel[strlen( device ) + strlen(INPUT_SUFFIX)];
    strcpy( input_channel, device );
    strcat( input_channel, INPUT_SUFFIX );
    char output_channel[strlen( device ) + strlen(OUTPUT_SUFFIX)];
    strcpy( output_channel, device );
    strcat( output_channel, OUTPUT_SUFFIX );
    if( arguments.verbosity >= 0 ) {
        printf( "input channel: %s\n", input_channel );
        printf( "output channel: %s\n", output_channel );
    }

    lcm_t * lio = lcm_create( NULL );
    if( NULL == lio ) {
        fputs( "could not create LCM instance", stderr );
        exit( EXIT_FAILURE );
    }

    raw_bytes_t_subscribe( lio, output_channel, &raw_handler, NULL );

    raw_publisher( (uint8_t *)"TEST\r\n", 6, input_channel, lio );

    struct pollfd pfd[1];
    pfd[0].fd = lcm_get_fileno( lio );
    pfd[0].events = POLLIN;
    switch ( poll( pfd, 1, 10000 ) ) {
        case -1:
            switch errno {
                case EINTR: // interrupted by a signal
                    break;
                default:
                    perror( "poll()" );
            }
            break;
        case 0: // timed out
            printf( "poll timed out\n" );
            break;
        default: // data available on serial or LCM -- always check both
            if( pfd[0].revents & POLLIN ) { // check for serial input
                if( arguments.verbosity > 1 ) {
                    printf( "handling LCM input on fd %d\n", pfd[1].fd );
                }
                lcm_handle( lio ); // handle the LCM input...
                if( arguments.verbosity > 1 ) {
                    printf( "handled serial input on fd %d\n", pfd[0].fd );
                }
            }
    }
    lcm_destroy( lio );

    exit( EXIT_SUCCESS );
}
