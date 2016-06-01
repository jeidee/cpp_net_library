#include "network_client.h"
#include "openssl_cert.h"

network_client::network_client(iprotocol_pt protocol, bool use_ssl)
: strand_(nullptr)
, ssl_socket_(nullptr)
, protocol_(protocol)
, connected_(false)
, use_ssl_(use_ssl) {
}

bool network_client::connect(const char* ip, int port) {
    if (connected_) {
        last_error_ = boost::system::errc::make_error_code(boost::system::errc::already_connected);
        return false;
    }
    
    if (io_service_ == nullptr) {
        io_service_ = io_service_pt(new asio_io_service_t());
    }
    
    if (strand_ == nullptr) {
        strand_ = strand_pt(new asio_strand_t(*io_service_));
    }
    
    
    boost::container::vector<unsigned char> cert_buff, key_buff;
    if (!make_certificate(cert_buff, key_buff)) {
        // @todo: make custom errors
        last_error_ = boost::system::errc::make_error_code(boost::system::errc::not_supported);
        return false;
    }
    
    asio_ssl_context_t ssl_context(*io_service_, asio_ssl_context_t::tlsv1);
    ssl_context.set_verify_mode(asio_ssl_context_t::verify_none);
    
    ssl_context.use_certificate(boost::asio::const_buffer(cert_buff.data(), cert_buff.size()), asio_ssl_context_t::asn1);
    ssl_context.use_private_key(boost::asio::const_buffer(key_buff.data(), key_buff.size()), asio_ssl_context_t::asn1);
    
    ssl_socket_ = ssl_socket_pt(new asio_ssl_socket_t(*io_service_, ssl_context));
    
    
    auto endpoint = boost::asio::ip::tcp::endpoint(
                                                   boost::asio::ip::address::from_string(ip), port);
    
    ssl_socket_->next_layer().async_connect(endpoint, strand_->wrap(boost::bind(&network_client::handle_connect, this, boost::asio::placeholders::error)));
    
    t_ = boost::thread(boost::bind(static_cast<size_t (boost::asio::io_service::*)()>(&boost::asio::io_service::run), io_service_));
   
    return true;
}

bool network_client::wait_connected(int timeout_millisec) {
    int elapsed_millisec = 0, period = 100;
    
    while(!connected_) {
        boost::this_thread::sleep(boost::posix_time::milliseconds(period));
        elapsed_millisec += period;
        
        if (elapsed_millisec >= timeout_millisec) {
            last_error_ = boost::system::errc::make_error_code(boost::system::errc::timed_out);
            return connected_;
        }
    }
    
    return connected_;
}

void network_client::close() {
    ssl_socket_->get_io_service().stop();
    ssl_socket_->lowest_layer().close();
    
    connected_ = false;
}

bool network_client::write(char* data, size_t size) {
    if (!connected_ || !ssl_socket_->lowest_layer().is_open()) {
        last_error_ = boost::system::errc::make_error_code(boost::system::errc::not_connected);
        return false;
    }
    
    if (use_ssl_) {
        boost::asio::async_write(
                                 *ssl_socket_,
                                 boost::asio::buffer(data, size),
                                 strand_->wrap(boost::bind(&network_client::handle_write,
                                                           this,
                                                           boost::asio::placeholders::error,
                                                           boost::asio::placeholders::bytes_transferred)));
    } else {
        boost::asio::async_write(
                                 ssl_socket_->next_layer(),
                                 boost::asio::buffer(data, size),
                                 strand_->wrap(boost::bind(&network_client::handle_write,
                                                           this,
                                                           boost::asio::placeholders::error,
                                                           boost::asio::placeholders::bytes_transferred)));
    }
    
    return true;
}


bool network_client::is_connected() {
    return connected_;
}

const char* network_client::last_error_message() {
    if (last_error_.value() == 0) {
        return "";
    } else {
        return last_error_.message().c_str();
    }
}

int network_client::last_error_code() {
    return last_error_.value();
}

bool network_client::make_certificate(boost::container::vector<unsigned char>& cert_buff, boost::container::vector<unsigned char>& key_buff) {
    openssl_cert cert;
    return cert.make(cert_buff, key_buff);
}

void network_client::handle_connect(const boost::system::error_code& error) {
    if (error) {
        last_error_ = error;
        connected_ = false;
        close();
        return;
    }
    
   
    if (use_ssl_) {
        ssl_socket_->async_handshake(boost::asio::ssl::stream_base::client,
                                     boost::bind(&network_client::handle_handshake, this, boost::asio::placeholders::error));
    } else {
        boost::asio::ip::tcp::no_delay no_delay(true);
        boost::asio::socket_base::non_blocking_io non_blocking_io(true);
        ssl_socket_->lowest_layer().io_control(non_blocking_io);
        ssl_socket_->lowest_layer().set_option(no_delay);
        
        connected_ = true;
        
        post_read();
    }
}

void network_client::handle_handshake(const boost::system::error_code& error) {
    if (error) {
        last_error_ = error;
        connected_ = false;
        return;
    }
    
    boost::asio::ip::tcp::no_delay no_delay(true);
    boost::asio::socket_base::non_blocking_io non_blocking_io(true);
    ssl_socket_->lowest_layer().io_control(non_blocking_io);
    ssl_socket_->lowest_layer().set_option(no_delay);
    
    connected_ = true;
    
    post_read();
}

void network_client::post_read() {
    if (!ssl_socket_->lowest_layer().is_open()) {
        return;
    }
    
    receive_buffer_.assign(0);
    if (use_ssl_) {
        boost::asio::async_read(*ssl_socket_,
                                boost::asio::buffer(receive_buffer_),
                                strand_->wrap(boost::bind(
                                                          &network_client::handle_read,
                                                          this,
                                                          boost::asio::placeholders::error,
                                                          boost::asio::placeholders::bytes_transferred)));
    } else {
//        boost::asio::async_read(ssl_socket_->next_layer(),
//                                boost::asio::buffer(receive_buffer_),
//                                strand_->wrap(boost::bind(
//                                                          &network_client::handle_read,
//                                                          this,
//                                                          boost::asio::placeholders::error,
//                                                          boost::asio::placeholders::bytes_transferred)));
        ssl_socket_->next_layer().async_read_some(boost::asio::buffer(receive_buffer_),
                                strand_->wrap(boost::bind(
                                                          &network_client::handle_read,
                                                          this,
                                                          boost::asio::placeholders::error,
                                                          boost::asio::placeholders::bytes_transferred)));
    }
    
}

void network_client::handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
    if (error) {
        last_error_ = error;
        printf("%s", error.message().c_str());
        close();
        return;
    }
    
    if (bytes_transferred == 0) {
        last_error_ = boost::system::errc::make_error_code(boost::system::errc::connection_refused);
        close();
        return;
    }
    
    protocol_->handle_read(receive_buffer_.c_array(), bytes_transferred);
    
    
    post_read();
}

void network_client::handle_write(const boost::system::error_code& error, size_t bytes_transferred) {
    if (error) {
        last_error_ = error;
        printf("error raised %s\n", error.message().c_str());
        return;
    }
}
