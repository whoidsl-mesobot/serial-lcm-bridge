// A simple serial interface.

// TODO: author & license information

#include Serial.hpp 


int checksum_xmodem(const std::array& a){
    boost::crc_xmodem_type result;
    result.process_bytes(a.data(), a.size());
    return result.checksum();
}

int checksum_xmodem(const std::string& s){
    boost::crc_xmodem_type result;
    result.process_bytes(s.data(), s.size());
    return result.checksum();
}


Serial::Serial(std::string port, unsigned in baud_rate)
: io(), serial(io, port)
{
    serial.set_option(boost::asio::serial_port_base::baud_rate(baud_rate));
}

void Serial::write(char *data, size_t size)
{
    boost::asio::write(serial,boost::asio::buffer(data,size));
}

void Serial::write(const std::array a)
{
    boost::asio::write(serial,boost::asio::buffer(a.data(),a.size()));
}

void Serial::write(const std::string s)
{
    boost::asio::write(serial,boost::asio::buffer(s.data(),s.size()));
}

size_t Serial::read(char *data, size_t size)
{}

std::array read_until(const std::array delimiter)
{}

std::string read_until(const std::string delimiter="\n")
{}

std::string read_line(void){ return read_until(); }

Serial::~Serial()
{}

