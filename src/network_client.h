#ifndef NETWORK_CLIENT_H
#define NETWORK_CLIENT_H

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/container/vector.hpp>
#include <boost/array.hpp>
#include <boost/atomic.hpp>

#define MAX_RECV_BUFF_LEN	8092

class network_client;

class iprotocol {
public:
    virtual void handle_read(char* data, size_t size) = 0;
};

typedef iprotocol* iprotocol_pt;

typedef boost::asio::ip::tcp::endpoint asio_tcp_endpoint_t;

typedef boost::asio::io_service asio_io_service_t;
typedef boost::shared_ptr<asio_io_service_t> io_service_pt;
typedef boost::asio::ip::tcp::socket asio_socket_t;
typedef boost::shared_ptr<asio_socket_t> socket_pt;
typedef boost::asio::io_service::strand asio_strand_t;
typedef boost::shared_ptr<asio_strand_t> strand_pt;
typedef boost::asio::ssl::stream<asio_socket_t> asio_ssl_socket_t;
typedef boost::shared_ptr<asio_ssl_socket_t> ssl_socket_pt;
typedef boost::asio::ssl::context asio_ssl_context_t;


class network_client {
    
public:
    network_client(iprotocol_pt protocol, bool use_ssl = false);
    virtual ~network_client() {}
    
    bool connect(const char* ip, int port);
    bool wait_connected(int timeout_millisec);
    void close();
    bool write(char* data, size_t size);

    bool is_connected();
    
    const char* last_error_message();
    int last_error_code();
    
private:
    
    bool make_certificate(boost::container::vector<unsigned char>& cert_buff, boost::container::vector<unsigned char>& key_buff);
    void post_read();
    
    void handle_connect(const boost::system::error_code& error);
    void handle_handshake(const boost::system::error_code& error);
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
    void handle_write(const boost::system::error_code& error, size_t bytes_transferred);
    
private:
    strand_pt strand_;
    io_service_pt io_service_;
    ssl_socket_pt ssl_socket_;
    iprotocol_pt protocol_;
    
    boost::atomic<bool> connected_;
    boost::array<char, MAX_RECV_BUFF_LEN> receive_buffer_;
    
    boost::thread t_;
    boost::system::error_code last_error_;
    
    bool use_ssl_;
};

#endif //NETWORK_CLIENT_H
