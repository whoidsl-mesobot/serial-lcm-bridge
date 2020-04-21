#ifndef _SIMPLE_H
#define _SIMPLE_H

#define MAX_LENGTH 255
#define SLEEP_MICROSECONDS 10

static char doc[] = "simple-serial-lcm-bridge -- a bridge between serial device and LCM";
static char args_doc[] = "device";

static struct argp_option options[] = {
    { "verbose", 'v', 0, 0, "say more" },
    { "quiet", 'q', 0, 0, "say less" },
    { "baudrate", 'b', "baudrate", 0, "baudrate" },
    { 0 }
};

struct arguments {
    char * dev;
    int8_t verbosity;
    speed_t baudrate;
};

static error_t parse_opt( int key, char *arg, struct argp_state *state ) {
    struct arguments *args = state->input;
    switch( key ){
        case 'q':
            args->verbosity = -1;
            break;
        case 'v':
            args->verbosity += 1;
            break;
        case 'b':
            args->baudrate = char_to_baudrate( arg );
            break;
        case ARGP_KEY_ARG:
            if( state->arg_num >= 1 ) argp_usage( state );
            args->dev = arg;
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

struct arguments args;

struct termios tio = {
        .c_cflag = CS8 | CLOCAL | CREAD | B9600,
        .c_iflag = IGNBRK,
        .c_oflag = 0,
        .c_lflag = 0,
        .c_cc[VMIN] = 0,
        .c_cc[VTIME] = 1,
};

#endif // _SIMPLE_H
