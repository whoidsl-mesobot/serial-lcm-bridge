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
        .length = 6,
        .data = (uint8_t *)"hello\n",
    };

    const char * channel = ( argc > 1 ) ? argv[1] : "test";

    raw_bytes_t_publish( lio, channel, &msg );

    for( int a = 3; a < argc; a++ ) {
        msg.utime = a;
        msg.length = strlen( argv[a] );
        msg.data = (uint8_t *)argv[a];
        raw_bytes_t_publish( lio, channel, &msg );
        printf( "sent %d-byte message:", msg.length );
        for( int b = 0; b < msg.length; b++ ) {
            printf( " %02hhX", msg.data[b] );
        }
        printf( "\n" );
    }

    lcm_destroy( lio );
    exit( EXIT_SUCCESS );
}
