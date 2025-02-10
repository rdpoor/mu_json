/**
 * MIT License
 *
 * Copyright (c) 2021-2024 R. D. Poor <rdpoor@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// *****************************************************************************
// Includes

#include "mu_json.h"

#include "mu_log.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// *****************************************************************************
// Private types and definitions

typedef struct parser {
    const char *str;
    int str_len;
    int str_pos;

    mu_json_token_t *token_store;
    int max_tokens;
    int token_count;

    int level;
    mu_json_token_t *current_token;
} parser_t;

// *****************************************************************************
// Private (static) storage

// define s_error_names[], an array that maps an error number to a string
#define EXPAND_ERROR_NAMES(_name, _value) #_name,
static const char *s_error_names[] = {
    DEFINE_MU_JSON_ERRORS(EXPAND_ERROR_NAMES)};
#define N_ERROR_NAMES (sizeof(s_error_names) / sizeof(s_error_names[0]))

// define s_token_type_names[], an array that maps a token type to a string
#define EXPAND_TOKEN_TYPE_NAMES(_name) #_name,
static const char *s_token_type_names[] = {
    DEFINE_MU_JSON_TOKEN_TYPES(EXPAND_TOKEN_TYPE_NAMES)};
#define N_TOKEN_TYPES                                                          \
    (sizeof(s_token_type_names) / sizeof(s_token_type_names[0]))

// *****************************************************************************
// Private (forward) declarations and inline functions

/**
 * @brief Return true if there are no more chars in the input string.
 */
inline static bool at_eos(parser_t *p) { return p->str_pos >= p->str_len; }

/**
 * @brief Access the next char in the input string without fetching it.
 * @note It is an error to call this function if already at end of string.
 */
inline static char peek_char(parser_t *p) { return p->str[p->str_pos]; }

/**
 * @brief Fetch the next char in the input string.
 * @note It is an error to call this function if already at end of string.
 */
inline static char get_char(parser_t *p) { return p->str[p->str_pos++]; }

/**
 * @brief Fetch a pointer to the next chacter in the input string.
 * @note It is an error to call this function if already at end of string.
 */
inline static const char *ref_char(parser_t *p) { return &p->str[p->str_pos]; }

inline static bool is_digit(char ch) { return (ch >= '0') && (ch <= '9'); }

// Helper function to check if a token is NULL
static inline bool token_is_null(mu_json_token_t *token) {
    return token == NULL;
}

// Helper function to check if a token is first in the array
static inline bool token_is_first(mu_json_token_t *token) {
    // assume non-null token
    return token->level == 0;
}

// Helper function to check if a token is last in the array
static inline bool token_is_last(mu_json_token_t *token) {
    // assume non-null token
    return token->is_last;
}

/**
 * @brief Skip over ' ', '\t', '\r', '\n', quitting on non-whitespace char or
 * end of string.
 */
static void skip_whitespace(parser_t *p);

static void parser_reset(parser_t *p);

/**
 * @brief Parse json.  Validates input arguments before calling parse_element().
 */
static int parse_json(const char *str, int str_len,
                      mu_json_token_t *token_store, int token_count);

/**
 * @brief Parse json.  Toplevel element must be a container (array or object)
 * or a singleton primary type (number, string, true, false, null).
 */
static mu_json_err_t parse_element(parser_t *p);

static mu_json_err_t parse_string(parser_t *p);
static mu_json_err_t parse_number(parser_t *p);
static mu_json_err_t parse_literal(parser_t *p, const char *literal,
                                   mu_json_token_type_t type);
static mu_json_err_t parse_object(parser_t *p);
static mu_json_err_t parse_array(parser_t *p);

static bool is_hex(char ch);

static mu_json_token_t *alloc_token(parser_t *p);

static mu_json_token_t *init_token(parser_t *p, mu_json_token_type_t type);

static void finalize_token(parser_t *p, mu_json_token_t *tok);

static void set_token_string(mu_json_token_t *token, const char *string);

static const char *get_token_string(mu_json_token_t *token);
static void set_token_string_length(mu_json_token_t *token, int string_length);
static void set_token_type(mu_json_token_t *token,
                           mu_json_token_type_t token_type);
static void set_token_level(mu_json_token_t *token, int level);
static void set_token_is_last(mu_json_token_t *token, bool is_last);

static const char *token_type_name(mu_json_token_type_t type);
static void log_input_state(const char *message, parser_t *p);

/**
 * @brief Search forward (or backward) to find the next (or previous) sibling,
 * using the m() function to move forward (or backward) through the tree.
 */
static mu_json_token_t *find_sibling(mu_json_token_t *token,
                                     mu_json_token_t *(*m)(mu_json_token_t *));

// *****************************************************************************
// Public code

int mu_json_parse_str(const char *str, mu_json_token_t *token_store,
                      int max_tokens) {
    return parse_json(str, strlen(str), token_store, max_tokens);
}

int mu_json_parse_buffer(const uint8_t *input, int input_len,
                         mu_json_token_t *token_store, int max_tokens) {
    return parse_json((const char *)input, input_len, token_store, max_tokens);
}

const char *mu_json_error_name(mu_json_err_t errnum) {
    errnum = -errnum; // mu_json_err_t values are negative
    if (errnum < 0) {
        return s_error_names[0];
    } else if (errnum >= N_ERROR_NAMES) {
        return "MU_JSON_ERR_UNKNOWN";
    } else {
        return s_error_names[errnum];
    }
}

const char *mu_json_token_string(mu_json_token_t *token) {
    if (token_is_null(token)) {
        return NULL;
    } else {
        return token->str;
    }
}

int mu_json_token_string_length(mu_json_token_t *token) {
    if (token_is_null(token)) {
        return -1;
    } else {
        return token->str_len;
    }
}

mu_json_token_type_t mu_json_token_type(mu_json_token_t *token) {
    if (token_is_null(token)) {
        return MU_JSON_TOKEN_TYPE_UNKNOWN;
    } else {
        return token->type;
    }
}

int mu_json_token_level(mu_json_token_t *token) {
    if (token_is_null(token)) {
        return -1;
    } else {
        return token->level;
    }
}

bool mu_json_token_is_first(mu_json_token_t *token) {
    if (token_is_null(token)) {
        return false;
    } else {
        return token_is_first(token);
    }
}

bool mu_json_token_is_last(mu_json_token_t *token) {
    if (token_is_null(token)) {
        return false;
    } else {
        return token_is_last(token);
    }
}

// ****************************************
// Navigating token trees.
//
// NOTE: These functions asssume:
// - tokens are members of a densly packed array, i.e. prev = &token[-1]
// - the token tree has depth-first ordering
// - the first token in the array is the root, has level == 0
// - the last token in the array has 'is_last' set

mu_json_token_t *mu_json_find_root_token(mu_json_token_t *token) {
    if (token_is_null(token))
        return NULL;
    while (!token_is_first(token)) {
        token = &token[-1];
    }
    return token;
}

mu_json_token_t *mu_json_find_next_token(mu_json_token_t *token) {
    if (token_is_null(token) || token_is_last(token))
        return NULL;
    return &token[1];
}

mu_json_token_t *mu_json_find_prev_token(mu_json_token_t *token) {
    if (token_is_null(token) || token_is_first(token))
        return NULL;
    return &token[-1];
}

mu_json_token_t *mu_json_find_parent_token(mu_json_token_t *token) {
    if (token_is_null(token) || token_is_first(token))
        return NULL;

    mu_json_token_t *current = token;
    int target_level = token->level - 1;

    while (current->level > target_level) {
        current = mu_json_find_prev_token(current);
        if (token_is_null(current))
            return NULL;
    }

    return current;
}

mu_json_token_t *mu_json_find_child_token(mu_json_token_t *token) {
    if (token_is_null(token) || token_is_last(token))
        return NULL;

    mu_json_token_t *next = mu_json_find_next_token(token);
    if (token_is_null(next) || next->level <= token->level)
        return NULL;

    return next;
}

mu_json_token_t *mu_json_find_next_sibling_token(mu_json_token_t *token) {
    return find_sibling(token, mu_json_find_next_token);
}

mu_json_token_t *mu_json_find_prev_sibling_token(mu_json_token_t *token) {
    return find_sibling(token, mu_json_find_prev_token);
}

// *****************************************************************************
// Private (static) code

static void skip_whitespace(parser_t *p) {
    while (!at_eos(p)) {
        char ch = peek_char(p);
        if ((ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r')) {
            get_char(p);
        } else {
            break;
        }
    }
}

static void parser_reset(parser_t *p) {
    mu_json_token_t *tokens = p->token_store;

    // reset all tokens and mutable parts of the parser
    memset(tokens, 0, sizeof(mu_json_token_t) * p->max_tokens);
    p->str_pos = 0;
    p->token_count = 0;
    p->level = 0;
    p->current_token = NULL;
}

static int parse_json(const char *str, int str_len,
                      mu_json_token_t *token_store, int max_tokens) {
    mu_json_err_t err;

    if (str == NULL) {
        return MU_JSON_ERR_BAD_ARGUMENT;
    } else if (token_store == NULL) {
        return MU_JSON_ERR_BAD_ARGUMENT;
    } else if (str_len == 0) {
        return MU_JSON_ERR_BAD_ARGUMENT;
    } else if (max_tokens == 0) {
        // TODO: consider using max_tokens == 0 for 'dry run', returning the
        // number of tokens required.
        return MU_JSON_ERR_BAD_ARGUMENT;
    }

    parser_t parser = {
        .str = str,
        .str_len = str_len,
        .token_store = token_store,
        .max_tokens = max_tokens,
    };
    parser_t *p = &parser;
    parser_reset(p);

    err = parse_element(p);

    // endgame
    if (err != MU_JSON_ERR_NONE) {
        // encountered error while parsing
        MU_LOG_DEBUG("parse_json: %s", mu_json_error_name(err));
        return err;

    } else if (p->token_count == 0) {
        MU_LOG_DEBUG("parse_json: %s", mu_json_error_name(err));
        return MU_JSON_ERR_NO_ENTITIES;
    }

    skip_whitespace(p);

    if (!at_eos(p)) {
        // stray non-whitespace characters follow the toplevel entity
        MU_LOG_DEBUG("parse_json: %s", mu_json_error_name(err));
        return MU_JSON_ERR_STRAY_INPUT;
    }

    // success!
    // token traversal functions require last marker.
    set_token_is_last(&p->token_store[p->token_count - 1], true);
    return p->token_count;
}

static mu_json_err_t parse_element(parser_t *p) {
    // Parse one entity.
    // Return with peek_char() set to the first char that terrminated the
    // entity, or end of string.
    mu_json_err_t err = MU_JSON_ERR_NONE;

    MU_LOG_TRACE("parse_element(\"%.*s\")", p->str_len - p->str_pos,
                 p->str + p->str_pos);

    skip_whitespace(p);

    // if (!at_eos(p)) {
    //     switch (peek_char(p)) {
    //     case '"':
    //         err = parse_string(p);
    //         break;
    //     case '-':
    //     case '0':
    //     case '1':
    //     case '2':
    //     case '3':
    //     case '4':
    //     case '5':
    //     case '6':
    //     case '7':
    //     case '8':
    //     case '9':
    //         err = parse_number(p);
    //         break;
    //     case 't':
    //         err = parse_literal(p, "true", MU_JSON_TOKEN_TYPE_TRUE);
    //         break;
    //     case 'f':
    //         err = parse_literal(p, "false", MU_JSON_TOKEN_TYPE_FALSE);
    //         break;
    //     case 'n':
    //         err = parse_literal(p, "null", MU_JSON_TOKEN_TYPE_NULL);
    //         break;
    //     case '{':
    //         err = parse_object(p);
    //         break;
    //     case '[':
    //         err = parse_array(p);
    //         break;
    //     default:
    //         MU_LOG_DEBUG("parse_element: unrecognized input = '%.*s'",
    //                      p->str_len - p->str_pos, p->str + p->str_pos);
    //         err = MU_JSON_ERR_BAD_FORMAT;
    //         break;
    //     }
    // }

    if (!at_eos(p)) {
        char ch = peek_char(p);
        if (ch == '"') {
            err = parse_string(p);
        } else if ((is_digit(ch) || ch == '-')) {
            err = parse_number(p);
        } else if (ch == 't') {
            err = parse_literal(p, "true", MU_JSON_TOKEN_TYPE_TRUE);            
        } else if (ch == 'f') {
            err = parse_literal(p, "false", MU_JSON_TOKEN_TYPE_FALSE);            
        } else if (ch == 'n') {
            err = parse_literal(p, "null", MU_JSON_TOKEN_TYPE_NULL);            
        } else if (ch == '{') {
            err = parse_object(p);            
        } else if (ch == '[') {
            err = parse_array(p);            
        } else {
            MU_LOG_DEBUG("parse_element: unrecognized input = '%.*s'",
                         p->str_len - p->str_pos, p->str + p->str_pos);
            if (ch & 0x80) {
                err = MU_JSON_ERR_NO_MULTIBYTE;
            } else {
                err = MU_JSON_ERR_BAD_FORMAT;            
            }
        }
    }
    return err;
}

// *****************************************
// About parse_xxx(parser_t *p):
//
// Upon entry, peek_char(p) will return the byte that starts the token (string,
// number, literal, etc).
// Upon success:
// -- a token has been allocated and initialized (but see object and array)
// -- at_eos(p) returns true if the last byte in the input string was consumed.
// -- else peek_char(p) returns the first byte past the end of the token
// -- the parse_xxx(p) function returns MU_JSON_ERR_NONE
// Upon failuer:
// -- the parse_xxx(p) function returns an appropriate error code.
// -- the state of peek_char() and most recent token is undefined.

mu_json_err_t parse_string(parser_t *p) {

    if (at_eos(p) || peek_char(p) != '"') {
        MU_LOG_DEBUG("parse_string: expected opening '\"', input = '%.*s'",
                     p->str_len - p->str_pos, p->str + p->str_pos);
        return MU_JSON_ERR_INTERNAL;
    }

    MU_LOG_TRACE("parse_string(\"%.*s\")", p->str_len - p->str_pos,
                 p->str + p->str_pos);

    // Allocate and initialize a token to hold the string

    mu_json_token_t *tok = init_token(p, MU_JSON_TOKEN_TYPE_STRING);
    if (tok == NULL) {
        return MU_JSON_ERR_NOT_ENOUGH_TOKENS; // Failed to allocate a token
    }
    get_char(p); // consume the opening quote

    // Parse the string content
    while (!at_eos(p)) {
        char ch = peek_char(p);
        MU_LOG_TRACE("parse_string: '%c' (0x%02x), input = '%.*s'", ch, 
            (unsigned char)ch,
                     p->str_len - p->str_pos, p->str + p->str_pos);

        // Handle escape sequences
        if (ch == '\\') {
            get_char(p); // consume the '\'
            if (at_eos(p)) {
                return MU_JSON_ERR_INCOMPLETE; // Incomplete escape sequence
            }
            char escape_char = peek_char(p);
            switch (escape_char) {
            case '"':  // Double quote
            case '\\': // Backslash
            case '/':  // Forward slash
            case 'b':  // Backspace
            case 'f':  // Form feed
            case 'n':  // Newline
            case 'r':  // Carriage return
            case 't':  // Tab
                // Valid escape sequence, continue
                get_char(p); // consume the escape char
                break;
            case 'u':        // Unicode escape sequence (4 hex digits)
                get_char(p); // consume the escape char
                for (int i = 0; i < 4; i++) {
                    if (at_eos(p) || !is_hex(peek_char(p))) {
                        return MU_JSON_ERR_BAD_FORMAT; // Invalid Unicode escape
                    }
                    get_char(p); // Consume the hex digit
                }
                break;
            default:
                return MU_JSON_ERR_BAD_FORMAT; // Invalid escape sequence
            }
        } else if (ch & 0x80) {
            return MU_JSON_ERR_NO_MULTIBYTE;
        } else if (ch < 0x20) {
            // Control characters are not allowed in JSON strings
            return MU_JSON_ERR_BAD_FORMAT;
        } else if (ch == '"') {
            // Found closing quote
            break;
        } else {
            // ordinary char -- fetch next
            get_char(p);
        }
    }

    if (at_eos(p)) {
        MU_LOG_DEBUG("parse_string: premature end of input");
        return MU_JSON_ERR_INCOMPLETE; // Premature end of input
    }
    if (peek_char(p) != '"') {
        MU_LOG_DEBUG(
            "parse_string: broke on something other than '\"', input = '%.*s'",
            p->str_len - p->str_pos, p->str + p->str_pos);
        return MU_JSON_ERR_INTERNAL;
    }

    get_char(p); // consume the closing quote

    // Success. Finish initializing the token.
    finalize_token(p, tok);
    return MU_JSON_ERR_NONE;
}

static bool is_number_prefix(char ch) {
    return (ch == '-') || ((ch >= '0') && (ch <= '9'));
}

static mu_json_err_t parse_number(parser_t *p) {

    if (at_eos(p) || !is_number_prefix(peek_char(p))) {
        MU_LOG_DEBUG("parse_string: expected opening '\"', input = '%.*s'",
                     p->str_len - p->str_pos, p->str + p->str_pos);
        return MU_JSON_ERR_INTERNAL;
    }

    MU_LOG_TRACE("parse_number(\"%.*s\")", p->str_len - p->str_pos,
                 p->str + p->str_pos);

    // Start with INTEGER, but promote to NUMBER if there's a fractional part.
    mu_json_token_t *tok = init_token(p, MU_JSON_TOKEN_TYPE_INTEGER);
    if (tok == NULL) {
        return MU_JSON_ERR_NOT_ENOUGH_TOKENS; // Failed to allocate a token
    }

    // Check if the number is negative
    if (peek_char(p) == '-') {
        get_char(p); // Consume the '-'
    }

    if (at_eos(p)) {
        return MU_JSON_ERR_INCOMPLETE;
    }

    // Parse leading zero if present...
    bool has_leading_zero = false;
    if (!at_eos(p)) {
        char ch = peek_char(p);
        if (ch == '0') {
            has_leading_zero = true;
            get_char(p); // consume the zero
        }
    }

    // Prohibit multiple leading zeros
    if (has_leading_zero && !at_eos(p)) {
        char ch = peek_char(p);
        if (ch == '0') {
            // multiple leading 0s are not allowed
            return MU_JSON_ERR_BAD_FORMAT;
        }
    }

    // Parse the integer part
    bool has_integer_part = false;
    while (!at_eos(p)) {
        char ch = peek_char(p);
        if (ch >= '0' && ch <= '9') {
            has_integer_part = true;
            get_char(p); // Consume the digit
        } else {
            break;
        }
    }

    // If there was a leading zero followed by integers, the number is invalid
    if (has_leading_zero && has_integer_part) {
        MU_LOG_DEBUG("parse_number: has leading zero followed by digits");
        return MU_JSON_ERR_BAD_FORMAT;
    }

    // If there was no integer part, the number is invalid
    if (!has_leading_zero && !has_integer_part) {
        MU_LOG_DEBUG("parse_number: no integer part nor leading 0");
        return MU_JSON_ERR_BAD_FORMAT;
    }

    // Parse the fractional part (if any)
    bool has_fractional_part = false;
    if (!at_eos(p) && peek_char(p) == '.') {
        set_token_type(tok, MU_JSON_TOKEN_TYPE_NUMBER); // promote to number
        get_char(p);                                    // Consume the '.'
        while (!at_eos(p)) {
            char ch = peek_char(p);
            if (ch >= '0' && ch <= '9') {
                has_fractional_part = true;
                get_char(p); // Consume the digit
            } else {
                break;
            }
        }
        // If there was a '.' but no digits after it, the number is invalid
        if (!has_fractional_part) {
            MU_LOG_DEBUG("parse_number: has . but no fractional part");
            return MU_JSON_ERR_BAD_FORMAT;
        }
    }

    // Parse the exponent part (if any)
    if (!at_eos(p) && (peek_char(p) == 'e' || peek_char(p) == 'E')) {
        set_token_type(tok, MU_JSON_TOKEN_TYPE_NUMBER); // promote to number
        get_char(p); // Consume the 'e' or 'E'
        // Check for an optional '+' or '-' sign
        if (!at_eos(p) && (peek_char(p) == '+' || peek_char(p) == '-')) {
            get_char(p); // Consume the sign
        }
        // Parse the exponent digits
        bool has_exponent_digits = false;
        while (!at_eos(p)) {
            char ch = peek_char(p);
            if (ch >= '0' && ch <= '9') {
                has_exponent_digits = true;
                get_char(p); // Consume the digit
            } else {
                break;
            }
        }
        // If there was an 'e' or 'E' but no digits after it, the number is
        // invalid
        if (!has_exponent_digits) {
            MU_LOG_DEBUG("parse_number: has 'e' but no following digits");
            return MU_JSON_ERR_BAD_FORMAT;
        }
    }

    // success.  Finish initializing the token.
    finalize_token(p, tok);
    return MU_JSON_ERR_NONE;
}

static mu_json_err_t parse_literal(parser_t *p, const char *literal,
                                   mu_json_token_type_t type) {
    if (at_eos(p)) {
        MU_LOG_DEBUG("parse_literal: eos encountered");
        return MU_JSON_ERR_INTERNAL;
    }

    MU_LOG_TRACE("parse_literal(\"%.*s\")", p->str_len - p->str_pos,
                 p->str + p->str_pos);

    int literal_len = strlen(literal);

    // allocate and initialize a token to hold the literal
    mu_json_token_t *tok = init_token(p, type);
    if (tok == NULL) {
        return MU_JSON_ERR_NOT_ENOUGH_TOKENS; // Failed to allocate a token
    }

    // Match the literal character by character
    for (int i = 0; i < literal_len; i++) {
        if (at_eos(p)) {
            // premature end of input
            return MU_JSON_ERR_INCOMPLETE;
        } else if (get_char(p) != literal[i]) {
            // mismatch
            return MU_JSON_ERR_BAD_FORMAT;
        }
    }

    // success.  Finish initializing the token.
    finalize_token(p, tok);
    return MU_JSON_ERR_NONE;
}

mu_json_err_t find_and_skip(parser_t *p, char delimiter) {
    MU_LOG_TRACE("find_and_skip('%c'): on entry, input = '%.*s'", delimiter,
                 p->str_len - p->str_pos, p->str + p->str_pos);
    skip_whitespace(p);
    if (at_eos(p)) {
        MU_LOG_DEBUG("find_and_skip: eos hit before '%c'", delimiter);
        return MU_JSON_ERR_BAD_FORMAT;
    }
    if (peek_char(p) != delimiter) {
        MU_LOG_DEBUG("find_and_skip: '%c' not found", delimiter);
        return MU_JSON_ERR_BAD_FORMAT;
    }
    get_char(p); // consume the delimiter
    skip_whitespace(p);
    if (at_eos(p)) {
        MU_LOG_DEBUG("find_and_skip: eos hit after '%c'", delimiter);
        return MU_JSON_ERR_BAD_FORMAT;
    }
    MU_LOG_TRACE("find_and_skip('%c'): on exit, input = '%.*s'", delimiter,
                 p->str_len - p->str_pos, p->str + p->str_pos);
    return MU_JSON_ERR_NONE;
}

mu_json_err_t parse_object(parser_t *p) {
    mu_json_err_t err;

    if (at_eos(p) || peek_char(p) != '{') {
        MU_LOG_DEBUG("parse_object: expected opening '{', input = '%.*s'",
                     p->str_len - p->str_pos, p->str + p->str_pos);
        return MU_JSON_ERR_INTERNAL;
    }

    MU_LOG_TRACE("parse_object(\"%.*s\")", p->str_len - p->str_pos,
                 p->str + p->str_pos);

    // Allocate and initialize a token to hold the object
    mu_json_token_t *tok = init_token(p, MU_JSON_TOKEN_TYPE_OBJECT);
    if (tok == NULL) {
        return MU_JSON_ERR_NOT_ENOUGH_TOKENS; // Failed to allocate a token
    }

    // Increment level for nested parsing
    p->level++;

    get_char(p); // Consume the opening '{' (it is included in the token)

    // Parse key-value pairs
    err = MU_JSON_ERR_NONE;
    bool first_pair = true;
    while (true) {
        skip_whitespace(p);
        if (at_eos(p)) {
            break;
        }
        if (peek_char(p) == '}') {
            break; // end of object
        }
        if (!first_pair) {
            err = find_and_skip(p, ',');
            if (err != MU_JSON_ERR_NONE) {
                MU_LOG_DEBUG("Failed to find ',' delimeter");
                return err;
            }
        }
        first_pair = false;
        // expecting a string
        err = parse_string(p);
        if (err != MU_JSON_ERR_NONE) {
            MU_LOG_DEBUG("parse_object: expected string key, got %s",
                         mu_json_error_name(err));
            return err;
        }
        // expecting : delimiter
        err = find_and_skip(p, ':');
        if (err != MU_JSON_ERR_NONE) {
            MU_LOG_DEBUG("Failed to find ':' delimeter");
            return err;
        }
        err = parse_element(p);
        if (err != MU_JSON_ERR_NONE) {
            MU_LOG_DEBUG("parse_object: expected entity value, got %s",
                         mu_json_error_name(err));
            return err;
        }
    } // while()...

    // Here because '}' was found or end of input

    if (at_eos(p)) {
        MU_LOG_DEBUG("parse_object: premature end of input following pair");
        return MU_JSON_ERR_INCOMPLETE; // Premature end of input
    }
    if (peek_char(p) != '}') {
        MU_LOG_DEBUG(
            "parse_object: broke on something other than '}', input = '%.*s'",
            p->str_len - p->str_pos, p->str + p->str_pos);
        return MU_JSON_ERR_INTERNAL;
    }

    // Closing '}' was seen: Decrement level after parsing the array
    get_char(p); // include '}' in token
    p->level--;

    // Success. Finish initializing the token.
    set_token_string_length(tok, ref_char(p) - get_token_string(tok));
    MU_LOG_TRACE("parse_object found object '%.*s'", tok->str_len, tok->str);
    return MU_JSON_ERR_NONE;
}

mu_json_err_t parse_array(parser_t *p) {
    mu_json_err_t err;

    if (at_eos(p) || peek_char(p) != '[') {
        MU_LOG_DEBUG("parse_arrray: expected opening '[', input = '%.*s'",
                     p->str_len - p->str_pos, p->str + p->str_pos);
        return MU_JSON_ERR_INTERNAL;
    }

    MU_LOG_TRACE("parse_array(\"%.*s\")", p->str_len - p->str_pos,
                 p->str + p->str_pos);

    // Allocate and initialize a token to hold the array
    mu_json_token_t *tok = init_token(p, MU_JSON_TOKEN_TYPE_ARRAY);
    if (tok == NULL) {
        return MU_JSON_ERR_NOT_ENOUGH_TOKENS; // Failed to allocate a token
    }

    // Increment level for nested parsing
    p->level++;

    // Consume the opening '[' (it is included in the token string)
    get_char(p);

    // Parse array elements
    bool first_element = true;
    while (true) {
        skip_whitespace(p);
        if (at_eos(p)) {
            break;
        }

        if (peek_char(p) == ']') {
            break; // end of array
        }
        if (!first_element) {
            err = find_and_skip(p, ',');
            if (err != MU_JSON_ERR_NONE) {
                MU_LOG_DEBUG("Failed to find ',' delimeter");
                return err;
            }
        }
        first_element = false;

        // expecting an element...
        mu_json_err_t err = parse_element(p);
        if (err != MU_JSON_ERR_NONE) {
            return err; // Propagate errors from parse_element
        }
    } // while...

    if (at_eos(p)) {
        MU_LOG_DEBUG("parse_array: premature end of input following entity");
        return MU_JSON_ERR_INCOMPLETE; // Premature end of input
    }
    if (peek_char(p) != ']') {
        MU_LOG_DEBUG(
            "parse_array: broke on something other than ']', input = '%.*s'",
            p->str_len - p->str_pos, p->str + p->str_pos);
        return MU_JSON_ERR_INTERNAL;
    }

    // Closing ']' was seen: Decrement level after parsing the array
    get_char(p); // include '}' in token
    p->level--;

    // Success.  Finish initializing the token.
    set_token_string_length(tok, ref_char(p) - get_token_string(tok));
    MU_LOG_TRACE("parse_array found %s '%.*s'", token_type_name(tok->type),
                 tok->str_len, tok->str);
    return MU_JSON_ERR_NONE;
}

static bool is_hex(char ch) {
    return ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') ||
            (ch >= 'a' && ch <= 'f'));
}

static mu_json_token_t *alloc_token(parser_t *p) {
    if (p->token_count >= p->max_tokens) {
        return NULL;
    } else {
        return &p->token_store[p->token_count++];
    }
}

static mu_json_token_t *init_token(parser_t *p, mu_json_token_type_t type) {
    mu_json_token_t *tok = alloc_token(p);
    if (tok == NULL) {
        return NULL;
    }
    set_token_string(tok, ref_char(p));
    set_token_type(tok, type);
    set_token_level(tok, p->level);
    return tok;
}

static void finalize_token(parser_t *p, mu_json_token_t *tok) {
    set_token_string_length(tok, ref_char(p) - get_token_string(tok));
    MU_LOG_TRACE("found %s '%.*s'", token_type_name(tok->type), tok->str_len,
                 tok->str);
}

static void set_token_string(mu_json_token_t *token, const char *string) {
    token->str = string;
}

static const char *get_token_string(mu_json_token_t *token) {
    return token->str;
}

static void set_token_string_length(mu_json_token_t *token, int string_length) {
    token->str_len = string_length;
}

static void set_token_type(mu_json_token_t *token,
                           mu_json_token_type_t token_type) {
    token->type = token_type;
}

static void set_token_level(mu_json_token_t *token, int level) {
    token->level = level;
}

static void set_token_is_last(mu_json_token_t *token, bool is_last) {
    token->is_last = is_last;
}

static const char *token_type_name(mu_json_token_type_t type) {
    if (type >= N_TOKEN_TYPES) {
        return "MU_JSON_TOKEN_TYPE_UNKNOWN";
    } else {
        return s_token_type_names[type];
    }
}

static void log_input_state(const char *message, parser_t *p) {
    MU_LOG_DEBUG("%s: input = '%.*s'", message, p->str_len - p->str_pos,
                 p->str + p->str_pos);
}

static mu_json_token_t *find_sibling(mu_json_token_t *token,
                                     mu_json_token_t *(*m)(mu_json_token_t *)) {
    if (token_is_null(token) || token_is_first(token))
        return NULL;

    mu_json_token_t *current = token;
    int target_level = token->level;

    while (true) {
        current = m(current);
        if (token_is_null(current))
            return NULL;
        if (current->level == target_level)
            return current;
        if (current->level < target_level)
            return NULL;
    }
}
