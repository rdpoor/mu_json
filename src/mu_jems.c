/**
 * @file jems.c
 *
 * MIT License
 *
 * Copyright (c) 2022-2024 R. Dunbar Poor
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
 *
 */

// *****************************************************************************
// Includes

#include "mu_jems.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// *****************************************************************************
// Private types and definitions

// *****************************************************************************
// Private (static) storage

// *****************************************************************************
// Private (static, forward) declarations

static mu_jems_t *push_level(mu_jems_t *jems, bool is_object);
static mu_jems_t *pop_level(mu_jems_t *jems);
static mu_jems_t *emit_char(mu_jems_t *jems, char ch);
static mu_jems_t *emit_quoted_byte(mu_jems_t *jems, uint8_t byte);
static mu_jems_t *emit_string(mu_jems_t *jems, const char *s);
static mu_jems_t *emit_quoted_string(mu_jems_t *jems, const char *s);
static mu_jems_t *emit_quoted_bytes(mu_jems_t *jems, const uint8_t *bytes,
                                    size_t len);
static mu_jems_t *commify(mu_jems_t *jems);
static mu_jems_level_t *level_ref(mu_jems_t *jems);

// *****************************************************************************
// Public code

mu_jems_t *mu_jems_init(mu_jems_t *jems, mu_jems_level_t *levels,
                        size_t max_level, mu_jems_writer_fn writer,
                        uintptr_t arg) {
    jems->levels = levels;
    jems->max_level = max_level;
    jems->writer = writer;
    jems->arg = arg;
    return mu_jems_reset(jems);
}

mu_jems_t *mu_jems_reset(mu_jems_t *jems) {
    jems->curr_level = 0;
    level_ref(jems)->item_count = 0;
    level_ref(jems)->is_object = false;
    return jems;
}

mu_jems_t *mu_jems_object_open(mu_jems_t *jems) {
    commify(jems);
    emit_char(jems, '{');
    return push_level(jems, true);
}

mu_jems_t *mu_jems_object_close(mu_jems_t *jems) {
    emit_char(jems, '}');
    return pop_level(jems);
}

mu_jems_t *mu_jems_array_open(mu_jems_t *jems) {
    commify(jems);
    emit_char(jems, '[');
    return push_level(jems, false);
}

mu_jems_t *mu_jems_array_close(mu_jems_t *jems) {
    emit_char(jems, ']');
    return pop_level(jems);
}

mu_jems_t *mu_jems_number(mu_jems_t *jems, double value) {
    char buf[22];
    int64_t i = value;
    if ((double)i == value) {
        // if number can be represented exactly as an int, print as int
        snprintf(buf, sizeof(buf), "%" PRId64, i);
    } else {
        snprintf(buf, sizeof(buf), "%lf", value);
    }
    commify(jems);
    return emit_string(jems, buf);
}

mu_jems_t *mu_jems_integer(mu_jems_t *jems, int64_t value) {
    char buf[22]; // 20 digits, 1 sign, 1 null
    snprintf(buf, sizeof(buf), "%" PRId64, value);
    commify(jems);
    return emit_string(jems, buf);
}

mu_jems_t *mu_jems_string(mu_jems_t *jems, const char *string) {
    commify(jems);
    emit_char(jems, '"');
    emit_quoted_string(jems, string);
    return emit_char(jems, '"');
}

mu_jems_t *mu_jems_bytes(mu_jems_t *jems, const uint8_t *bytes, size_t length) {
    commify(jems);
    emit_char(jems, '"');
    emit_quoted_bytes(jems, bytes, length);
    return emit_char(jems, '"');
}

mu_jems_t *mu_jems_bool(mu_jems_t *jems, bool boolean) {
    commify(jems);
    return emit_string(jems, boolean ? "true" : "false");
}

mu_jems_t *mu_jems_true(mu_jems_t *jems) {
    commify(jems);
    return emit_string(jems, "true");
}

mu_jems_t *mu_jems_false(mu_jems_t *jems) {
    commify(jems);
    return emit_string(jems, "false");
}

mu_jems_t *mu_jems_null(mu_jems_t *jems) {
    commify(jems);
    return emit_string(jems, "null");
}

mu_jems_t *mu_jems_literal(mu_jems_t *jems, const char *literal,
                           size_t n_bytes) {
    commify(jems);
    for (int i = 0; i < n_bytes; i++) {
        const uint8_t b = literal[i];
        emit_char(jems, b);
    }
    return jems;
}

// ***************
// key:value pairs

mu_jems_t *mu_jems_key_object_open(mu_jems_t *jems, const char *key) {
    return mu_jems_object_open(mu_jems_string(jems, key));
}

mu_jems_t *mu_jems_key_array_open(mu_jems_t *jems, const char *key) {
    return mu_jems_array_open(mu_jems_string(jems, key));
}

mu_jems_t *mu_jems_key_number(mu_jems_t *jems, const char *key, double value) {
    return mu_jems_number(mu_jems_string(jems, key), value);
}

mu_jems_t *mu_jems_key_integer(mu_jems_t *jems, const char *key,
                               int64_t value) {
    return mu_jems_integer(mu_jems_string(jems, key), value);
}

mu_jems_t *mu_jems_key_string(mu_jems_t *jems, const char *key,
                              const char *string) {
    return mu_jems_string(mu_jems_string(jems, key), string);
}

mu_jems_t *mu_jems_key_bytes(mu_jems_t *jems, const char *key,
                             const uint8_t *bytes, size_t length) {
    return mu_jems_bytes(mu_jems_string(jems, key), bytes, length);
}

mu_jems_t *mu_jems_key_bool(mu_jems_t *jems, const char *key, bool boolean) {
    return mu_jems_bool(mu_jems_string(jems, key), boolean);
}

mu_jems_t *mu_jems_key_true(mu_jems_t *jems, const char *key) {
    return mu_jems_true(mu_jems_string(jems, key));
}

mu_jems_t *mu_jems_key_false(mu_jems_t *jems, const char *key) {
    return mu_jems_false(mu_jems_string(jems, key));
}

mu_jems_t *mu_jems_key_null(mu_jems_t *jems, const char *key) {
    return mu_jems_null(mu_jems_string(jems, key));
}

mu_jems_t *mu_jems_key_literal(mu_jems_t *jems, const char *key,
                               const char *literal, size_t n_bytes) {
    return mu_jems_literal(mu_jems_string(jems, key), literal, n_bytes);
}

size_t mu_jems_curr_level(mu_jems_t *jems) { return jems->curr_level; }

size_t mu_jems_item_count(mu_jems_t *jems) {
    return level_ref(jems)->item_count;
}

// *****************************************************************************
// Private (static) code

static mu_jems_t *push_level(mu_jems_t *jems, bool is_object) {
    if (jems->curr_level < jems->max_level - 1) {
        jems->curr_level += 1;
        level_ref(jems)->item_count = 0;
        level_ref(jems)->is_object = is_object;
    }
    return jems;
}

static mu_jems_t *pop_level(mu_jems_t *jems) {
    if (jems->curr_level > 0) {
        jems->curr_level -= 1;
    }
    return jems;
}

static mu_jems_t *emit_char(mu_jems_t *jems, char ch) {
    jems->writer(ch, jems->arg);
    return jems;
}

static mu_jems_t *emit_quoted_byte(mu_jems_t *jems, uint8_t byte) {
    if ((byte < 0x20) || (byte >= 127)) {
        char buf[7];
        sprintf(buf, "\\u%04x", byte);
        emit_string(jems, buf);
    } else {
        if ((byte == '\\') || (byte == '"')) {
            emit_char(jems, '\\');
        }
        emit_char(jems, (char)byte);
    }
    return jems;
}

static mu_jems_t *emit_string(mu_jems_t *jems, const char *s) {
    while (*s) {
        emit_char(jems, *s++);
    }
    return jems;
}

static mu_jems_t *emit_quoted_string(mu_jems_t *jems, const char *s) {
    while (*s) {
        emit_quoted_byte(jems, (uint8_t)(*s++));
    }
    return jems;
}

static mu_jems_t *emit_quoted_bytes(mu_jems_t *jems, const uint8_t *bytes,
                                    size_t len) {
    for (int i = 0; i < len; i++) {
        const uint8_t b = bytes[i];
        emit_quoted_byte(jems, b);
    }
    return jems;
}

static mu_jems_t *commify(mu_jems_t *jems) {
    mu_jems_level_t *level = level_ref(jems);
    size_t count = level->item_count;
    if (level->is_object) {
        // within { ... }:
        // the 0th item has no prefix
        // odd items are prefixed with a ':'
        // even items are prefixed with a ','
        if (count > 0) {
            emit_char(jems, (count & 1) ? ':' : ',');
        }
    } else {
        // for other lists:
        // the 0th item has no prefix
        // all other items are prefixed with ','
        if (count > 0) {
            emit_char(jems, ',');
        }
    }
    level->item_count += 1;
    return jems;
}

static mu_jems_level_t *level_ref(mu_jems_t *jems) {
    return &jems->levels[jems->curr_level];
}

// *****************************************************************************
// End of file
