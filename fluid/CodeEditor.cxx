//
// "$Id$"
//
// Code editor widget for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2016 by Bill Spitzak and others.
//
// This library is free software. Distribution and use rights are outlined in
// the file "COPYING" which should have been included with this file.  If this
// file is missing or damaged, see the license at:
//
//     http://www.fltk.org/COPYING.php
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

//
// Include necessary headers...
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "CodeEditor.h"

Fl_Text_Display::Style_Table_Entry CodeEditor::
		styletable[] = {	// Style table
		  { FL_FOREGROUND_COLOR, FL_COURIER,        11 }, // A - Plain
		  { FL_DARK_GREEN,       FL_COURIER_ITALIC, 11 }, // B - Line comments
		  { FL_DARK_GREEN,       FL_COURIER_ITALIC, 11 }, // C - Block comments
		  { FL_BLUE,             FL_COURIER,        11 }, // D - Strings
		  { FL_DARK_RED,         FL_COURIER,        11 }, // E - Directives
		  { FL_DARK_RED,         FL_COURIER_BOLD,   11 }, // F - Types
		  { FL_BLUE,             FL_COURIER_BOLD,   11 }  // G - Keywords
		};

const char * const CodeEditor::
		code_keywords[] = {	// Sorted list of C/C++ keywords...
		  "and",
		  "and_eq",
		  "asm",
		  "bitand",
		  "bitor",
		  "break",
		  "case",
		  "catch",
		  "compl",
		  "continue",
		  "default",
		  "delete",
		  "do",
		  "else",
		  "false",
		  "for",
		  "goto",
		  "if",
		  "new",
		  "not",
		  "not_eq",
		  "operator",
		  "or",
		  "or_eq",
		  "return",
		  "switch",
		  "template",
		  "this",
		  "throw",
		  "true",
		  "try",
		  "while",
		  "xor",
		  "xor_eq"
		};

const char * const CodeEditor::
		code_types[] = {	// Sorted list of C/C++ types...
		  "auto",
		  "bool",
		  "char",
		  "class",
		  "const",
		  "const_cast",
		  "double",
		  "dynamic_cast",
		  "enum",
		  "explicit",
		  "extern",
		  "float",
		  "friend",
		  "inline",
		  "int",
		  "long",
		  "mutable",
		  "namespace",
		  "private",
		  "protected",
		  "public",
		  "register",
		  "short",
		  "signed",
		  "sizeof",
		  "static",
		  "static_cast",
		  "struct",
		  "template",
		  "typedef",
		  "typename",
		  "union",
		  "unsigned",
		  "virtual",
		  "void",
		  "volatile"
		};

// attempt to make the fluid code editor widget honour textsize setting
void CodeEditor::textsize(Fl_Fontsize s) {
  Fl_Text_Editor::textsize(s); // call base class method
  // now attempt to update our styletable to honour the new size...
  int entries = sizeof(styletable) / sizeof(styletable[0]);
  for(int iter = 0; iter < entries; iter++) {
    styletable[iter].size = s;
  }
} // textsize


// 'compare_keywords()' - Compare two keywords...
extern "C" {
  static int compare_keywords(const void *a, const void *b) {
    return strcmp(*((const char **)a), *((const char **)b));
  }
}

// See if 'find' is a C/C++ keyword.
//     Refer to bsearch(3) for return value.
//
void* CodeEditor::search_keywords(char *find) {
  return bsearch(&find, code_keywords,
                 sizeof(code_keywords) / sizeof(code_keywords[0]),
                 sizeof(code_keywords[0]), compare_keywords);
}

// See if 'find' is a C/C++ type.
//     Refer to bsearch(3) for return value.
//
void* CodeEditor::search_types(char *find) {
  return bsearch(&find, code_types,
                 sizeof(code_types) / sizeof(code_types[0]),
                 sizeof(code_types[0]), compare_keywords);
}

// 'style_parse()' - Parse text and produce style data.
void CodeEditor::style_parse(const char *in_tbuff,         // text buffer to parse
                             char       *in_sbuff,         // style buffer we modify
                             int         in_len,           // byte length to parse
                             char        in_style) {       // starting style letter
  // Style letters:
  //
  // 'A' - Plain
  // 'B' - Line comments  // ..
  // 'C' - Block comments /*..*/
  // 'D' - Strings        "xxx"
  // 'E' - Directives     #define, #include..
  // 'F' - Types          void, char..
  // 'G' - Keywords       if, while..

  StyleParseInfo spi;
  spi.tbuff  = in_tbuff;
  spi.sbuff  = in_sbuff;
  spi.len    = in_len;
  spi.style  = in_style;
  spi.lwhite = 1;        // 1:while parsing over leading white and first char past, 0:past white
  spi.col    = 0;
  spi.last   = 0;

  char c,c2;
  const char no_crlf = 0;
  while ( spi.len > 0 ) {
    c  = spi.tbuff[0];                     // current char
    c2 = (spi.len > 0) ? spi.tbuff[1] : 0; // next char
    //DEBUG printf("WORKING ON %d (%c) style=%c lwhite=%d len=%d\n", spi.col, c, spi.style, spi.lwhite, spi.len);
    if ( spi.style == 'C' ) {
      // Started *already inside* a block comment? Parse to end of comment or buffer
      spi.parse_block_comment();
    } else if ( spi.style != 'C' && c == '/' && c2 == '*' ) {
      // Start of new block comment? Parse to end of comment or buffer
      spi.parse_block_comment();
    } else if ( c == '\\' ) {
      // Handle backslash escape sequence
      //    Purposefully don't 'handle' \n, since an escaped \n should be
      //    a continuation of a line, such as in a multiline #directive
      //
      if ( !spi.parse_over_char(no_crlf) ) break;     // backslash
      if ( !spi.parse_over_char(no_crlf) ) break;     // char escaped
      continue;
    } else if ( spi.style != 'D' && spi.style != 'B' && c == '/' && c2 == '/' ) {
      spi.parse_line_comment();
      continue;
    } else if ( spi.style != 'D' && c == '"' ) {
      // Start of quoted string?
      spi.parse_quoted_string();
      continue;
    } else if ( c == '#' && spi.lwhite ) { // '#' Directive?
      spi.parse_directive();
      continue;
    } else if ( // spi.style != 'D' &&     // not parsing in a string
                !spi.last && (islower(c) || c == '_') ) { // Possible C/C++ keyword?
      spi.parse_keyword();
      continue;
    }
    spi.last = isalnum(*spi.tbuff) || *spi.tbuff == '_' || *spi.tbuff == '.';
    // Parse over the character
    if ( !spi.parse_over_char() ) break;
  }
}

// 'style_unfinished_cb()' - Update unfinished styles.
void CodeEditor::style_unfinished_cb(int, void*) { }

// 'style_update()' - Update the style buffer...
void CodeEditor::style_update(int pos, int nInserted, int nDeleted,
                              int /*nRestyled*/, const char * /*deletedText*/,
                              void *cbArg) {
  CodeEditor	*editor = (CodeEditor *)cbArg;
  char		*style,				// Style data
		*text;				// Text data

  // If this is just a selection change, just unselect the style buffer...
  if (nInserted == 0 && nDeleted == 0) {
    editor->mStyleBuffer->unselect();
    return;
  }

  // Track changes in the text buffer...
  if (nInserted > 0) {
    // Insert characters into the style buffer...
    style = new char[nInserted + 1];
    memset(style, 'A', nInserted);
    style[nInserted] = '\0';

    editor->mStyleBuffer->replace(pos, pos + nDeleted, style);
    delete[] style;
  } else {
    // Just delete characters in the style buffer...
    editor->mStyleBuffer->remove(pos, pos + nDeleted);
  }

  editor->mStyleBuffer->select(pos, pos + nInserted - nDeleted);

  // Reparse whole buffer, don't get cute. Maybe optimize range later
  int len = editor->buffer()->length();
  text  = editor->mBuffer->text_range(0, len);
  style = editor->mStyleBuffer->text_range(0, len);

  //DEBUG printf("BEFORE:\n"); show_buffer(editor); printf("-- END BEFORE\n");
  style_parse(text, style, editor->mBuffer->length(), 'A');
  //DEBUG printf("AFTER:\n"); show_buffer(editor); printf("-- END AFTER\n");

  editor->mStyleBuffer->replace(0, len, style);
  editor->redisplay_range(0, len);
  editor->redraw();

  free(text);
  free(style);
}

int CodeEditor::auto_indent(int, CodeEditor* e) {
  if (e->buffer()->selected()) {
    e->insert_position(e->buffer()->primary_selection()->start());
    e->buffer()->remove_selection();
  }

  int pos = e->insert_position();
  int start = e->line_start(pos);
  char *text = e->buffer()->text_range(start, pos);
  char *ptr;

  for (ptr = text; isspace(*ptr); ptr ++) {/*empty*/}
  *ptr = '\0';
  if (*text) {
    // use only a single 'insert' call to avoid redraw issues
    int n = strlen(text);
    char *b = (char*)malloc(n+2);
    *b = '\n';
    strcpy(b+1, text);
    e->insert(b);
    free(b);
  } else {
    e->insert("\n");
  }
  e->show_insert_position();
  e->set_changed();
  if (e->when()&FL_WHEN_CHANGED) e->do_callback();

  free(text);

  return 1;
}

// Create a CodeEditor widget...
CodeEditor::CodeEditor(int X, int Y, int W, int H, const char *L) :
  Fl_Text_Editor(X, Y, W, H, L) {
  buffer(new Fl_Text_Buffer);

  char *style = new char[mBuffer->length() + 1];
  char *text = mBuffer->text();

  memset(style, 'A', mBuffer->length());
  style[mBuffer->length()] = '\0';

  highlight_data(new Fl_Text_Buffer(mBuffer->length()), styletable,
                 sizeof(styletable) / sizeof(styletable[0]),
		 'A', style_unfinished_cb, this);

  style_parse(text, style, mBuffer->length(), 'A');

  mStyleBuffer->text(style);
  delete[] style;
  free(text);

  mBuffer->add_modify_callback(style_update, this);
  add_key_binding(FL_Enter, FL_TEXT_EDITOR_ANY_STATE,
                  (Fl_Text_Editor::Key_Func)auto_indent);
}

// Destroy a CodeEditor widget...
CodeEditor::~CodeEditor() {
  Fl_Text_Buffer *buf = mStyleBuffer;
  mStyleBuffer = 0;
  delete buf;

  buf = mBuffer;
  buffer(0);
  delete buf;
}


CodeViewer::CodeViewer(int X, int Y, int W, int H, const char *L)
: CodeEditor(X, Y, W, H, L)
{
  default_key_function(kf_ignore);
  remove_all_key_bindings(&key_bindings);
  cursor_style(CARET_CURSOR);
}


void CodeViewer::draw()
{
  // Tricking Fl_Text_Display into using bearable colors for this specific task
  Fl_Color c = Fl::get_color(FL_SELECTION_COLOR);
  Fl::set_color(FL_SELECTION_COLOR, fl_color_average(FL_BACKGROUND_COLOR, FL_FOREGROUND_COLOR, 0.9f));
  CodeEditor::draw();
  Fl::set_color(FL_SELECTION_COLOR, c);
}

//
// End of "$Id$".
//
