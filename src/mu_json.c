/**
 * @file mu_json.c
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

// *****************************************************************************
// Includes

#include "mu_json.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// *****************************************************************************
// Private types and definitions

#define DEBUG_TRACE
#ifdef DEBUG_TRACE
#include <stdio.h>
#define TRACE_PRINTF(...) fprintf(stderr, __VA_ARGS__)
#else
#define TRACE_PRINTF(...)
#endif

#define __ (uint8_t) - 1 /* the universal error code */

// The 128 ASCII chars map into one of the following character classes.
// See s_c_class_map[]
//
#define DEFINE_CHAR_CLASSES(M)                                                 \
    M(C_SPACE) /* space */                                                     \
    M(C_WHITE) /* other whitespace */                                          \
    M(C_LCURB) /* {  */                                                        \
    M(C_RCURB) /* } */                                                         \
    M(C_LSQRB) /* [ */                                                         \
    M(C_RSQRB) /* ] */                                                         \
    M(C_COLON) /* : */                                                         \
    M(C_COMMA) /* , */                                                         \
    M(C_QUOTE) /* " */                                                         \
    M(C_BACKS) /* \ */                                                         \
    M(C_SLASH) /* / */                                                         \
    M(C_PLUS)  /* + */                                                         \
    M(C_MINUS) /* - */                                                         \
    M(C_POINT) /* . */                                                         \
    M(C_ZERO)  /* 0 */                                                         \
    M(C_DIGIT) /* 123456789 */                                                 \
    M(C_LOW_A) /* a */                                                         \
    M(C_LOW_B) /* b */                                                         \
    M(C_LOW_C) /* c */                                                         \
    M(C_LOW_D) /* d */                                                         \
    M(C_LOW_E) /* e */                                                         \
    M(C_LOW_F) /* f */                                                         \
    M(C_LOW_L) /* l */                                                         \
    M(C_LOW_N) /* n */                                                         \
    M(C_LOW_R) /* r */                                                         \
    M(C_LOW_S) /* s */                                                         \
    M(C_LOW_T) /* t */                                                         \
    M(C_LOW_U) /* u */                                                         \
    M(C_ABCDF) /* ABCDF */                                                     \
    M(C_E)     /* E */                                                         \
    M(C_ETC)   /* everything else */                                           \
    M(NR_CLASSES)

#define EXPAND_CH_CLASS_ENUMS(_name) _name,
typedef enum { DEFINE_CHAR_CLASSES(EXPAND_CH_CLASS_ENUMS) } ch_class_t;

// The JSON parser uses a finite state machine with the following states.
// States before NR_STATES simply transition from one state the next with
// each character read.  States following (and with mixed-case symbol names)
// perform some action prior to transitioning to another state.
#define DEFINE_STATES(M)                                                       \
    M(VA, "VAlue: expect element or [ or {")                                   \
    M(AV, "Array Value: expect element or ]")                                  \
    M(AS, "Array Separator: expect , or ]")                                    \
    M(OK, "Object Key: expect string or }")                                    \
    M(OC, "Object Colon: expect :")                                            \
    M(OS, "Object Separator: expect , or }")                                   \
    M(ST, "string")                                                            \
    M(ES, "escape")                                                            \
    M(U1, "u1")                                                                \
    M(U2, "u2")                                                                \
    M(U3, "u3")                                                                \
    M(U4, "u4")                                                                \
    M(MI, "minus")                                                             \
    M(ZE, "zero")                                                              \
    M(IN, "integer")                                                           \
    M(FR, "fraction")                                                          \
    M(FS, "fraction")                                                          \
    M(E1, "e")                                                                 \
    M(E2, "ex")                                                                \
    M(E3, "exp")                                                               \
    M(T1, "tr")                                                                \
    M(T2, "tru")                                                               \
    M(T3, "true")                                                              \
    M(F1, "fa")                                                                \
    M(F2, "fal")                                                               \
    M(F3, "fals")                                                              \
    M(F4, "false")                                                             \
    M(N1, "nu")                                                                \
    M(N2, "nul")                                                               \
    M(N3, "null")                                                              \
    M(NR_STATES, "actions follow")                                             \
    M(Ba, "begin array")                                                       \
    M(Bd, "begin digit")                                                       \
    M(Bf, "begin false")                                                       \
    M(Bm, "begin minus")                                                       \
    M(Bn, "begin nUll")                                                        \
    M(Bo, "begin object")                                                      \
    M(Bs, "begin string")                                                      \
    M(Bt, "begin true")                                                        \
    M(Bz, "begin zero")                                                        \
    M(Fa, "finish array")                                                      \
    M(Fo, "finish object")                                                     \
    M(Pd, "process decimal point")                                             \
    M(Pl, "process colon")                                                     \
    M(Pm, "process comma")                                                     \
    M(Ps, "process trailing space")                                            \
    M(Pq, "process close quote")                                               \
    M(Px, "process exponent")

#define EXPAND_STATE_ENUMS(_name, _description) _name,
enum { DEFINE_STATES(EXPAND_STATE_ENUMS) };

typedef struct {
    mu_str_t *json;          // the JSON source string
    mu_json_token_t *tokens; // caller-supplied tokens store
    size_t max_tokens;       // number of tokens in token store
    int token_count;         // number of allocated tokens
    int depth;               // current parse tree depth
    int char_pos;            // position of char being parsed
    int state;               // parser state
    mu_json_err_t error;     // error status
} parser_t;

// *****************************************************************************
// Private (static) storage

/**
 * @brief Map an ASCII character to a corresponding character class.  This lets
 * us construct a much more compact state transition table (see below).
 */
// clang-format off
static int ascii_classes[128] = {
    __,      __,      __,      __,      __,      __,      __,      __,
    __,      C_WHITE, C_WHITE, __,      __,      C_WHITE, __,      __,
    __,      __,      __,      __,      __,      __,      __,      __,
    __,      __,      __,      __,      __,      __,      __,      __,

    C_SPACE, C_ETC,   C_QUOTE, C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_PLUS,  C_COMMA, C_MINUS, C_POINT, C_SLASH,
    C_ZERO,  C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT, C_DIGIT,
    C_DIGIT, C_DIGIT, C_COLON, C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,

    C_ETC,   C_ABCDF, C_ABCDF, C_ABCDF, C_ABCDF, C_E,     C_ABCDF, C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_LSQRB, C_BACKS, C_RSQRB, C_ETC,   C_ETC,

    C_ETC,   C_LOW_A, C_LOW_B, C_LOW_C, C_LOW_D, C_LOW_E, C_LOW_F, C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_ETC,   C_LOW_L, C_ETC,   C_LOW_N, C_ETC,
    C_ETC,   C_ETC,   C_LOW_R, C_LOW_S, C_LOW_T, C_LOW_U, C_ETC,   C_ETC,
    C_ETC,   C_ETC,   C_ETC,   C_LCURB, C_ETC,   C_RCURB, C_ETC,   C_ETC
};
// clang-format on

/**
 * @brief Map a [state, char_class] to a new state or action.
 *
 * This state transition table takes the current state and the current symbol,
 * and returns either a new state or an action. An action is signified by a
 * mixed-case symbol and has a value greater than NR_STATES.
 */
// clang-format off
static int state_transition_table[NR_STATES * NR_CLASSES] = {
/*             white                                      1-9                                   ABCDF  etc
           space |  {  }  [  ]  :  ,  "  \  /  +  -  .  0  |  a  b  c  d  e  f  l  n  r  s  t  u  |  E  |*/
/*value  VA*/ VA,VA,Bo,Fo,Ba,Fa,__,Pm,Bs,__,__,__,Bm,__,Bz,Bd,__,__,__,__,__,Bf,__,Bn,__,__,Bt,__,__,__,__,
/*ar val AV*/ AV,AV,Bo,__,Ba,Fa,__,__,Bs,__,__,__,Bm,__,Bz,Bd,__,__,__,__,__,Bf,__,Bn,__,__,Bt,__,__,__,__,
/*ar sep AS*/ AS,AS,__,__,__,Fa,__,Pm,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,
/*ob key OK*/ OK,OK,__,Fo,__,__,__,__,Bs,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,
/*ob cln OC*/ OC,OC,__,__,__,__,Pl,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,
/*ob sep OS*/ OS,OS,__,Fo,__,__,__,Pm,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,
/*string ST*/ ST,__,ST,ST,ST,ST,ST,ST,Pq,ES,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,ST,
/*escape ES*/ __,__,__,__,__,__,__,__,Bs,ST,ST,__,__,__,__,__,__,ST,__,__,__,ST,__,ST,ST,__,ST,U1,__,__,__,
/*u1     U1*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,U2,U2,U2,U2,U2,U2,U2,U2,__,__,__,__,__,__,U2,U2,__,
/*u2     U2*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,U3,U3,U3,U3,U3,U3,U3,U3,__,__,__,__,__,__,U3,U3,__,
/*u3     U3*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,U4,U4,U4,U4,U4,U4,U4,U4,__,__,__,__,__,__,U4,U4,__,
/*u4     U4*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,ST,ST,ST,ST,ST,ST,ST,ST,__,__,__,__,__,__,ST,ST,__,
/*minus  MI*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,ZE,IN,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,
/*zero   ZE*/ Ps,OK,__,Fo,__,Fa,__,Pm,__,__,__,__,__,Pd,__,__,__,__,__,__,Px,__,__,__,__,__,__,__,__,Px,__,
/*int    IN*/ Ps,Ps,__,Fo,__,Fa,__,Pm,__,__,__,__,__,Pd,IN,IN,__,__,__,__,Px,__,__,__,__,__,__,__,__,Px,__,
/*frac   FR*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,FS,FS,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,
/*fracs  FS*/ Ps,OK,__,Fo,__,Fa,__,Pm,__,__,__,__,__,__,FS,FS,__,__,__,__,Px,__,__,__,__,__,__,__,__,Px,__,
/*e      E1*/ __,__,__,__,__,__,__,__,__,__,__,E2,E2,__,E3,E3,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,
/*ex     E2*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,E3,E3,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,
/*exp    E3*/ Ps,OK,__,Fo,__,Fa,__,Pm,__,__,__,__,__,__,E3,E3,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,
/*tr     T1*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,T2,__,__,__,__,__,__,
/*tru    T2*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,T3,__,__,__,
/*true   T3*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,VA,__,__,__,__,__,__,__,__,__,__,
/*fa     F1*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,F2,__,__,__,__,__,__,__,__,__,__,__,__,__,__,
/*fal    F2*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,F3,__,__,__,__,__,__,__,__,
/*fals   F3*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,F4,__,__,__,__,__,
/*false  F4*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,VA,__,__,__,__,__,__,__,__,__,__,
/*nu     N1*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,N2,__,__,__,
/*nul    N2*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,N3,__,__,__,__,__,__,__,__,
/*null   N3*/ __,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,__,VA,__,__,__,__,__,__,__,__
};
// clang-format on

// *****************************************************************************
// start DEBUG_TRACE support
#ifdef DEBUG_TRACE

#define EXPAND_CH_CLASS_NAMES(_name) #_name,
__attribute__((unused)) static const char *s_ch_class_names[] = {
    DEFINE_CHAR_CLASSES(EXPAND_CH_CLASS_NAMES)};

#define N_CH_CLASSES sizeof(s_ch_class_names) / sizeof(s_ch_class_names[0])

#define EXPAND_STATE_NAMES(_name, _description) #_name,
__attribute__((unused)) static const char *s_state_names[] = {
    DEFINE_STATES(EXPAND_STATE_NAMES)};

#define EXPAND_STATE_DESCRIPTIONS(_name, _description) _description,
__attribute__((unused)) static const char *s_state_descriptions[] = {
    DEFINE_STATES(EXPAND_STATE_DESCRIPTIONS)};

#define N_STATES sizeof(s_state_names) / sizeof(s_state_names[0])

__attribute__((unused)) static const char *ch_class_name(ch_class_t ch_class) {
    if (ch_class >= 0 && ch_class < N_CH_CLASSES) {
        return s_ch_class_names[ch_class];
    } else {
        return "CH_UNK?";
    }
}

__attribute__((unused)) static const char *state_name(int state) {
    if (state >= 0 && state < N_STATES) {
        return s_state_names[state];
    } else {
        return "__";
    }
}

__attribute__((unused)) static const char *state_description(int state) {
    if (state >= 0 && state < N_STATES) {
        return s_state_descriptions[state];
    } else {
        return "unknown state";
    }
}

__attribute__((unused)) static const char *error_name(mu_json_err_t error) {
    if (error == MU_JSON_ERR_NONE) {
        return "MU_JSON_ERR_NONE";
    } else if (error == MU_JSON_ERR_BAD_FORMAT) {
        return "MU_JSON_ERR_BAD_FORMAT";
    } else if (error == MU_JSON_ERR_NO_TOKENS) {
        return "MU_JSON_ERR_NO_TOKENS";
    } else {
        return "UNKNOWN ERROR";
    }
}

#define EXPAND_TOKEN_TYPE_NAMES(_name) #_name,
__attribute__((unused)) static const char *s_token_type_names[] = {
    DEFINE_MU_JSON_TOKEN_TYPES(EXPAND_TOKEN_TYPE_NAMES)};
#define N_TOKEN_TYPES                                                          \
    (sizeof(s_token_type_names) / sizeof(s_token_type_names[0]))

__attribute__((unused)) static const char *
token_type_name(mu_json_token_type_t token_type) {
    if (token_type >= 0 && token_type < N_TOKEN_TYPES) {
        return s_token_type_names[token_type];
    } else {
        return "unkown token type";
    }
}

/**
 * @brief Return a string describing the token.
 *
 * WARNING 1: prints into a static buf that gets re-written on the next call.
 * WARNING 2: limits results to 100 bytes.
 */
static char *token_string(mu_json_token_t *token) {
    static char buf[100];

    if (token == NULL) {
        snprintf(buf, sizeof(buf), "<null token>");
    } else {
        mu_str_t *s = &token->json;
        snprintf(buf, sizeof(buf), "<token %p: %s %d '%.*s'>", token,
                 token_type_name(token->type), token->depth,
                 (int)mu_str_length(s), mu_str_buf(s));
    }
    buf[sizeof(buf) - 1] = '\0'; // just in case.
    return buf;
}

#endif
// end DEBUG_TRACE support
// *****************************************************************************

// *****************************************************************************
// Private (forward) declarations

static int parse(mu_json_token_t *tokens, size_t max_tokens,
                 mu_str_t *json_input);

/**
 * @brief Return the character class for the given character.
 */
static int classify_char(uint8_t ch);

/**
 * @brief Convenience interface to begin_token(): Allocates a token, sets its
 * type, and set the parser state to s.
 *
 * Rife with side effects, but makes for a condensed dispatch table.
 */
static void std_alloc(parser_t *parser, mu_json_token_type_t type, int s);

/**
 * @brief Initialize and add a token to the token list.
 *
 * Initialize the token using the current char_pos and depth from the parser.
 *
 * Return false if no tokens are avaialble.
 */
static bool begin_token(parser_t *parser, mu_json_token_type_t type);

/**
 * @brief Complete a token.
 *
 * Trim the token's JSON slice end at the current char_pos and seal it.  If
 * incl_delim is true, slice one char past the current char pos (e.g. for
 * close quote, close ] and close }).
 */
static void finish_token(parser_t *parser, mu_json_token_t *token,
                         bool incl_delim);

/**
 * @brief Map a (state, char_class) pair to a new state.
 */
static inline int lookup_state(int state, int char_class) {
    return state_transition_table[state * NR_CLASSES + char_class];
}

/**
 * @brief Return true if token is an OBJECT or ARRAY
 */
static inline bool is_container(mu_json_token_t *token) {
    return (token->type == MU_JSON_TOKEN_TYPE_ARRAY) ||
           (token->type == MU_JSON_TOKEN_TYPE_OBJECT);
}

/**
 * @brief Return true if this token is "sealed", that is, the end of the token
 * has been found.  For containers (like ARRAY and OBJECT) this means that no
 * more items will be added to it.
 */
static inline bool token_is_sealed(mu_json_token_t *token) {
    return token->flags & MU_JSON_TOKEN_FLAG_IS_SEALED;
}

/**
 * @brief Seal a token.
 */
static inline void seal_token(mu_json_token_t *token) {
    token->flags |= MU_JSON_TOKEN_FLAG_IS_SEALED;
}

/**
 * @brief Return true if this is the first token.
 */
static inline bool token_is_first(mu_json_token_t *token) {
    return token->flags & MU_JSON_TOKEN_FLAG_IS_FIRST;
}

/**
 * @brief Mark this token as being first.
 */
static inline void set_is_first(mu_json_token_t *token) {
    token->flags |= MU_JSON_TOKEN_FLAG_IS_FIRST;
}

/**
 * @brief Return true if this is the last token.
 */
static inline bool token_is_last(mu_json_token_t *token) {
    return token->flags & MU_JSON_TOKEN_FLAG_IS_LAST;
}

/**
 * @brief Mark this token as being last.
 */
static inline void set_is_last(mu_json_token_t *token) {
    token->flags |= MU_JSON_TOKEN_FLAG_IS_LAST;
}

/**
 * @brief "Top of Stack": Return the most recently allocated token or NULL if
 * none have been allocated.
 */
static mu_json_token_t *tos(parser_t *parser);

/**
 * @brief Search backwards through the token stack to find the container to
 * which new items will be added.  Return NULL if not inside a container.
 */
static mu_json_token_t *active_container(parser_t *parser);

/**
 * @brief Called when ] or } encountered.
 *
 * Close any open tos element, close the active container, reduce parser
 * depth by 1, set next state.
 */
static void close_container_aux(parser_t *parser, mu_json_token_type_t type);

/**
 * @brief return a new state depending on several factors.
 *
 * @param parser parser.
 * @param not_in_container State to be returned if token is not in a container
 * @param in_array State to be returned if token is inside an array
 * @param in_object_key State to be returned if token is an object key.
 * @param in_object_value State to be returned if token is an object value.
 * @return One of the above four values.
 */
static int select_state(parser_t *parser, int not_in_container,
                        int in_array, int in_object_key, int in_object_value);

/**
 * @brief Return the number of top-level children inside container, stopping
 * when token is encountered.
 *
 * IMPORTANT: container must be a parent of token.
 */
static int child_count(mu_json_token_t *container, mu_json_token_t *token);

/**
 * @brief Set the parser state.  If DEBUG_TRACE in effect, print transition.
 */
static inline void set_state(parser_t *parser, int state) {
    parser->state = state;
    TRACE_PRINTF(" => %s", state_name(parser->state));
}


// *****************************************************************************
// Public code

int mu_json_parse_c_str(mu_json_token_t *token_store, size_t max_tokens,
                        const char *json, void *arg) {
    (void)arg; // reserved for future use
    mu_str_t mu_str;
    return parse(token_store, max_tokens, mu_str_init_cstr(&mu_str, json));
}

int mu_json_parse_mu_str(mu_json_token_t *token_store, size_t max_tokens,
                         mu_str_t *mu_json, void *arg) {
    (void)arg; // reserved for future use
    return parse(token_store, max_tokens, mu_json);
}

int mu_json_parse_buffer(mu_json_token_t *token_store, size_t max_tokens,
                         const uint8_t *buf, size_t buflen, void *arg) {
    (void)arg; // reserved for future use
    mu_str_t mu_str;
    return parse(token_store, max_tokens, mu_str_init(&mu_str, buf, buflen));
}

mu_str_t *mu_json_token_slice(mu_json_token_t *token) {
    if (token == NULL) {
        return NULL;
    } else {
        return &token->json;
    }
}

mu_json_token_type_t mu_json_token_type(mu_json_token_t *token) {
    if (token == NULL) {
        return MU_JSON_TOKEN_TYPE_UNKNOWN;
    } else {
        return token->type;
    }
}

int mu_json_token_depth(mu_json_token_t *token) {
    if (token == NULL) {
        return -1;
    } else {
        return token->depth;
    }
}

bool mu_json_token_is_first(mu_json_token_t *token) {
    if (token == NULL) {
        // TODO: warn
        return false;
    } else {
        return token_is_first(token);
    }
}

bool mu_json_token_is_last(mu_json_token_t *token) {
    if (token == NULL) {
        // TODO: warn
        return false;
    } else {
        return token_is_last(token);
    }
}

mu_json_token_t *mu_json_token_prev(mu_json_token_t *token) {
    if (token == NULL) {
        return NULL;
    } else if (token_is_first(token)) {
        return NULL;
    } else {
        return &token[-1];
    }
}

mu_json_token_t *mu_json_token_next(mu_json_token_t *token) {
    if (token == NULL) {
        return NULL;
    }
    if (token_is_last(token)) {
        return NULL;
    } else {
        return &token[1];
    }
}

mu_json_token_t *mu_json_token_root(mu_json_token_t *token) {
    if (token == NULL) {
        return NULL;
    }
    while (!token_is_first(token)) {
        token = mu_json_token_prev(token);
    }
    return token;
}

mu_json_token_t *mu_json_token_parent(mu_json_token_t *token) {
    if (token == NULL) {
        return NULL;
    }
    // search backwards until a token with a shallower depth is found
    int depth = mu_json_token_depth(token);
    mu_json_token_t *prev = token;

    do {
        prev = mu_json_token_prev(prev);
    } while (prev != NULL && mu_json_token_depth(prev) >= depth);
    return prev;
}

mu_json_token_t *mu_json_token_child(mu_json_token_t *token) {
    if (token == NULL) {
        return NULL;
    }
    int depth = mu_json_token_depth(token);
    mu_json_token_t *next = mu_json_token_next(token);
    if (mu_json_token_depth(next) > depth) {
        return next;
    } else {
        return NULL;
    }
}

mu_json_token_t *mu_json_token_prev_sibling(mu_json_token_t *token) {
    if (token == NULL) {
        return NULL;
    }
    mu_json_token_t *prev = mu_json_token_prev(token);
    while (true) {
        if (prev == NULL) {
            return NULL;
        } else if (mu_json_token_depth(prev) < mu_json_token_depth(token)) {
            return NULL;
        } else if (mu_json_token_depth(prev) == mu_json_token_depth(token)) {
            return prev;
        } else {
            prev = mu_json_token_prev(prev);
        }
    }
}

mu_json_token_t *mu_json_token_next_sibling(mu_json_token_t *token) {
    if (token == NULL) {
        return NULL;
    }
    mu_json_token_t *next = mu_json_token_next(token);
    while (true) {
        if (next == NULL) {
            return NULL;
        } else if (mu_json_token_depth(next) < mu_json_token_depth(token)) {
            return NULL;
        } else if (mu_json_token_depth(next) == mu_json_token_depth(token)) {
            return next;
        } else {
            next = mu_json_token_next(next);
        }
    }
}

// *****************************************************************************
// Private (static) code

static int parse(mu_json_token_t *token_store, size_t max_tokens,
                 mu_str_t *json_input) {
    parser_t parser;
    parser.json = json_input;
    parser.tokens = token_store;
    parser.max_tokens = max_tokens;
    parser.token_count = 0;
    parser.depth = 0;
    parser.char_pos = 0;
    parser.state = VA;
    parser.error = MU_JSON_ERR_NONE;

    bool eos = false;
    uint8_t ch;
    int char_class;
    int next_state;

    TRACE_PRINTF("\n==== parsing '%.*s'", (int)mu_str_length(json_input),
                 mu_str_buf(json_input));

    while (!eos) {
        if (parser.error != MU_JSON_ERR_NONE) {
            // allocation or format error
            break;
        } else if (parser.state == __) {
            // parse error
            break;
        }
        eos = !mu_str_get_byte(parser.json, parser.char_pos, &ch);
        if (eos) {
            // treat end of string like a space delimiter: it simplififes the
            // endgame logic.
            char_class = C_SPACE;
        } else if ((char_class = classify_char(ch)) < 0) {
            // illegal character in json_input
            parser.error = MU_JSON_ERR_BAD_FORMAT;
            TRACE_PRINTF("\n'%c': illegal character", ch);
            break;
        }

        next_state = lookup_state(parser.state, char_class);

        TRACE_PRINTF("\n%d %d '%c': %s %s => %s", parser.token_count,
                     parser.depth, ch, ch_class_name(char_class),
                     state_name(parser.state), state_name(next_state));

        if (next_state >= 0 && next_state < NR_STATES) {
            // Simple state transition w/o special action
            set_state(&parser, next_state);

        } else {
            // These states perform an action before transitioning to next state
            switch (next_state) {

            // These actions cause tokens to be allocated.
            case Ba: {
                // [ - Begin Array
                std_alloc(&parser, MU_JSON_TOKEN_TYPE_ARRAY, AV);
                parser.depth += 1;
                break;
            }

            case Bd: {
                // Begin digit 1..9
                std_alloc(&parser, MU_JSON_TOKEN_TYPE_INTEGER, IN);
                break;
            }

            case Bf: {
                // Begin false
                std_alloc(&parser, MU_JSON_TOKEN_TYPE_FALSE, F1);
                break;
            }

            case Bm: {
                // Begin minus
                std_alloc(&parser, MU_JSON_TOKEN_TYPE_INTEGER, MI);
                break;
            }

            case Bn: {
                // begin null
                std_alloc(&parser, MU_JSON_TOKEN_TYPE_NULL, N1);
                break;
            }

            case Bo: {
                // begin object
                std_alloc(&parser, MU_JSON_TOKEN_TYPE_OBJECT, OK);
                parser.depth += 1;
                break;
            }

            case Bs: {
                // begin string
                std_alloc(&parser, MU_JSON_TOKEN_TYPE_STRING, ST);
                break;
            }

            case Bt: {
                // begin true
                std_alloc(&parser, MU_JSON_TOKEN_TYPE_TRUE, T1);
                break;
            }

            case Bz: {
                // Begin zero
                std_alloc(&parser, MU_JSON_TOKEN_TYPE_INTEGER, ZE);
                break;
            }

            // These actions cause tokens to be finished and/or state change.
            case Fa: {
                // ] (finish array)
                close_container_aux(&parser, MU_JSON_TOKEN_TYPE_ARRAY);
                break;
            }

            case Fo: {
                // } (finish object)
                close_container_aux(&parser, MU_JSON_TOKEN_TYPE_OBJECT);
                break;
            }

            case Pd: {
                // Process decimal point: convert INTEGER to NUMBER
                mu_json_token_t *token = tos(&parser);
                token->type = MU_JSON_TOKEN_TYPE_NUMBER;
                set_state(&parser, FR);
                break;
            }

            case Pl: {
                // Process colon
                mu_json_token_t *token = tos(&parser);
                finish_token(&parser, token, false);
                set_state(&parser, VA);
                break;
            }

            case Pm: {
                // Process comma
                mu_json_token_t *token = tos(&parser);
                TRACE_PRINTF("\nPm: token->depth=%d, parser->depth=%d",
                    token->depth, parser.depth);
                finish_token(&parser, token, false);
                set_state(&parser, select_state(&parser, __, AV, OC, OK));
                break;
            }

            case Ps: {
                // process trailing space
                mu_json_token_t *token = tos(&parser);
                if (!is_container(token)) {
                    finish_token(&parser, token, false);
                }
                set_state(&parser, select_state(&parser, VA, AS, OC, OS));
                break;
            }

            case Pq: {
                // Process closing quote:
                mu_json_token_t *token = tos(&parser);
                finish_token(&parser, token, true);
                set_state(&parser, select_state(&parser, VA, AS, OC, OS));
                break;
            }

            case Px: {
                // Process exponent: convert INTEGER to NUMBER
                mu_json_token_t *token = tos(&parser);
                token->type = MU_JSON_TOKEN_TYPE_NUMBER;
                set_state(&parser, E1);
                break;
            }

            default: {
                // Bad action.
                parser.error = MU_JSON_ERR_BAD_FORMAT;
                break;
            }

            } // switch(next_state)
        }

        // advance to next char
        parser.char_pos += 1;
    } // while(true)

    TRACE_PRINTF("\n=== endgame: depth=%d, state=%s, err=%d\n", parser.depth,
                 state_name(parser.state), parser.error);

    int retval;

    if (parser.error != MU_JSON_ERR_NONE) {
        TRACE_PRINTF("\nendgame: parse error");
        retval = parser.error;
    } else if (parser.depth != 0) {
        TRACE_PRINTF("\nendgame: non-zero depth");
        retval = MU_JSON_ERR_INCOMPLETE;
    } else if (parser.state != VA) {
        TRACE_PRINTF("\nendgame: final state != VA");
        retval = MU_JSON_ERR_BAD_FORMAT;
    } else {
        mu_json_token_t *token = tos(&parser);
        if (token) {
            set_is_last(tos(&parser)); // mark last token as such
            finish_token(&parser, mu_json_token_root(tos(&parser)), false);
        }
        TRACE_PRINTF("\nendgame: success");
        retval = parser.token_count;
    }
    TRACE_PRINTF("...returning %d\n", retval);
    return retval;
}

static int classify_char(uint8_t ch) {
    if (ch >= sizeof(ascii_classes) / sizeof(ascii_classes[0])) {
        return C_ETC;
    } else {
        return ascii_classes[ch];
    }
}

static void std_alloc(parser_t *parser, mu_json_token_type_t type, int s) {
    if (!begin_token(parser, type)) {
        parser->error = MU_JSON_ERR_NO_TOKENS;
    } else {
        parser->state = s;
        TRACE_PRINTF(" => %s", state_name(s));
    }
}

static bool begin_token(parser_t *parser, mu_json_token_type_t type) {
    if (parser->token_count >= parser->max_tokens) {
        return false;
    }
    mu_json_token_t *token = &parser->tokens[parser->token_count++];
    memset(token, 0, sizeof(mu_json_token_t));
    // Since we haven't parsed to the end of this token yet, initialize the
    // token's string to start at char_pos and extend to the end of the input
    // string.  This will get adjusted in a call to finish_token() [q.v.].
    mu_str_slice(&token->json, parser->json, parser->char_pos, MU_STR_END);
    token->type = type;
    if (parser->token_count == 1) {
        set_is_first(token);
    }
    token->flags |= parser->token_count == 1 ? MU_JSON_TOKEN_FLAG_IS_FIRST : 0;
    token->depth = parser->depth;
    // TRACE_PRINTF("\nStart %s", token_string(token));
    return true;
}

static void finish_token(parser_t *parser, mu_json_token_t *token,
                         bool incl_delim) {
    // TODO: Add mu_json_token_type_t arg to confim we're closing the right one
    if (token == NULL) {
        TRACE_PRINTF("\nAttempt to finish null token?");
        return;
    } else if (token_is_sealed(token)) {
        // already finished...
        return;
    }
    // On entry, token->json extends from the token start to the end of
    // the input string.  If incl_delim is true, slice it to end at
    // parser->char_pos + 1, else at parser->char_pos.
    //
    // How it works:
    // start_index is the index of the start of the token's string **within the
    // original input string**.  Re-slice to start at start_index and end at
    // end_index.
    int start_index = mu_str_length(parser->json) - mu_str_length(&token->json);
    int end_index = incl_delim ? parser->char_pos + 1 : parser->char_pos;
    mu_str_slice(&token->json, parser->json, start_index, end_index);
    TRACE_PRINTF("\nFinish %s", token_string(token));
    seal_token(token);
}

static mu_json_token_t *tos(parser_t *parser) {
    if (parser->token_count == 0) {
        return NULL;
    } else {
        return &parser->tokens[parser->token_count - 1];
    }
}

static mu_json_token_t *active_container(parser_t *parser) {
    // Search back through the token stack to find the container currently being
    // added to.  It will be at parser->depth - 1.
    mu_json_token_t *container = NULL;
    for (int i = parser->token_count - 1; i >= 0; --i) {
        container = &parser->tokens[i];
        if (container->depth == parser->depth-1) {
            return container;
        }
    }
    return NULL;
}

static void close_container_aux(parser_t *parser, mu_json_token_type_t type) {
    // here when ] or } seen
    mu_json_token_t *token = tos(parser);

    if (mu_json_token_type(token) != type) {
        // the closing ] or } may have terminated a token in progress.  Finish
        // it.  (If token was already finished, this is a no-op.)
        finish_token(parser, token, false);
    }
    mu_json_token_t *container = active_container(parser);
    if (container == NULL) {
        // not in a container
        parser->error = MU_JSON_ERR_BAD_FORMAT;
    } else if (container->type == type) {
        // wrong type of container
        parser->error = MU_JSON_ERR_BAD_FORMAT;
    } else {
        // success: close container, decrease depth.
        finish_token(parser, container, true);
        parser->depth -= 1;
        set_state(parser, select_state(container, VA, AS, OC, OS));
    }
}

static int select_state(parser_t *parser, int not_in_container,
                        int in_array, int in_object_key, int in_object_value) {
    mu_json_token_t *container = active_container(parser);

    TRACE_PRINTF("\nselect_state(): ");
    if (token == NULL) {
        return __;
    } else if (container == NULL) {
        TRACE_PRINTF("not in container");
        return not_in_container;
    } else if (container->type == MU_JSON_TOKEN_TYPE_ARRAY) {
        TRACE_PRINTF("in array");
        return in_array;
    } else if ((child_count(container, token) & 1) == 1) {
        // in object with odd count: just processed a key.  expect : (or value)
        TRACE_PRINTF("object key");
        return in_object_key;
    } else {
        // in object with even count (or zero): expect a string key
        TRACE_PRINTF("object value");
        return in_object_value;
    }
}

static int child_count(mu_json_token_t *container, mu_json_token_t *token) {
    // Assumes that token is a direct child of an active container.  Count
    // children of the container until token is seen
    if (container == token) {
        // handle the case where 0 children have been added to container.
        return 0;
    }
    int count = 1;
    mu_json_token_t *child = mu_json_token_child(container);
    while (child != token) {
        count += 1;
        child = mu_json_token_next_sibling(child);
        if (child == NULL) {
            // shouldn't happen
            TRACE_PRINTF("\n%s is not a child of container?",
                         token_string(token));
            return 0;
        }
    }
    return count;
}

