/**
 * Copyright (C) 2013 Tadas Vilkeliskis <vilkeliskis.t@gmail.com>
 *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef CSV_PARSER_INCLUDE_GUARD_E95A7C4D_BE4A_49D1_8607_900F36DD25B8
#define CSV_PARSER_INCLUDE_GUARD_E95A7C4D_BE4A_49D1_8607_900F36DD25B8

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <sys/types.h>


typedef enum csv_parser_state_t {
    csvps_line_start = 0,
    csvps_field_start,
    csvps_field_value,
    csvps_field_end,
    csvps_line_end_begin,
    csvps_line_end,
    csvps_error,
} csv_parser_state_t;


typedef struct csv_parser_t {
    csv_parser_state_t state;
    /* csv row */
    int row;
    /* csv column */
    int col;
    uint32_t nread;
    /* user data */
    void *data;
} csv_parser_t;


/**
 * Field callback interface.
 *
 * Field callback can be called multiple times for the same row and column.
 * It's up to the user to construct full field value.
 *
 * Return 0 value on success, anything else on error.
 */
typedef int (*csv_parser_field_calback_t)(csv_parser_t *parser,
                                          const char *data,
                                          size_t length,
                                          int row,
                                          int col);


typedef struct csv_parser_settings_t {
    char delimiter;
    csv_parser_field_calback_t field_cb;
} csv_parser_settings_t;


void csv_parser_init(csv_parser_t *parser);

/**
 * Executes CSV parser on the given data. Returns number
 * of bytes read. User should always check if parser state
 * is ``csvps_error``. Parser will get into the error state
 * if field callback returns a non-zero value.
 */
size_t csv_parser_execute(csv_parser_t *parser,
                          const csv_parser_settings_t *settings,
                          const char *data,
                          size_t data_len);


#ifdef __cplusplus
}
#endif
#endif /* end of include guard */
