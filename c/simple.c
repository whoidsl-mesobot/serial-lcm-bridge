#include "bridges.h"
// ^ common header for both simple and complex bridge
// #includes: config.h, lcm.h, raw_bytes_t.h, r2_epoch.h, etc.

#include "simple.h"
#include "r2_sfd.h"


static void sfd_handle( const int sfd, const char * channel, lcm_t * lio ) {
    uint8_t data[MAX_LENGTH] = { 0 };
    raw_bytes_t msg = {
        .utime = r2_epoch_usec_now(),
        .length = 0,
        .data = data,
    };
    usleep( SLEEP_MICROSECONDS );
    msg.length = read( sfd, msg.data, MAX_LENGTH );
    if( args.verbosity > 1 ) {
        for( int j = 0; j < msg.length; j++ ) {
            putchar( msg.data[j] );
        }
        putchar( '\n' );
    }
    raw_bytes_t_publish( lio, channel, &msg );
}


int main( int argc, char ** argv ) {
    args.verbosity = 0;
    args.baudrate = B9600;
    argp_parse( &argp, argc, argv, 0, 0, &args );

    if( 0 > cfsetispeed( &tio, args.baudrate ) || 0 > cfsetospeed( &tio, args.baudrate ) ) {
        fprintf( stderr, "error setting baudrate\n" );
    }

    if( args.verbosity > 0 ) {
        printf( "opening serial port: %s\n", args.dev );
    }
    int sfd = r2_sfd_open( args.dev, &tio );
    if( -1 == sfd ) {
        fprintf( stderr, "could not open serial port: %s\n", args.dev );
    } else if( args.verbosity > 0 ) {
        printf( "opened serial port with file descriptor %d\n", sfd );
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
                    sfd_handle( sfd, output_channel, lio );
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
    close( sfd );

    exit( EXIT_SUCCESS );
}
