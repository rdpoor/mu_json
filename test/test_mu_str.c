/**
 * @file test_mu_str.c
 *
 * MIT License
 *
 * Copyright (c) 2024 R. D. Poor <rdpoor # gmail.com>
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

#include "../src/mu_str.h"
#include "fff.h"
#include "unity.h"

DEFINE_FFF_GLOBALS;

void setUp(void) {
    // Code to run before each test
}

void tearDown(void) {
    // Code to run after each test
}

void test_mu_str_init(void) {
    mu_str_t s;
    uint8_t buf[10];
    const char *cstr = "Purple Waves";

    TEST_ASSERT_EQUAL_PTR(&s, mu_str_init(&s, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_INT(sizeof(buf), mu_str_length(&s));
    TEST_ASSERT_FALSE(mu_str_is_empty(&s));

    TEST_ASSERT_EQUAL_PTR(&s, mu_str_init(&s, buf, 0));
    TEST_ASSERT_EQUAL_INT(0, mu_str_length(&s));
    TEST_ASSERT_TRUE(mu_str_is_empty(&s));

    TEST_ASSERT_EQUAL_PTR(&s, mu_str_init_cstr(&s, cstr));
    TEST_ASSERT_EQUAL_INT(strlen(cstr), mu_str_length(&s));
    TEST_ASSERT_FALSE(mu_str_is_empty(&s));
}

void test_mu_str_get_byte(void) {
    mu_str_t s;
    const char *cstr = "abcd";
    uint8_t byte;

    mu_str_init_cstr(&s, cstr);
    TEST_ASSERT_FALSE(mu_str_get_byte(&s, -1, &byte));
    TEST_ASSERT_TRUE(mu_str_get_byte(&s, 0, &byte));
    TEST_ASSERT_EQUAL_INT('a', byte);
    TEST_ASSERT_TRUE(mu_str_get_byte(&s, 3, &byte));
    TEST_ASSERT_EQUAL_INT('d', byte);
    TEST_ASSERT_FALSE(mu_str_get_byte(&s, 4, &byte));
}

void test_mu_str_copy(void) {
    mu_str_t s1, s2;
    uint8_t buf[10];

    mu_str_init(&s1, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_PTR(&s2, mu_str_copy(&s2, &s1));
    TEST_ASSERT_EQUAL_PTR(s1.buf, s1.buf);
    TEST_ASSERT_EQUAL_INT(s1.length, s2.length);
}

void test_mu_str_compare(void) {
    mu_str_t s1, s2;

    mu_str_init_cstr(&s1, "Purple Waves");

    mu_str_init_cstr(&s2, "Purple Waves");
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare(&s1, &s2));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare(&s2, &s1));

    mu_str_init_cstr(&s2, "Purple");
    TEST_ASSERT_LESS_THAN_INT(0, mu_str_compare(&s1, &s2));
    TEST_ASSERT_GREATER_THAN_INT(0, mu_str_compare(&s2, &s1));

    mu_str_init_cstr(&s2, "Purple Zebras");
    TEST_ASSERT_GREATER_THAN_INT(0, mu_str_compare(&s1, &s2));
    TEST_ASSERT_LESS_THAN_INT(0, mu_str_compare(&s2, &s1));

    mu_str_init_cstr(&s1, "");
    mu_str_init_cstr(&s2, "");
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare(&s1, &s2));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare(&s2, &s1));

    mu_str_init_cstr(&s1, "Purple Waves");
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s1, "Purple Waves"));
    TEST_ASSERT_LESS_THAN_INT(0, mu_str_compare_cstr(&s1, "Purple"));
    TEST_ASSERT_GREATER_THAN_INT(0, mu_str_compare_cstr(&s1, "Purple Zebras"));
    mu_str_init_cstr(&s1, "");
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s1, ""));
}

void test_mu_str_slice(void) {
    mu_str_t s1, s2, s3;

    mu_str_init_cstr(&s1, "pantomime");

    TEST_ASSERT_EQUAL_PTR(&s2, mu_str_slice(&s2, &s1, 0, MU_STR_END));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, "pantomime"));

    TEST_ASSERT_EQUAL_PTR(&s2, mu_str_slice(&s2, &s1, 5, MU_STR_END));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, "mime"));

    TEST_ASSERT_EQUAL_PTR(&s2, mu_str_slice(&s2, &s1, 0, 5));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, "panto"));

    TEST_ASSERT_EQUAL_PTR(&s2, mu_str_slice(&s2, &s1, 1, 4));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, "ant"));

    TEST_ASSERT_EQUAL_PTR(&s2, mu_str_slice(&s2, &s1, -4, MU_STR_END));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, "mime"));

    TEST_ASSERT_EQUAL_PTR(&s2, mu_str_slice(&s2, &s1, 0, -4));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, "panto"));

    TEST_ASSERT_EQUAL_PTR(&s2, mu_str_slice(&s2, &s1, -8, -5));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, "ant"));

    // start > length
    TEST_ASSERT_EQUAL_PTR(&s2, mu_str_slice(&s2, &s1, 99, 100));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, ""));

    // start > end
    TEST_ASSERT_EQUAL_PTR(&s2, mu_str_slice(&s2, &s1, 4, 2));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, ""));


    TEST_ASSERT_EQUAL_PTR(&s1, mu_str_split(&s2, &s3, &s1, 0));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, ""));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s3, "pantomime"));

    TEST_ASSERT_EQUAL_PTR(&s1, mu_str_split(&s2, &s3, &s1, 1));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, "p"));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s3, "antomime"));

    TEST_ASSERT_EQUAL_PTR(&s1, mu_str_split(&s2, &s3, &s1, 8));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, "pantomim"));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s3, "e"));

    TEST_ASSERT_EQUAL_PTR(&s1, mu_str_split(&s2, &s3, &s1, -1));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, "pantomim"));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s3, "e"));

    TEST_ASSERT_EQUAL_PTR(&s1, mu_str_split(&s2, &s3, &s1, MU_STR_END));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s2, "pantomime"));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&s3, ""));
}

void test_mu_str_find_byte(void) {
    mu_str_t s;

    mu_str_init_cstr(&s, "abcba");
    TEST_ASSERT_EQUAL_INT(0, mu_str_find_byte(&s, 'a'));
    TEST_ASSERT_EQUAL_INT(1, mu_str_find_byte(&s, 'b'));
    TEST_ASSERT_EQUAL_INT(2, mu_str_find_byte(&s, 'c'));
    TEST_ASSERT_EQUAL_INT(MU_STR_NOT_FOUND, mu_str_find_byte(&s, 'd'));

    TEST_ASSERT_EQUAL_INT(4, mu_str_rfind_byte(&s, 'a'));
    TEST_ASSERT_EQUAL_INT(3, mu_str_rfind_byte(&s, 'b'));
    TEST_ASSERT_EQUAL_INT(2, mu_str_rfind_byte(&s, 'c'));
    TEST_ASSERT_EQUAL_INT(MU_STR_NOT_FOUND, mu_str_rfind_byte(&s, 'd'));

    mu_str_init_cstr(&s, "");
    TEST_ASSERT_EQUAL_INT(MU_STR_NOT_FOUND, mu_str_find_byte(&s, 'a'));
    TEST_ASSERT_EQUAL_INT(MU_STR_NOT_FOUND, mu_str_rfind_byte(&s, 'a'));
}

void test_mu_str_find_substr(void) {
    mu_str_t s, subs;

    mu_str_init_cstr(&s, "bananacreampie");
    mu_str_init_cstr(&subs, "banana");
    TEST_ASSERT_EQUAL_INT(0, mu_str_find_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(0, mu_str_find_subcstr(&s, "banana"));
    TEST_ASSERT_EQUAL_INT(0, mu_str_rfind_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(0, mu_str_rfind_subcstr(&s, "banana"));

    mu_str_init_cstr(&subs, "cream");
    TEST_ASSERT_EQUAL_INT(6, mu_str_find_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(6, mu_str_find_subcstr(&s, "cream"));
    TEST_ASSERT_EQUAL_INT(6, mu_str_rfind_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(6, mu_str_rfind_subcstr(&s, "cream"));

    mu_str_init_cstr(&subs, "pie");
    TEST_ASSERT_EQUAL_INT(11, mu_str_find_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(11, mu_str_find_subcstr(&s, "pie"));
    TEST_ASSERT_EQUAL_INT(11, mu_str_rfind_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(11, mu_str_rfind_subcstr(&s, "pie"));

    mu_str_init_cstr(&subs, "an");
    TEST_ASSERT_EQUAL_INT(1, mu_str_find_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(1, mu_str_find_subcstr(&s, "an"));
    TEST_ASSERT_EQUAL_INT(3, mu_str_rfind_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(3, mu_str_rfind_subcstr(&s, "an"));

    mu_str_init_cstr(&subs, "na");
    TEST_ASSERT_EQUAL_INT(2, mu_str_find_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(2, mu_str_find_subcstr(&s, "na"));
    TEST_ASSERT_EQUAL_INT(4, mu_str_rfind_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(4, mu_str_rfind_subcstr(&s, "na"));

    mu_str_init_cstr(&subs, "nac");
    TEST_ASSERT_EQUAL_INT(4, mu_str_find_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(4, mu_str_find_subcstr(&s, "nac"));
    TEST_ASSERT_EQUAL_INT(4, mu_str_rfind_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(4, mu_str_rfind_subcstr(&s, "nac"));

    mu_str_init_cstr(&subs, "mango");
    TEST_ASSERT_EQUAL_INT(MU_STR_NOT_FOUND, mu_str_find_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(MU_STR_NOT_FOUND, mu_str_find_subcstr(&s, "mango"));
    TEST_ASSERT_EQUAL_INT(MU_STR_NOT_FOUND, mu_str_rfind_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(MU_STR_NOT_FOUND, mu_str_rfind_subcstr(&s, "mango"));

    mu_str_init_cstr(&subs, "");
    TEST_ASSERT_EQUAL_INT(0, mu_str_find_substr(&s, &subs));
    TEST_ASSERT_EQUAL_INT(14, mu_str_rfind_substr(&s, &subs));
}

void test_mu_str_find(void) {
    mu_str_t s;
    const char *c = "abc12 xyz";

    mu_str_init_cstr(&s, c);
    // find leftmost word char
    TEST_ASSERT_EQUAL_INT(0, mu_str_find(&s, mu_str_is_word, NULL, true));
    TEST_ASSERT_EQUAL_INT(0, mu_str_find_cstr(c, mu_str_is_word, NULL, true));
    // find leftmost non-word char
    TEST_ASSERT_EQUAL_INT(5, mu_str_find(&s, mu_str_is_word, NULL, false));
    TEST_ASSERT_EQUAL_INT(5, mu_str_find_cstr(c, mu_str_is_word, NULL, false));

    // find rightmost word char
    TEST_ASSERT_EQUAL_INT(8, mu_str_rfind(&s, mu_str_is_word, NULL, true));
    TEST_ASSERT_EQUAL_INT(8, mu_str_rfind_cstr(c, mu_str_is_word, NULL, true));
    // find rightmost non-word char
    TEST_ASSERT_EQUAL_INT(5, mu_str_rfind(&s, mu_str_is_word, NULL, false));
    TEST_ASSERT_EQUAL_INT(5, mu_str_rfind_cstr(c, mu_str_is_word, NULL, false));
}

bool is_not_digit(uint8_t byte, void *arg) {
    return !mu_str_is_digit(byte, arg);
}

void test_mu_str_trim(void) {
    mu_str_t src, dst;

    mu_str_init_cstr(&src, "abc12 xyz");

    TEST_ASSERT_EQUAL_PTR(&dst, mu_str_ltrim(&dst, &src, mu_str_is_word, NULL));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&dst, " xyz"));
    TEST_ASSERT_EQUAL_PTR(&dst, mu_str_ltrim(&dst, &src, is_not_digit, NULL));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&dst, "12 xyz"));

    TEST_ASSERT_EQUAL_PTR(&dst, mu_str_rtrim(&dst, &src, mu_str_is_word, NULL));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&dst, "abc12 "));
    TEST_ASSERT_EQUAL_PTR(&dst, mu_str_rtrim(&dst, &src, is_not_digit, NULL));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&dst, "abc12"));

    TEST_ASSERT_EQUAL_PTR(&dst, mu_str_trim(&dst, &src, mu_str_is_word, NULL));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&dst, " "));
    TEST_ASSERT_EQUAL_PTR(&dst, mu_str_trim(&dst, &src, is_not_digit, NULL));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&dst, "12"));

    // all bytes match predicate, so all bytes get trimmed.
    mu_str_init_cstr(&src, "xyzzy");
    TEST_ASSERT_EQUAL_PTR(&dst, mu_str_ltrim(&dst, &src, mu_str_is_word, NULL));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&dst, ""));

    TEST_ASSERT_EQUAL_PTR(&dst, mu_str_rtrim(&dst, &src, mu_str_is_word, NULL));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&dst, ""));

    TEST_ASSERT_EQUAL_PTR(&dst, mu_str_trim(&dst, &src, mu_str_is_word, NULL));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&dst, ""));
}

void test_mu_str_char_class(void) {
    TEST_ASSERT_TRUE(mu_str_is_whitespace(' ', NULL));
    TEST_ASSERT_TRUE(mu_str_is_whitespace('\t', NULL));
    TEST_ASSERT_TRUE(mu_str_is_whitespace('\n', NULL));
    TEST_ASSERT_TRUE(mu_str_is_whitespace('\r', NULL));
    TEST_ASSERT_TRUE(mu_str_is_whitespace('\f', NULL));
    TEST_ASSERT_TRUE(mu_str_is_whitespace('\v', NULL));
    TEST_ASSERT_FALSE(mu_str_is_whitespace('x', NULL));

    TEST_ASSERT_TRUE(mu_str_is_digit('0', NULL));
    TEST_ASSERT_TRUE(mu_str_is_digit('1', NULL));
    TEST_ASSERT_TRUE(mu_str_is_digit('2', NULL));
    TEST_ASSERT_TRUE(mu_str_is_digit('3', NULL));
    TEST_ASSERT_TRUE(mu_str_is_digit('4', NULL));
    TEST_ASSERT_TRUE(mu_str_is_digit('5', NULL));
    TEST_ASSERT_TRUE(mu_str_is_digit('6', NULL));
    TEST_ASSERT_TRUE(mu_str_is_digit('7', NULL));
    TEST_ASSERT_TRUE(mu_str_is_digit('8', NULL));
    TEST_ASSERT_TRUE(mu_str_is_digit('9', NULL));
    TEST_ASSERT_FALSE(mu_str_is_digit('x', NULL));

    TEST_ASSERT_TRUE(mu_str_is_hex('0', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('1', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('2', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('3', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('4', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('5', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('6', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('7', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('8', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('9', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('a', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('b', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('c', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('d', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('e', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('f', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('A', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('B', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('C', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('D', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('E', NULL));
    TEST_ASSERT_TRUE(mu_str_is_hex('F', NULL));
    TEST_ASSERT_FALSE(mu_str_is_hex('x', NULL));

    TEST_ASSERT_TRUE(mu_str_is_word('0', NULL));
    TEST_ASSERT_TRUE(mu_str_is_word('9', NULL));
    TEST_ASSERT_TRUE(mu_str_is_word('a', NULL));
    TEST_ASSERT_TRUE(mu_str_is_word('z', NULL));
    TEST_ASSERT_TRUE(mu_str_is_word('A', NULL));
    TEST_ASSERT_TRUE(mu_str_is_word('Z', NULL));
    TEST_ASSERT_TRUE(mu_str_is_word('_', NULL));
    TEST_ASSERT_FALSE(mu_str_is_word('.', NULL));
    TEST_ASSERT_FALSE(mu_str_is_word('?', NULL));
}

void test_mu_str_parsers(void) {
    mu_str_t s;

    // blank
    mu_str_init_cstr(&s, "");
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_unsigned_int(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int8(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint8(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int16(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint16(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int32(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint32(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int64(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint64(&s));
    TEST_ASSERT_EQUAL_INT(0x0, mu_str_parse_hex(&s));

    // only 0
    mu_str_init_cstr(&s, "0");
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_unsigned_int(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int8(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint8(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int16(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint16(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int32(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint32(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int64(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint64(&s));
    TEST_ASSERT_EQUAL_INT(0x0, mu_str_parse_hex(&s));

    // non-numeric
    mu_str_init_cstr(&s, "z");
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_unsigned_int(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int8(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint8(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int16(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint16(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int32(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint32(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_int64(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint64(&s));
    TEST_ASSERT_EQUAL_INT(0x0, mu_str_parse_hex(&s));

    // leading 0
    mu_str_init_cstr(&s, "01");
    TEST_ASSERT_EQUAL_INT(1, mu_str_parse_int(&s));
    TEST_ASSERT_EQUAL_INT(1, mu_str_parse_unsigned_int(&s));
    TEST_ASSERT_EQUAL_INT(1, mu_str_parse_int8(&s));
    TEST_ASSERT_EQUAL_INT(1, mu_str_parse_uint8(&s));
    TEST_ASSERT_EQUAL_INT(1, mu_str_parse_int16(&s));
    TEST_ASSERT_EQUAL_INT(1, mu_str_parse_uint16(&s));
    TEST_ASSERT_EQUAL_INT(1, mu_str_parse_int32(&s));
    TEST_ASSERT_EQUAL_INT(1, mu_str_parse_uint32(&s));
    TEST_ASSERT_EQUAL_INT(1, mu_str_parse_int64(&s));
    TEST_ASSERT_EQUAL_INT(1, mu_str_parse_uint64(&s));
    TEST_ASSERT_EQUAL_INT(0x1, mu_str_parse_hex(&s));

    // negative
    mu_str_init_cstr(&s, "-1");
    TEST_ASSERT_EQUAL_INT(-1, mu_str_parse_int(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_unsigned_int(&s));
    TEST_ASSERT_EQUAL_INT(-1, mu_str_parse_int8(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint8(&s));
    TEST_ASSERT_EQUAL_INT(-1, mu_str_parse_int16(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint16(&s));
    TEST_ASSERT_EQUAL_INT(-1, mu_str_parse_int32(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint32(&s));
    TEST_ASSERT_EQUAL_INT(-1, mu_str_parse_int64(&s));
    TEST_ASSERT_EQUAL_INT(0, mu_str_parse_uint64(&s));
    TEST_ASSERT_EQUAL_INT(0x0, mu_str_parse_hex(&s));

    // slightly hexed
    mu_str_init_cstr(&s, "89ab");
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_int(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_unsigned_int(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_int8(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_uint8(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_int16(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_uint16(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_int32(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_uint32(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_int64(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_uint64(&s));
    TEST_ASSERT_EQUAL_INT(0x89ab, mu_str_parse_hex(&s));

    // slightly HEXED
    mu_str_init_cstr(&s, "89AB");
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_int(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_unsigned_int(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_int8(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_uint8(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_int16(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_uint16(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_int32(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_uint32(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_int64(&s));
    TEST_ASSERT_EQUAL_INT(89, mu_str_parse_uint64(&s));
    TEST_ASSERT_EQUAL_INT(0x89ab, mu_str_parse_hex(&s));

    // overflow 8 bit
    mu_str_init_cstr(&s, "257");
    TEST_ASSERT_EQUAL_INT(257, mu_str_parse_int(&s));
    TEST_ASSERT_EQUAL_INT(257, mu_str_parse_unsigned_int(&s));
    TEST_ASSERT_EQUAL_INT(1, mu_str_parse_int8(&s));
    TEST_ASSERT_EQUAL_INT(1, mu_str_parse_uint8(&s));
    TEST_ASSERT_EQUAL_INT(257, mu_str_parse_int16(&s));
    TEST_ASSERT_EQUAL_INT(257, mu_str_parse_uint16(&s));
    TEST_ASSERT_EQUAL_INT(257, mu_str_parse_int32(&s));
    TEST_ASSERT_EQUAL_INT(257, mu_str_parse_uint32(&s));
    TEST_ASSERT_EQUAL_INT(257, mu_str_parse_int64(&s));
    TEST_ASSERT_EQUAL_INT(257, mu_str_parse_uint64(&s));
    TEST_ASSERT_EQUAL_INT(0x257, mu_str_parse_hex(&s));
}

void test_mu_str_examples(void) {
    mu_str_t src, dst, ext;
    size_t index;

    mu_str_init_cstr(&src, "C:/home/test.txt");
    index = mu_str_find_byte(&src, ':');
    mu_str_slice(&dst, &src, index+1, MU_STR_END);
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&dst, "/home/test.txt"));

    mu_str_init_cstr(&src, "C:/home/test.txt");
    index = mu_str_rfind_byte(&src, '.');
    mu_str_split(&dst, &ext, &src, index);
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&dst, "C:/home/test"));
    TEST_ASSERT_EQUAL_INT(0, mu_str_compare_cstr(&ext, ".txt"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mu_str_init);
    RUN_TEST(test_mu_str_get_byte);
    RUN_TEST(test_mu_str_copy);
    RUN_TEST(test_mu_str_compare);
    RUN_TEST(test_mu_str_slice);
    RUN_TEST(test_mu_str_find_byte);
    RUN_TEST(test_mu_str_find_substr);
    RUN_TEST(test_mu_str_find);
    RUN_TEST(test_mu_str_trim);
    RUN_TEST(test_mu_str_char_class);
    RUN_TEST(test_mu_str_parsers);
    RUN_TEST(test_mu_str_examples);

    return UNITY_END();
}
