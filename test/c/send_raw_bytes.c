#include <stdio.h>

#include <lcm/lcm.h>

#include "raw_bytes_t.h"

int main( int argc, char* argv[] ){

    lcm_t * lio = lcm_create( NULL );
    if( !lio ) {
        fputs( "could not create LCM instance", stderr );
        exit( EXIT_FAILURE );
    }

    raw_bytes_t msg = {
        .utime = -1,
        .length = 5,
        .data = (uint8_t *)"hello",
    };
    raw_bytes_t_publish( lio, argv[0], &msg );

    for( int a = 2; a < argc; a++ ) {
        msg.utime = a;
        msg.length = strlen( argv[a] );
        msg.data = (uint8_t *)argv[a];
        raw_bytes_t_publish( lio, argv[1], &msg );
        printf( "sent %d-byte message:", msg.length );
        for( int b = 0; b < msg.length; b++ ) {
            printf( " %02hhX", msg.data[b] );
        }
        printf( "\n" );
    }

    lcm_destroy( lio );
    exit( EXIT_SUCCESS );
}
