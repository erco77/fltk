#ifndef StyleParseInfo_h
#define StyleParseInfo_h

// Class to manage style parsing, friend of CodeEditor
class StyleParseInfo {
  public:
    const char *tbuff;        // text buffer
    char       *sbuff;        // style buffer
    int         len;          // running length
    char        style;        // current style
    char        lwhite;       // leading white space (1=white, 0=past white)
    int         col;          // line's column counter
    char        keyword[40];  // keyword parsing buffer
    char        last;         // flag for keyword parsing

    int  parse_over(int handle_crlf=1);
    void parse_block_comment();
    void buffer_keyword();
    void parse_keyword();
    int  parse_quoted_string();
};

#endif //StyleParseInfo_h
