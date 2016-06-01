#include "xml_parser.h"
#include <iostream>

using namespace std;

xml_parser::xml_parser()
    : parser_(nullptr)
    , depth_(0) {
}

xml_parser::~xml_parser() {
    deinit();
}

bool xml_parser::init(const XML_Char *encoding, const XML_Char *namespace_separator) {
    deinit();
    
    parser_ = XML_ParserCreate_MM(encoding, nullptr, namespace_separator);
    if (parser_ == nullptr) {
        return false;
    }
    
    XML_SetUserData(parser_, (void *)this);
    XML_SetStartElementHandler(parser_, start_element);
    XML_SetEndElementHandler(parser_, end_element);
    XML_SetCharacterDataHandler(parser_, visit_data);
    
    return true;
}

void xml_parser::deinit() {
    if (parser_ != nullptr) {
        XML_SetStartElementHandler(parser_, nullptr);
        XML_SetEndElementHandler(parser_, nullptr);
        XML_SetCharacterDataHandler(parser_, nullptr);

        XML_ParserFree(parser_);
        parser_ = nullptr;
    }
}

bool xml_parser::parse(const char *buffer, int length, bool is_final) {
    if (parser_ == nullptr) {
        return false;
    }
    
    if (length == 0) {
        return true;
    }
    
    if (length < 0) {
        length = (int)strlen(buffer);
    }
    
    void *buff = (char*)XML_GetBuffer(parser_, 1024);
    memcpy(buff, buffer, length);
    
    auto ret = XML_ParseBuffer(parser_, length, is_final);
    if (ret != XML_STATUS_OK) {
        auto error_code = XML_GetErrorCode(parser_);
        
        cout << "XML_Parse error. Error code is " << error_code << ", " << XML_ErrorString(error_code) << endl;
        return false;
    }
    
    return true;
}

void xml_parser::on_start_element(const XML_Char *name, const XML_Char **attrs) {
    depth_++;

    const XML_Char *attr_name = nullptr;
    const XML_Char *attr_value = nullptr;
    
    cout << "on_start_element : " << name << ", depth : " << depth_ << endl;
    
    for (int i = 0; attrs[i] != nullptr; i += 2) {
        attr_name = attrs[i];
        attr_value = attrs[i + 1];
        
        cout << "name : " << attr_name << ", value : " << attr_value << endl;
    }

}

void xml_parser::on_visit_data(const XML_Char *s, int len) {
    XML_Char value[1024] = {0,};
    strncpy_s(value, s, len);
    
    cout << "on_visit_data : " << value << ", len : " << len << endl;

}

void xml_parser::on_end_element(const XML_Char *name) {
    depth_--;
    
    cout << "on_end_element : " << name << ", depth : " << depth_ << endl;
}


void xml_parser::start_element(void *data, const XML_Char *name, const XML_Char **attrs) {
    auto parser = (xml_parser*)data;
    
    parser->on_start_element(name, attrs);
}

void xml_parser::visit_data(void *data, const XML_Char *s, int len) {
    auto parser = (xml_parser*)data;
    
    parser->on_visit_data(s, len);

}

void xml_parser::end_element(void *data, const XML_Char *name) {
    auto parser = (xml_parser*)data;
    
    parser->on_end_element(name);

}
