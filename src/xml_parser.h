#ifndef __XML_PARSER_H__
#define __XML_PARSER_H__


#define XML_STATIC

#include <expat.h>

class xml_parser {
public:
    xml_parser();
    virtual ~xml_parser();
    
    bool init(const XML_Char *encoding = nullptr, const XML_Char *namespace_separator = nullptr);
    void deinit();
    
    bool parse(const char *buffer, int length = -1, bool is_final = true);
    
    
public:
    virtual void on_start_element(const XML_Char *name, const XML_Char **attrs);
    virtual void on_visit_data(const XML_Char *s, int len);
    virtual void on_end_element(const XML_Char *name);

private:
    static void XMLCALL start_element(void *data, const XML_Char *name, const XML_Char **attrs);
    static void XMLCALL visit_data(void *data, const XML_Char *s, int len);
    static void XMLCALL end_element(void *data, const XML_Char *name);

protected:
    int depth_;

private:
    XML_Parser parser_;
};


#endif // __XML_PARSER_H__
