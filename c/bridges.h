#include "config.h"

#ifndef _BRIDGES_H
#define _BRIDGES_H

#include <argp.h>
#include <termios.h>
#include <unistd.h>

#include <sys/epoll.h>

#include <lcm/lcm.h>
#include "raw_bytes_t.h"

#include "r2_epoch.h"

#define INPUT_SUFFIX "i"
#define OUTPUT_SUFFIX "o"

extern char **environ;

const char *argp_program_version = PACKAGE_STRING;

const char *argp_program_bug_address = PACKAGE_BUGREPORT;

static speed_t char_to_baudrate( char *arg ) {
    switch( atoi( arg ) ) {
        case 9600:
            return B9600;
            break;
        case 19200:
            return B19200;
            break;
        case 38400:
            return B38400;
            break;
        case 57600:
            return B57600;
            break;
        case 115200:
            return B115200;
            break;
        case 230400:
            return B230400;
            break;
        default:
            printf( "%s baud not supported\n", arg );
            exit( EXIT_FAILURE );
    }
}

static void raw_handler( const lcm_recv_buf_t *rbuf, const char * channel,
        const raw_bytes_t * msg, void * user ) {
    write( *( (int *) user ), msg->data, msg->length );
}

#endif // _BRIDGES_H
