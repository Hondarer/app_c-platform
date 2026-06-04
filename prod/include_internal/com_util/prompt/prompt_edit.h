/**
 *  @file           prompt_edit.h
 *  @brief          Prompt edit buffer helper internals.
 *
 *  @hideincludedbygraph
 *
 */

/* NOTE: このヘッダーは多数のソース ファイルから参照されるため、            */
/*       @hideincludedbygraph によって "Included by" グラフを無効にします。 */

#ifndef COM_UTIL_PROMPT_EDIT_H
#define COM_UTIL_PROMPT_EDIT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t com_util_prompt_edit_utf8_prev_boundary(const char *buf, size_t pos);
size_t com_util_prompt_edit_utf8_next_boundary(const char *buf, size_t len, size_t pos);
size_t com_util_prompt_edit_utf8_sanitize_boundary(const char *buf, size_t len, size_t pos);

int com_util_prompt_edit_ensure_capacity(char **buf, size_t *cap, size_t max_bytes, size_t required);

void com_util_prompt_edit_resolve_options(size_t requested_history_max,
                                          size_t requested_initial_capacity,
                                          size_t requested_max_bytes,
                                          size_t initial_capacity_default,
                                          size_t *history_max,
                                          size_t *initial_capacity,
                                          size_t *max_bytes);

#ifdef __cplusplus
}
#endif

#endif /* COM_UTIL_PROMPT_EDIT_H */
