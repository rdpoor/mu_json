/**
 * @file jems.h
 *
 * MIT License
 *
 * Copyright (c) 20222024 R. Dunbar Poor
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
 * @brief a pure-C JSON serializer for embedded systems
 */

#ifndef _MU_JEMS_H_
#define _MU_JEMS_H_

// *****************************************************************************
// Includes

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// *****************************************************************************
// C++ compatibility

#ifdef __cplusplus
extern "C" {
#endif

// *****************************************************************************
// Public types and definitions

typedef struct {
    size_t item_count; // # of items emitted at this level
    bool is_object;    // if true, use ':' separator
} mu_jems_level_t;

// Signature for the mu_jems_emit function
typedef void (*mu_jems_writer_fn)(char ch, uintptr_t arg);

typedef struct _jems {
    mu_jems_level_t *levels;
    size_t max_level;
    size_t curr_level;
    mu_jems_writer_fn writer;
    uintptr_t arg;
} mu_jems_t;

// *****************************************************************************
// Public declarations

/**
 * @brief Initialize the jems system.
 *
 * Example:
 *
 *     #include "jems.h"
 *     #include <stdio.h>
 *
 *     #define MU_JEMS_MAX_LEVEL 10
 *     static mu_jems_level_t mu_jems_levels[MU_JEMS_MAX_LEVEL];
 *     static mu_jems_t mu_jems_obj;
 *     mu_jems_init(&mu_jems_obj, mu_jems_levels, MU_JEMS_MAX_LEVEL, writer, 0);
 *
 * @param jems A jems struct to hold state.
 * @param level An array of mu_jems_level objects.
 * @param max_level The number of elements in @ref level.
 * @param writer A function that takes one char arg and renders it.
 * @param arg User-supplied argument passed to the writer function.
 */
mu_jems_t *mu_jems_init(mu_jems_t *jems, mu_jems_level_t *levels,
                        size_t max_level, mu_jems_writer_fn writer,
                        uintptr_t arg);

/**
 * @brief Reset to top level.
 */
mu_jems_t *mu_jems_reset(mu_jems_t *jems);

/**
 * @brief Start a JSON object, i.e. emit '{'
 */
mu_jems_t *mu_jems_object_open(mu_jems_t *jems);

/**
 * @brief End a JSON object, i.e. emit '}'
 */
mu_jems_t *mu_jems_object_close(mu_jems_t *jems);

/**
 * @brief Start a JSON array, i.e. emit '['
 */
mu_jems_t *mu_jems_array_open(mu_jems_t *jems);

/**
 * @brief End a JSON array, i.e. emit ']'
 */
mu_jems_t *mu_jems_array_close(mu_jems_t *jems);

/**
 * @brief Emit a number in JSON format.
 *
 * Note: if value can be exactly represented as an integer, this is equivalent
 * to mu_jems_integer(jems, value);
 */
mu_jems_t *mu_jems_number(mu_jems_t *jems, double value);

/**
 * @brief Emit an integer in JSON format.
 */
mu_jems_t *mu_jems_integer(mu_jems_t *jems, int64_t value);

/**
 * @brief Emit a null-terminated string in JSON format, quoting as needed.
 */
mu_jems_t *mu_jems_string(mu_jems_t *jems, const char *string);

/**
 * @brief Emit length bytes as a string in JSON format, quoting as needed.
 */
mu_jems_t *mu_jems_bytes(mu_jems_t *jems, const uint8_t *bytes, size_t length);

/**
 * @brief Emit a boolean (true or false) in JSON format.
 */
mu_jems_t *mu_jems_bool(mu_jems_t *jems, bool boolean);

/**
 * @brief Emit a true value in JSON format.
 */
mu_jems_t *mu_jems_true(mu_jems_t *jems);

/**
 * @brief Emit a false value in JSON format.
 */
mu_jems_t *mu_jems_false(mu_jems_t *jems);

/**
 * @brief Emit a null in JSON format.
 */
mu_jems_t *mu_jems_null(mu_jems_t *jems);

/**
 * @brief Emit a literal string verbatim (no quotes)
 */
mu_jems_t *mu_jems_literal(mu_jems_t *jems, const char *literal,
                           size_t n_bytes);

/**
 * @brief Emit a string key followed by an open object.
 */
mu_jems_t *mu_jems_key_object_open(mu_jems_t *jems, const char *key);

/**
 * @brief Emit a string key followed by an open array.
 */
mu_jems_t *mu_jems_key_array_open(mu_jems_t *jems, const char *key);

/**
 * @brief Emit a string key followed by a number.
 *
 * Note: if value can be exactly represented as an integer, this is equivalent
 * to mu_jems_key_integer(jems, key, value);
 */
mu_jems_t *mu_jems_key_number(mu_jems_t *jems, const char *key, double value);

/**
 * @brief Emit a string key followed by an integer.
 */
mu_jems_t *mu_jems_key_integer(mu_jems_t *jems, const char *key, int64_t value);

/**
 * @brief Emit a string key followed by a string, quoting as needed.
 */
mu_jems_t *mu_jems_key_string(mu_jems_t *jems, const char *key,
                              const char *string);

/**
 * @brief Emit a string key followed by a string of bytes in JSON string format.
 */
mu_jems_t *mu_jems_key_bytes(mu_jems_t *jems, const char *key,
                             const uint8_t *bytes, size_t length);

/**
 * @brief Emit a string key followed by boolean (true or false).
 */
mu_jems_t *mu_jems_key_bool(mu_jems_t *jems, const char *key, bool boolean);

/**
 * @brief Emit a string key followed by a JSON true value.
 */
mu_jems_t *mu_jems_key_true(mu_jems_t *jems, const char *key);

/**
 * @brief Emit a string key followed by a JSON false value.
 */
mu_jems_t *mu_jems_key_false(mu_jems_t *jems, const char *key);

/**
 * @brief Emit a string key followed by a JSON null value.
 */
mu_jems_t *mu_jems_key_null(mu_jems_t *jems, const char *key);

/**
 * @brief Emit a string key followed by a literal string verbatim (no quotes)
 */
mu_jems_t *mu_jems_key_literal(mu_jems_t *jems, const char *key,
                               const char *literal, size_t n_bytes);

/**
 * @brief Return the current expression depth.
 */
size_t mu_jems_curr_level(mu_jems_t *jems);

/**
 * @brief Return the number of items emitted at this level.
 */
size_t mu_jems_item_count(mu_jems_t *jems);

// *****************************************************************************
// End of file

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MU_JEMS_H_ */
