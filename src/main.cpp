#include "network_client.h"
#include "xml_parser.h"
#include "auto_lock.h"
#include "concurrent_stack.h"

#include <stdio.h>
#include <boost/container/vector.hpp>
#include <boost/unordered_map.hpp>
#include <boost/foreach.hpp>
#include <boost/algorithm/co>

using namespace std;

class stanza;

typedef boost::shared_ptr<stanza> stanza_pt;
typedef boost::container::vector<stanza_pt> stanza_children_t;
typedef boost::unordered_map<std::string, std::string> stanza_attrs_t;

class stanza {
public:
    const char* name() {
        return name_.c_str();
    }

	static stanza_pt make_stanza() {
		return boost::make_shared<stanza>();
	}
    
    void set_name(const char* name) {
        name_ = name;
    }
    
    const char* value() {
        return value_.c_str();
    }
    
    void set_value(const char* value) {
        value_ = value;
    }
    
    void add_child(stanza_pt stanza) {
        children_.push_back(stanza);
    }
    
    void add_attr(const char* name, const char* value) {
        attrs_[name] = value;
    }
    
    const char* get_attr(const char* name) {
        if (attrs_.find(name) == attrs_.end()) {
            return nullptr;
        }
        return attrs_[name].c_str();
    }

	bool has_child(const char *child_name, const char *ns) {
		if (child_name == nullptr) {
			return false;
		}

		BOOST_FOREACH(auto s, children_) {
			if (strcmp(s->name(), child_name) == 0 &&
				strcmp(s->get_ns(), ns) == 0) {
				return true;
			}
		}

		return false;
	}

	void get_children(stanza_children_t results, const char *child_name) {

	}

	const char* get_ns() {
		return get_attr("xmlns");
	}

    
private:
    std::string name_;
    std::string value_;

	stanza_children_t children_;
    stanza_attrs_t attrs_;
};

class xmpp_handler {
public:
	virtual void on_sasl(stanza_pt s) = 0;
	virtual void on_message(stanza_pt s) = 0;
	virtual void on_iq(stanza_pt s) = 0;
	virtual void on_presence(stanza_pt s) = 0;
	virtual void on_unhandled_stanza(stanza_pt s) = 0;
};

class xmpp_protocol
    : public iprotocol
    , public xml_parser {
        
public:
	const char* NS_XMPP_SASL = "urn:ietf:params:xml:ns:xmpp-sasl";

    xmpp_protocol() : idx_(0), xmpp_handler_(nullptr) {
        mtx_ = boost::make_shared<boost::mutex>();
        init();
    }

	void set_xmpp_handler(xmpp_handler* handler) {
		xmpp_handler_ = handler;
	}
    
    virtual ~xmpp_protocol() {
        deinit();
    }
   
    virtual void handle_read(char* data, size_t size) {
        auto_lock lock(mtx_);
        this->parse(data, (int)size, false);
    }
    
    virtual void on_start_element(const XML_Char *name, const XML_Char **attrs) {
        depth_++;
        
		auto s = stanza::make_stanza();
        s->set_name(name);
        
        stanza_stack_.push(s);
        
        const XML_Char *attr_name = nullptr;
        const XML_Char *attr_value = nullptr;
        
        cout << "on_start_element : " << name << ", depth : " << depth_ << endl;
        
        for (int i = 0; attrs[i] != nullptr; i += 2) {
            attr_name = attrs[i];
            attr_value = attrs[i + 1];
            s->add_attr(attr_name, attr_value);
            
            cout << "name : " << attr_name << ", value : " << attr_value << endl;
        }
        
    }
    
	virtual void on_visit_data(const XML_Char *data, int len) {
		auto s = stanza_stack_.pop();

		if (s == nullptr) {
			printf("on_visit_data not exists stanza\n");
			return;
		}

		s->set_value(std::string(data, len).c_str());

		stanza_stack_.push(s);

		cout << "on_visit_data : " << s->name() << ", " << s->value() << ", len : " << len << endl;

	}

    virtual void on_end_element(const XML_Char *name) {
        depth_--;
        
        auto s = stanza_stack_.pop();
        
        if (s == nullptr) {
            printf("on_end_element not exists stanza\n");
            return;
        }
        
        if (std::string(s->name()) != std::string(name)) {
            printf("on_end_element not exists matched stanza, %s : %s \n", s->name(), name);
            return;
        }
        
        cout << "on_end_element : " << name << ", depth : " << depth_ << endl;
        
		if (depth_ > 1) {
			auto parent = stanza_stack_.pop();
			parent->add_child(s);
			stanza_stack_.push(parent);
		} else if (depth_ == 1) {
			handle_stanza(s);
		}
    }

private:
	void handle_stanza(stanza_pt s) {
		if (xmpp_handler_ == nullptr) {
			return;
		}

		if (s == nullptr || s->name() == nullptr) {
			return;
		}

		std::string name = s->name();

		if (name == "stream:features" && s->has_child("mechanisms", NS_XMPP_SASL)) {
			xmpp_handler_->on_sasl(s);
		}
		else {
			xmpp_handler_->on_unhandled_stanza(s);
		}
	}
    
private:
    int idx_;
    boost::shared_ptr<boost::mutex> mtx_;
    concurrent_stack<stanza_pt> stanza_stack_;
	xmpp_handler* xmpp_handler_;
};

class xmpp_client : public xmpp_handler {
public:
    xmpp_client()
    : protocol_(nullptr)
    , nc_(nullptr) {
    }
    
    virtual ~xmpp_client() {
    }

	virtual void on_sasl(stanza_pt s) {
		printf("on_sasl...\n");
	}

	virtual void on_message(stanza_pt s) {
		printf("on_message...\n");
	}

	virtual void on_iq(stanza_pt s) {
		printf("on_iq...\n");
	}

	virtual void on_presence(stanza_pt s) {
		printf("on_presence...\n");
	}

	virtual void on_unhandled_stanza(stanza_pt s) {
		printf("on_unhandled_stanza...%s\n", s->name());
	}
    
    bool start(const char* ip, int port) {
        if (protocol_ == nullptr) {
            protocol_ = boost::make_shared<xmpp_protocol>();
			protocol_->set_xmpp_handler(this);
        }
        
        if (nc_ == nullptr) {
            nc_ = boost::make_shared<network_client>(protocol_.get());
        }
        
        if (!nc_->connect(ip, port)) {
            printf("Connection failed!\nLast Error: %d, %s\n", nc_->last_error_code(), nc_->last_error_message());
            return false;
        }
        
        if (!nc_->wait_connected(10000)) {
            printf("Connection timeout!\nLast Error: %d, %s\n", nc_->last_error_code(), nc_->last_error_message());
            return false;
        }
        
        return true;
    }
    
    void stop() {
        nc_->close();
    }
    
    bool login(const char* id, const char* host, const char* password) {
        id_ = id;
        host_ = host;
        password_ = password;
        
        std::string data = "<?xml version=\"1.0\"?><stream:stream xmlns=\"jabber:client\" xmlns:stream=\"http://etherx.jabber.org/streams\" to=\"" + std::string(host) + "\" version=\"1.0\">\n";
        
        nc_->write(const_cast<char*>(data.c_str()), data.size());
        
        return true;
    }
    
    const char* jid() {
        return (std::string(id_) + "@" + std::string(host_)).c_str();
    }

private:
    boost::shared_ptr<network_client> nc_;
    boost::shared_ptr<xmpp_protocol> protocol_;
    
    std::string id_;
    std::string host_;
    std::string password_;
    
    int status_;
};


int main() {
    
    xmpp_client nc;
    
    if (!nc.start("10.5.3.165", 5222)) {
        printf("start failed.\n");
        return -1;
    }
    
    if (!nc.login("test", "bypass", "1234")) {
        printf("login failed.");
        return -1;
        
    }

    
    getchar();
    
    nc.stop();
    
    return 0;
    
}
