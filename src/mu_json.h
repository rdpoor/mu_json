/**
 * MIT License
 *
 * Copyright (c) 2021-2025 R. D. Poor <rdpoor@gmail.com>
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

/**
 * @file mu_json.h
 *
 * @brief Compact, in-place JSON parser written in pure C.  Includes methods for
 * traversing tokens in the parsed tree.
 */

#ifndef _MU_JSON_H_
#define _MU_JSON_H_

// *****************************************************************************
// Includes

#include <stdint.h>
#include <stdbool.h>

// *****************************************************************************
// C++ Compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

/**
 * @brief Enumeration of error codes returned by mu_json functions.
 */
#define DEFINE_MU_JSON_ERRORS(M)                                               \
    M(MU_JSON_ERR_NONE, 0)         /**< No error */                            \
    M(MU_JSON_ERR_BAD_FORMAT, -1)  /**< Illegal JSON format */                 \
    M(MU_JSON_ERR_INCOMPLETE, -2)  /**< JSON ended with unterminated form */   \
    M(MU_JSON_ERR_NO_ENTITIES, -3) /**< No non-whitespace input found */       \
    M(MU_JSON_ERR_STRAY_INPUT, -4) /**< stray bytes after top-level entity */  \
    M(MU_JSON_ERR_NOT_ENOUGH_TOKENS, -5) /**< Not enough tokens provided */    \
    M(MU_JSON_ERR_BAD_ARGUMENT, -6)      /**< Illegal user arguments */        \
    M(MU_JSON_ERR_TOO_DEEP, -7)          /**< Nesting depth exceeded */        \
    M(MU_JSON_ERR_NO_MULTIBYTE, -8)      /**< multibyte utf-8 not supported */ \
    M(MU_JSON_ERR_INTERNAL, -9)          /**< Internal state error */

/**
 * @brief Expand mu_json_err_t types.
 */
#define EXPAND_MU_JSON_ERROR_ENUMS(_name, _value) _name = _value,
typedef enum {
    DEFINE_MU_JSON_ERRORS(EXPAND_MU_JSON_ERROR_ENUMS)
} mu_json_err_t;

#define DEFINE_MU_JSON_TOKEN_TYPES(M)                                          \
    M(MU_JSON_TOKEN_TYPE_UNKNOWN) /* [ ... ] */                                \
    M(MU_JSON_TOKEN_TYPE_ARRAY)   /* [ ... ] */                                \
    M(MU_JSON_TOKEN_TYPE_OBJECT)  /* { ... } */                                \
    M(MU_JSON_TOKEN_TYPE_STRING)  /* "..."   */                                \
    M(MU_JSON_TOKEN_TYPE_NUMBER)  /* 123.45  */                                \
    M(MU_JSON_TOKEN_TYPE_INTEGER) /* 12345 (specialized number) */             \
    M(MU_JSON_TOKEN_TYPE_TRUE)    /* true    */                                \
    M(MU_JSON_TOKEN_TYPE_FALSE)   /* false   */                                \
    M(MU_JSON_TOKEN_TYPE_NULL)    /* null    */

/**
 * @brief Enumeration of JSON token types.
 */
#define EXPAND_MU_JSON_TOKEN_TYPE_ENUMS(_name) _name,
typedef enum {
    DEFINE_MU_JSON_TOKEN_TYPES(EXPAND_MU_JSON_TOKEN_TYPE_ENUMS)
} mu_json_token_type_t;

typedef struct {
    const char *str;            // First character of the token
    unsigned int str_len : 16;  // length of token string
    unsigned int type : 4;      // Token type (up to 16 types)
    unsigned int level : 11;    // Depth in tree (up to 2047)
    unsigned int is_last : 1;   // True if last element of tree
} mu_json_token_t;

#define MAX_TOKEN_DEPTH (1<<11)

// *****************************************************************************
// Public declarations

/**
 * @defgroup json_parsing JSON Parsing
 *
 * @brief Functions for parsing JSON-formatted strings.
 *
 * This group contains functions for parsing JSON-formatted strings.
 */

/**
 * @brief Parse a JSON-formatted string provided as a null-terminated C string.
 * 
 * @ingroup json_parsing
 *
 * Parse the JSON-formatted string into a series of tokens stored in 
 * user-supplied `token_store` containing up to `max_tokens`. The JSON string
 * is expected to be null-terminated.
 *
 * @param str The JSON-formatted string to be parsed, provided as a 
 *        null-terminated C string.
 * @param token_store A user-supplied array of tokens for receiving the parsed
 *        results.
 * @param max_tokens Number of tokens in `token_store`.
 * @return Returns the number of parsed tokens if parsing is successful, or a
 *         negative error code if an error occurs.
 */
int mu_json_parse_str(const char *str,
                      mu_json_token_t *token_store, int max_tokens);

/**
 * @brief Parse a JSON-formatted string stored in a buffer.
 * 
 * @ingroup json_parsing
 *
 * Parse the JSON-formatted buffer `buf` of length `buflen` into a series of
 * tokens stored in the user-supplied `token_store` containing up to`max_tokens`
 * elements.  
 * 
 * @param buf Pointer to a uint8_t array containing the JSON-formatted buffer.
 * @param buflen Length of the JSON-formatted buffer `buf`.
 * @param token_store A user-supplied array of tokens for receiving the parsed
 *        results.
 * @param max_tokens Number of tokens in `token_store`.
 * @return Returns the number of parsed tokens if parsing is successful, or a
 *         negative error code if an error occurs.
 */
int mu_json_parse_buffer(const uint8_t *buf, int buflen,
                         mu_json_token_t *token_store, int max_tokens);


/**
 * @brief Convert an error code into a string.
 */
const char *mu_json_error_name(mu_json_err_t value);

/**
 * @defgroup token_accessor Accessors for parsed tokens
 * 
 * The mu_json parser operates in-place on the user-provided JSON string: it
 * identifies each element of the JSON structure and allocates a token for that
 * element, capturing the extent of the sub-string, the element type, and the
 * depth in the parsed tree.
 * 
 * Notably, it does not attempt to convert the JSON elements into native data
 * types.  For example, parsing the following JSON string:
 * 
 * {"sku":1785, "desc":{"size":[10.5,"EE"], "stock":true}}
 * 
 * will parse into 11 tokens as follows:
 * token  type    depth string
 * t[ 0]: OBJECT  0     {"sku":1785, "desc":{"size":[10.5,"EE"], "stock":true}}
 * t[ 1]: STRING  1     "sku"
 * t[ 2]: INTEGER 1     1785
 * t[ 3]: STRING  1     "desc"
 * t[ 4]: OBJECT  1     {"size":[10.5,"EE"], "stock":true}
 * t[ 5]: STRING  2     "size"
 * t[ 6]: ARRAY   2     [10.5,"EE"]
 * t[ 7]: NUMBER  3     10.5
 * t[ 8]: STRING  3     "EE"
 * t[ 9]: STRING  2     "stock"
 * t[10]: TRUE    2     true
 *  
 * 
 * Thee following functions access various fields from parsed tokens, valid 
 * only after a successful call to one of the @ref json_parsing functions.
 */

/**
 * @brief Return a pointer to the first character in the token's string.
 *
 * @ingroup token_accessor
 * 
 * @param token Pointer to the JSON token.
 * @return pointer to the first character in the token's string, or NULL if
 * `token` is NULL.
 */
const char *mu_json_token_string(mu_json_token_t *token);

/**
 * @brief Return the number of characters in the token's string.
 *
 * @ingroup token_accessor
 * 
 * @param token Pointer to the JSON token.
 * @return the number of bytes in the token's string, or 0 if `token` is NULL.
 */
int mu_json_token_string_length(mu_json_token_t *token);

/**
 * @brief Retrieve the JSON type of a JSON token.
 *
 * @ingroup token_accessor
 * 
 * @param token Pointer to the JSON token.
 * @return JSON type of the JSON token or MU_JSON_TOKEN_TYPE_UNKNOWN if `token`
 * is NULL.
 */
mu_json_token_type_t mu_json_token_type(mu_json_token_t *token);

/**
 * @brief Retrieve the level of a JSON token in the JSON hierarchy.
 *
 * @ingroup token_accessor
 * 
 * Return the level of the JSON token `token` in the JSON hierarchy.
 * The level is 0 for the top-level tokens, and increases for nested tokens.
 *
 * @param token Pointer to the JSON token.
 * @return Depth of the JSON token or -1 if `token` is NULL.
 */
int mu_json_token_level(mu_json_token_t *token);

/**
 * @brief Check if a JSON token is the first token in the list of parsed tokens.
 *
 * @ingroup token_accessor
 * 
 * This is useful for traversing a list of parsed tokens sequentially.
 * 
 * @param token Pointer to a parsed JSON token.
 * @return true if the token is the first token in the token list, false 
 *         otherwise.
 */
bool mu_json_token_is_first(mu_json_token_t *token);

/**
 * @brief Check if a JSON token is the last token in the list of parsed tokens.
 *
 * @ingroup token_accessor
 * 
 * This is useful for traversing a list of parsed tokens sequentially.
 * 
 * @param token Pointer to a parsed JSON token.
 * @return true if the token is the last token in the token list, false 
 *         otherwise.
 */
bool mu_json_token_is_last(mu_json_token_t *token);

/**
 * @defgroup json_navigation Navigating parsed JSON tokens
 *
 * @brief Functions for navigating a parsed colletion of JSON tokens.
 * 
 * Following a successful call to one of the json_parsing functions
 * (e.g. mu_json_parse_str(), etc), `token_store` will contain the
 * parsed tokens stored in preorder: `tokens[0]` is always the root
 * of the JSON tree.
 * 
 * You can navigate the parsed tree using sequential methods:
 * 
 * * mu_json_find_prev_token()
 * * mu_json_find_next_token()
 * 
 * ... or by using the structured methods:
 * 
 * * mu_json_find_root_token()
 * * mu_json_find_parent_token()
 * * mu_json_find_child_token()
 * * mu_json_find_prev_sibling_token()
 * * mu_json_find_next_sibling_token()
 */

/**
 * @brief Retrieves the previous JSON token in a token store.
 *
 * @ingroup json_navigation
 * 
 * Return a pointer to the previous JSON token in the token store, or NULL
 * if already at the beginning of the store.
 *
 * @param token Pointer to the JSON token.
 * @return Pointer to the previous JSON token, or NULL if at start of the 
 * token store or if `token` is NULL.
 */
mu_json_token_t *mu_json_find_prev_token(mu_json_token_t *token);

/**
 * @brief Retrieves the next JSON token in the token store.
 *
 * @ingroup json_navigation
 * 
 * Return a pointer to the next JSON token in the token store, or NULL
 * if already at the end of the store.
 *
 * @param token Pointer to the JSON token.
 * @return Pointer to the next JSON token, or NULL if at the end of the store
 * or if `token` is NULL.
 */
mu_json_token_t *mu_json_find_next_token(mu_json_token_t *token);

/**
 * @brief Retrieves the root token of a JSON token store
 *
 * @ingroup json_navigation
 * 
 * This is always equivalent to tokens[0], assuming at least one token was
 * found.
 *
 * @param token Pointer to the JSON token.
 * @return Pointer to the root JSON token or NULL if `token` is NULL.
 */
mu_json_token_t *mu_json_find_root_token(mu_json_token_t *token);

/**
 * @brief Retrieves the parent token of a JSON token.
 *
 * @ingroup json_navigation
 * 
 * Return a pointer to the parent token of a JSON token, or NULL if this token
 * has no parent (e.g. is already at the root).
 *
 * @param token Pointer to the JSON token.
 * @return Pointer to the parent JSON token, or NULL if token is the root token
 * or if `token` is NULL.
 */
mu_json_token_t *mu_json_find_parent_token(mu_json_token_t *token);

/**
 * @brief Retrieves the first child token of a JSON token.
 *
 * @ingroup json_navigation
 * 
 * Return a pointer to the first child token of the JSON token `token`.
 *
 * @param token Pointer to the JSON token.
 * @return Pointer to the first child JSON token, or NULL if `token` has no 
 * children or if `token` is NULL.
 */
mu_json_token_t *mu_json_find_child_token(mu_json_token_t *token);

/**
 * @brief Retrieves the previous sibling token of a JSON token.
 *
 * @ingroup json_navigation
 * 
 * Return a pointer to the the previous token at the same depth as `token`.
 *
 * @param token Pointer to the JSON token.
 * @return Pointer to the previous sibling JSON token, or NULL if no previous 
 * sibling exists or if `token` is NULL.
 */
mu_json_token_t *mu_json_find_prev_sibling_token(mu_json_token_t *token);

/**
 * @brief Retrieves the next sibling token of a JSON token.
 *
 * @ingroup json_navigation
 * 
 * Return a pointer to the the next token at the same depth as `token`.
 *
 * @param token Pointer to the JSON token.
 * @return Pointer to the next sibling JSON token, or NULL if no next sibling 
 * exists or if `token` is NULL.
 */
mu_json_token_t *mu_json_find_next_sibling_token(mu_json_token_t *token);

// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MU_JSON_H_ */
