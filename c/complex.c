#include "bridges.h"
// ^ common header for both simple and complex bridge
// #includes: config.h, lcm.h, raw_bytes_t.h, r2_epoch.h, etc.

#include "complex.h"
#include "r2_sio.h"


static void sio_handle( FILE * sio, const char * channel, lcm_t * lio ) {
    raw_bytes_t msg = {
        .utime = r2_epoch_usec_now(),
        .length = 1,
    };
    uint8_t tmp[MAX_LENGTH];

    // TODO: check fgetc for EOF, etc.
    tmp[msg.length-1] = fgetc( sio );

    // only search for initiator if it is different from terminator
    if( args.initiator != args.terminator ) {
        while( args.initiator != tmp[msg.length-1] ) {
            // TODO: check fgetc for EOF, etc.
            tmp[msg.length-1] = fgetc( sio );
            if( args.verbosity > 1 ) {
                printf( "%02hhx ", tmp[msg.length-1] );
            }
        }
        if( args.verbosity > 1 ) printf( "\n" );
    }

    // add all the data up to and including the terminator
    while( args.terminator != tmp[msg.length-1] ) {
        if( MAX_LENGTH == msg.length ) { // TODO or if there is no more data in the buffer or on the file descriptor
            fprintf( stderr, "terminator not found, sending %d bytes\n",
                    msg.length );
            msg.data = tmp;
            raw_bytes_t_publish( lio, channel, &msg );
            msg.length = 0;
        }
        // TODO: check return value of fgetc
        tmp[msg.length] = fgetc( sio );
        msg.length++;
    }

    msg.data = tmp;
    raw_bytes_t_publish( lio, channel, &msg );
}


int main( int argc, char ** argv ) {
    args.verbosity = 0;
    args.baudrate = B9600;
    args.terminator = 0x0a;
    args.initiator = args.terminator;
    argp_parse( &argp, argc, argv, 0, 0, &args );

    if( 0 > cfsetispeed( &tio, args.baudrate ) || 0 > cfsetospeed( &tio, args.baudrate ) ) {
        fprintf( stderr, "error setting baudrate\n" );
    }

    if( args.verbosity >= 0 ) {
        printf( "initiator: 0x%02hhx '%c'\n", args.initiator,
                args.initiator );
        printf( "terminator: 0x%02hhx '%c'\n", args.terminator,
                args.terminator );
    }

    if( args.verbosity > 0 ) {
        printf( "opening serial port: %s\n", args.dev );
    }
    FILE * sio = r2_sio_open( args.dev, &tio );
    if( NULL == sio ) {
        fprintf( stderr, "could not open serial port: %s\n", args.dev );
    }
    int sfd = fileno( sio );
    if( args.verbosity > 0 ) {
        printf( "opened serial port file stream with file descriptor %d\n", sfd );
    }

    if( args.verbosity > 0 ) {
        printf( "starting LCM\n" );
    }
    lcm_t * lio = lcm_create( NULL );
    if( NULL == lio ) {
        fputs( "could not create LCM instance\n", stderr );
        exit( EXIT_FAILURE );
    }
    int lfd = lcm_get_fileno( lio );
    if( args.verbosity > 0 ) {
        printf( "started LCM with file descriptor %d\n", lfd );
    }

    // TODO: stub out to bridges.h
    char tty[8] = { 0 };
    sscanf( args.dev, "%*4c/%s", tty );
    char input_channel[strlen( tty ) + strlen( INPUT_SUFFIX )];
    strcpy( input_channel, tty );
    strcat( input_channel, INPUT_SUFFIX );
    char output_channel[strlen( tty ) + strlen( OUTPUT_SUFFIX )];
    strcpy( output_channel, tty );
    strcat( output_channel, OUTPUT_SUFFIX );
    if( args.verbosity >= 0 ) {
        printf( "input channel: %s\n", input_channel );
        printf( "output channel: %s\n", output_channel );
    }

    raw_bytes_t_subscribe( lio, input_channel, &raw_handler, (void *)&sfd );

    // set up epoll to listen for input
    struct epoll_event ev = { 0 };
    ev.events = EPOLLIN;
    int epfd = epoll_create( 1 );
    if( -1 == epfd ) {
        perror( "epoll_create" );
        fputs( "failed to create epoll file descriptor\n", stderr );
        exit( EXIT_FAILURE );
    } else if ( args.verbosity > 1 ) {
        printf( "created epoll %d\n", epfd );
    }
    // add serial file descriptor to epoll
    ev.data.fd = sfd;
    if( -1 == epoll_ctl( epfd, EPOLL_CTL_ADD, ev.data.fd, &ev ) ) {
        perror( "epoll_ctl" );
        fprintf( stderr, "failed to add serial fd %d to epoll", ev.data.fd );
        exit( EXIT_FAILURE );
    } else if ( args.verbosity > 0 ) {
        printf( "added serial port fd %d to epoll\n", ev.data.fd );
    }
    // add LCM file descriptor to epoll
    ev.data.fd = lfd;
    if( -1 == epoll_ctl( epfd, EPOLL_CTL_ADD, ev.data.fd, &ev ) ) {
        perror( "epoll_ctl" );
        fprintf( stderr, "failed to add LCM fd %d to epoll", ev.data.fd );
        exit( EXIT_FAILURE );
    } else if ( args.verbosity > 0 ) {
        printf( "added LCM fd %d to epoll\n", ev.data.fd );
    }
    // clear epoll event to re-use
    memset( &ev, 0, sizeof( ev ) );
    int nfds = 0;
    if( args.verbosity > 0 ) {
        puts( "starting epoll loop" );
    }
    int loop = 1;
    while( loop ) {
        nfds = epoll_wait( epfd, &ev, 1, -1 );
        switch( nfds ) {
            case -1:
                perror( "epoll_wait" );
                loop = 0;
                break;
            case 1:
                if( args.verbosity > 2 ) {
                    printf( " epoll says %d is readable\n", ev.data.fd );
                }
                if( !( ev.events & EPOLLIN ) ) {
                    fprintf( stderr, "something unexpected happened with epoll"
                            " and it triggered without EPOLLIN\n" );
                    continue;
                } else if ( sfd == ev.data.fd ) {
                    sio_handle( sio, output_channel, lio );
                } else if ( lfd == ev.data.fd ) {
                    lcm_handle( lio );
                } else {
                    fprintf( stderr, "unexpected fd %d\n", ev.data.fd );
                    loop = 0;
                }
                break;
            default:
                fprintf( stderr, "epoll says %d fds are ready", nfds );
        }
    }

    close( epfd );
    lcm_destroy( lio );
    if( EOF == fclose( sio ) ) {
        fprintf( stderr, "fclose(): %s\n", strerror( ferror( sio ) ) );
        exit( EXIT_FAILURE );
    }

    exit( EXIT_SUCCESS );
}
