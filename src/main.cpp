#include "network_client.h"
#include <stdio.h>

#define INIT_QUEUE_SIZE		MAX_RECV_BUFF_LEN * 10

class line_packet{
public:
    std::string messages;
};

class line_protocol : public iprotocol {
    typedef line_packet* packet_type;
public:
    line_protocol() : packet_queue_(1)
    , buffer_("") {
    }
    
    virtual void handle_read(char* data, size_t size) {
        slice_data(data, size);
    }
    
    packet_type pop_packet() {
        packet_type packet;
        if (packet_queue_.pop(packet))
            return packet;
        else
            return nullptr;
    }
    
private:
    void slice_data(char* data, size_t size) {
        buffer_ += std::string(data, size);
        
        size_t i = 0, previ = 0;
        for (i = 0, previ = 0; i < buffer_.size(); ++i) {
            if (buffer_.at(i) == '\n') {
                auto packet = new line_packet();
                packet->messages =buffer_.substr(previ, (i - previ));
                packet_queue_.push(packet);
                previ = i + 1;
            }
        }
        
        if (previ < i) {
            buffer_ = buffer_.substr(previ, (i - previ));
        } else {
            buffer_ = "";
        }
    }
    
private:
    
    std::string buffer_;
    boost::lockfree::queue<packet_type> packet_queue_;
};

int main() {
    boost::shared_ptr<line_protocol> protocol(new line_protocol());
    
    network_client nc(protocol);
    
    std::string ip("10.5.3.165");
    int port(12345);
    
    if (!nc.connect(ip, port)) {
        printf("Connection failed!\nLast Error: %d, %s\n", nc.last_error_code(), nc.last_error_message());
        getchar();
        return -1;
    }
    
    if (!nc.wait_connected(10000)) {
        printf("Connection timeout!\nLast Error: %d, %s\n", nc.last_error_code(), nc.last_error_message());
        getchar();
        return -1;
    }
    
    boost::thread t([&](){
        int i = 0;
        while(true) {
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
            auto packet = protocol->pop_packet();
            if (packet != nullptr) {
                printf("--------------------------------------------\n");
                printf("Received %d - %s\n", ++i, packet->messages.c_str());
                printf("--------------------------------------------\n");
                delete packet;
            }
        }
    });
    
    
    for (int i = 0; i < 1000; ++i) {
        char *data = new char[1024];
        
        snprintf(data, 1024, "hello~world[%d]\n", i + 1);
        
        nc.write(data, strlen(data));
        
        delete[] data;
    }
    
    
    t.join();
    
    nc.close();
    
}
