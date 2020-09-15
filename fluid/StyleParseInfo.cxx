#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "StyleParseInfo.h"
#include "CodeEditor.h"

// Handle style parsing over a character
//    Handles updating col counter when \n encountered.
//    Applies the current style, advances to next text + style char.
//    Returns 0 if hit end of buffer, 1 otherwise.
//
int StyleParseInfo::parse_over(int handle_crlf) {
  char c = *tbuff;

  // End of line?
  if ( handle_crlf ) {
    if ( c == '\n' ) {
      lwhite = 1;           // restart leading white flag
      // Finish line comment or #directive
      if ( style == 'B' || style == 'E' ) style = 'A';
    } else {
      // End of leading white?
      if ( c != ' ' && c != '\t' ) lwhite = 0;
    }
  }

  // Adjust and advance
  //    If handling crlfs, zero col on crlf. If not handling, let col continue to count past crlf
  //    e.g. for multiline #define's that have lines ending in backslashes.
  //
  col = (c=='\n') ? (handle_crlf ? 0 : col) : col+1;   // column counter
  tbuff++;                    // advance text ptr
  *sbuff++ = style;           // apply style & advance its ptr
  if ( --len <= 0 ) return 0; // keep track of length
  return 1;
}

// Parse a block comment until end of comment or buffer.
void StyleParseInfo::parse_block_comment() {
  style = 'C';               // block comment mode
  while ( len > 0 ) {
    char c  = tbuff[0];      // current char
    char c2 = (len > 0) ? tbuff[1] : 0; // next char
    if ( c == '*' && c2 == '/' ) {
      parse_over();          // handle '*'
      parse_over();          // handle '/'
      style = 'A';           // return to normal style
      return;
    }
    if ( !parse_over() ) return;
  }
}

// Copy keyword -> keyword buffer
void StyleParseInfo::buffer_keyword() {
  char *key  = keyword;
  char *kend = key + sizeof(keyword) - 1; // end of buffer
  for ( const char *s=tbuff; (islower(*s) || *s=='_') && (key < kend); *key++ = *s++ ) { }
  *key = 0;     // terminate
}

// Parse line for possible keyword
//    Returns with spi.keyword[] containing word.
//
void StyleParseInfo::parse_keyword() {
  // Parse into 'keyword' buffer
  buffer_keyword();

  // C/C++ type? (void, char..)
  char *key = keyword;
  if ( CodeEditor::search_types(key) ) {
    char save = style;
    style = 'F';
    while ( *key++ && parse_over() ) { }
    last  = 1;
    style = save;
    return;
  } else if ( CodeEditor::search_keywords(key) ) {
    // C/C++ Keyword? (switch, return..)
    char save = style;
    style = 'G';
    while ( *key++ && parse_over() ) { }
    last  = 1;
    style = save;
    return;
  }
  // Not a type or keyword? Parse over it
  while ( *key++ && parse_over() ) { }
}

// Style parse a quoted string.
//    Returns 0 if hit end of buffer, 1 otherwise.
//
int StyleParseInfo::parse_quoted_string() {
  style = 'D';                           // enter string mode
  if ( !parse_over() ) return 0;         // parse over opening quote

  // Parse until closing quote reached
  char c;
  while ( len > 0 ) {
    c = tbuff[0];
    if ( c == '"' ) {
      // Closing quote? Parse and done
      return parse_over();
    } else if ( c == '\\' ) {
      // Escape sequence? Parse over, continue
      if ( !parse_over() ) return 0;     // escape
      if ( !parse_over() ) return 0;     // char being escaped
      continue;
    }
    // Keep parsing until end of buffer or closing quote..
    if ( !parse_over() ) return 0;
  }
  return 1;
}


