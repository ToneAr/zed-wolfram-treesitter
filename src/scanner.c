#include "tree_sitter/parser.h"

#include <wctype.h>

enum TokenType {
  COMMENT,
  EMPTY_SLOT_LEADING,
  EMPTY_SLOT_TRAILING,
};

static bool scan_comment(TSLexer *lexer) {
  int depth = 1;

  while (!lexer->eof(lexer)) {
    switch (lexer->lookahead) {
      case '*':
        lexer->advance(lexer, false);
        if (lexer->lookahead == ')') {
          depth--;
          if (depth == 0) {
            lexer->result_symbol = COMMENT;
            lexer->advance(lexer, false);
            lexer->mark_end(lexer);
            return true;
          }
        }
        break;

      case '(':
        lexer->advance(lexer, false);
        if (lexer->lookahead == '*') {
          depth++;
        }
        break;

      default:
        lexer->advance(lexer, false);
        break;
    }
  }

  return false;
}

void *tree_sitter_wolfram_external_scanner_create(void) {
  return NULL;
}

void tree_sitter_wolfram_external_scanner_destroy(void *payload) {
  (void)payload;
}

unsigned tree_sitter_wolfram_external_scanner_serialize(void *payload, char *buffer) {
  (void)payload;
  (void)buffer;
  return 0;
}

void tree_sitter_wolfram_external_scanner_deserialize(void *payload, const char *buffer, unsigned length) {
  (void)payload;
  (void)buffer;
  (void)length;
}

bool tree_sitter_wolfram_external_scanner_scan(void *payload, TSLexer *lexer, const bool *valid_symbols) {
  (void)payload;

  while (iswspace(lexer->lookahead)) {
    lexer->advance(lexer, true);
  }

  // Zero-width empty-slot tokens. The grammar (`commaList`) puts these in
  // valid_symbols only at slot positions inside `[ ]`, `[[ ]]`, `{ }`, and
  // `<| |>`. `_leading` fires when the slot is followed by `,`; `_trailing`
  // fires at the last slot, right before the closing bracket. We never
  // advance for these — the token is zero-width.
  if (valid_symbols[EMPTY_SLOT_LEADING] && lexer->lookahead == ',') {
    lexer->mark_end(lexer);
    lexer->result_symbol = EMPTY_SLOT_LEADING;
    return true;
  }
  if (valid_symbols[EMPTY_SLOT_TRAILING]) {
    int32_t c = lexer->lookahead;
    if (c == ']' || c == '}') {
      lexer->mark_end(lexer);
      lexer->result_symbol = EMPTY_SLOT_TRAILING;
      return true;
    }
    if (c == '|') {
      // Only fire when this is the start of `|>` (Association close), not
      // a stray `|` infix operator. Peek past `|`; mark_end before the peek
      // so the produced token stays zero-width regardless of what follows.
      lexer->mark_end(lexer);
      lexer->advance(lexer, false);
      if (lexer->lookahead == '>') {
        lexer->result_symbol = EMPTY_SLOT_TRAILING;
        return true;
      }
      return false;
    }
  }

  if (lexer->lookahead == '(') {
    lexer->advance(lexer, false);
    if (lexer->lookahead == '*') {
      lexer->advance(lexer, false);
      return scan_comment(lexer);
    }
  }

  return false;
}
