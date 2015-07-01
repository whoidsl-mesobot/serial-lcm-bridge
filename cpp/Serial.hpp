// Serial.h
//
// A simple serial interface.
//
// TODO: author & license information

#ifndef SERIAL_H_
#define SERIAL_H_

#include <array>          // for std::array
#include <string>         // for std::string

#include <boost/asio.hpp> // for boost::asio
#include <boost/crc.hpp>  // for boost::crc_xmodem_type


// convenience wrappers for XMODEM checksum
//
// see http://stackoverflow.com/questions/2573726/how-to-use-boostcrc
//
int checksum_xmodem(const std::array& a);
int checksum_xmodem(const std::string& s);

// simple serial uart interface based on boost::asio
//
// see http://www.webalice.it/fede.tft/serial_port/serial_port.html
//
class Serial{
public;
    Serial(std::string port, unsigned in baud_rate);

    void write(char *data, size_t size);

    void write(const std::array& a);

    void write(const std::string& s);

    size_t read(char *data, size_t size);

    std::array read_until(const std::array delimiter);

    std::string read_until(const std::string delimiter="\n");

    std::string read_line(void);

    ~Serial();
}

#endif // SERIAL_H_
