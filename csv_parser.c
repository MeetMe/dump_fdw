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
#include <assert.h>
#include <stdlib.h>
#include <csv_parser.h>


void csv_parser_init(csv_parser_t *parser)
{
    assert(parser);
    parser->state = csvps_line_start;
    parser->row = -1;
    parser->col = -1;
    parser->nread = 0;
    parser->data = NULL;
}


size_t csv_parser_execute(csv_parser_t *parser,
                          const csv_parser_settings_t *settings,
                          const char *data,
                          size_t data_len)
{
    assert(parser);
    assert(settings);
    assert(data);

    const char *cursor = data;
    const char *field_value = NULL;
    const char *data_end = data + data_len;
    int r;
    parser->nread = 0;

    if (data_len < 1) {
        return 0;
    }

    while (cursor < data_end) {
        char ch = *cursor;
        switch (parser->state) {
            case csvps_line_start:
                field_value = NULL;
                parser->row += 1;
                parser->col = -1;
                if (ch == '\r' || ch == '\n') {
                    parser->state = csvps_line_end_begin;
                } else {
                    parser->state = csvps_field_start;
                }
                break;
            case csvps_field_start:
                parser->col += 1;
                parser->state = csvps_field_value;
                field_value = cursor;
                break;
            case csvps_field_value:
                if (ch == settings->delimiter) {
                    parser->state = csvps_field_end;
                } else if (ch == '\r' || ch == '\n') {
                    parser->state = csvps_line_end_begin;
                } else {
                    // If we previously been in csvps_field_value state
                    // and field_value is not set, set it right away.
                    // This can happen when we execute parser multiple
                    // times in the value state.
                    if (field_value == NULL) {
                        field_value = cursor;
                    }

                    cursor++;
                    if (cursor == data_end) {
                        if (settings->field_cb && field_value) {
                            r = settings->field_cb(parser,
                                                   field_value,
                                                   cursor - field_value,
                                                   parser->row,
                                                   parser->col);
                            if (r) {
                                parser->state = csvps_error;
                                return parser->nread;
                            }
                        }
                    }
                    parser->nread++;
                }
                break;
            case csvps_field_end:
                // callback
                if (settings->field_cb && field_value) {
                    r = settings->field_cb(parser,
                                           field_value,
                                           cursor - field_value,
                                           parser->row,
                                           parser->col);
                    if (r) {
                        parser->state = csvps_error;
                        return parser->nread;
                    }
                }

                parser->state = csvps_field_start;
                cursor++;
                parser->nread++;
                break;
            case csvps_line_end_begin:
                // callback
                if (settings->field_cb && field_value) {
                    r = settings->field_cb(parser,
                                           field_value,
                                           cursor - field_value,
                                           parser->row,
                                           parser->col);
                    if (r) {
                        parser->state = csvps_error;
                        return parser->nread;
                    }
                }
                parser->state = csvps_line_end;
                break;
            case csvps_line_end:
                if (ch == '\r' || ch == '\n') {
                    cursor++;
                    parser->nread++;
                } else {
                    parser->state = csvps_line_start;
                }
                break;
            case csvps_error:
                return parser->nread;
                break;
            default:
                assert(0 && "invalid parser state");
                break;
        }
    }

    return parser->nread;
}
