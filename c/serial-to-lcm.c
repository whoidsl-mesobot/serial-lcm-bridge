//  Bidirectional bridge between a UART (serial port) and LCM

//  TODO: author & license information

// Adapted from listener_async.c example from LCM 
// and from "Waiting for Input from Multiple Sources" in the tldp serial HOWTO

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static void sio_handler()
{
// TODO: fill buffer from serial
    if(s[strlen(s)] == "\n") publish_serial_line(s, lcm, channel);
}

static void publish_serial_line(const char * buffer, const lcm_t * lcm, const char * channel)
{
    raw_bytes_t msg;
    msg.size = strlen(s)
    for(int i = 0; i < msg.size; i++) msg.raw[i] = s[i];
    raw_bytes_t_publish(lcm, channel, &msg);
}

static void raw_handler(const lcm_recv_buf_t *rbuf, const char * channel, const raw_bytes_t * msg, void * sio)
{
    for(int i=0; i < msg->size; i++) sio.write(msg->raw[i]);
}

int main(int argc, char ** argv)
{
    int sio_fd; // file descriptor for serial
    int lcm_fd; // file descriptor for lcm
    fd_set fds; // file descriptor set   

    lcm_t * lcm;

    char port_name[] = '/dev/ttyS0'

    // TODO: open an actual serial port interface...

    lcm = lcm_create(NULL);
    if( !lcm ) return 1;

    sio_fd = open_input_source(port_name);
    lcm_fd = lcm_get_fileno(lcm); // TODO: check whetehr this needs to be within the while loop (i.e., does LCM change the file descriptor for each transaction?)
    maxfd = MAX (sio_fd, lcm_fd) + 1;
    
    raw_bytes_t_subscription_t * sub = raw_bytes_t_subscribe(lcm, sio->port, &raw_handler, sio);

    while(1){
        FD_ZERO(&fds);
        FD_SET(sio_fd, &fds);
        FD_SET(lcm_fd, &fds);
        // block until input is available
        select(maxfd, &readfs, NULL, NULL, NULL);
        if(FD_ISSET(sio_fd, &fds)) handle_serial_data(); // take care of data on the serial line first
        if(FD_ISSET(lcm_fd, &fds)) lcm_handle(lcm); // then take care of data on LCM
    }
    
    raw_bytes_t_subscribe(lcm, sub);
    lcm_destroy(lcm);
    return 0;
}
