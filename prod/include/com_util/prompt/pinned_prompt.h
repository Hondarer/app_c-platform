/**
 *******************************************************************************
 *  @file           pinned_prompt.h
 *  @brief          Pinned prompt API for command-oriented CLIs.
 *  @author         Tetsuo Honda
 *  @date           2026/05/08
 *  @version        0.1.0
 *
 *  This API keeps a single-line prompt at the bottom of the terminal and writes
 *  application output above it.  The API is experimental and may change while
 *  the command-line interaction model is being refined.
 *
 *  @copyright      Copyright (C) Tetsuo Honda. 2026. All rights reserved.
 *
 *******************************************************************************
 */

#ifndef COM_UTIL_PINNED_PROMPT_H
#define COM_UTIL_PINNED_PROMPT_H

#include <stddef.h>

#include <com_util/base/compiler.h>
#include <com_util/base/platform.h>
#include <com_util/prompt/prompt.h>
#include <com_util/com_util_export.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     *  @brief  Pinned prompt handle.
     */
    typedef struct com_util_pinned_prompt_t com_util_pinned_prompt_t;

    /**
     *  @brief  Output channel used by com_util_pinned_prompt_write().
     */
    typedef enum
    {
        COM_UTIL_PINNED_PROMPT_CHANNEL_STDOUT = 0,
        COM_UTIL_PINNED_PROMPT_CHANNEL_STDERR = 1
    } com_util_pinned_prompt_channel_t;

    /**
     *  @brief  Status area position.
     */
    typedef enum
    {
        COM_UTIL_PINNED_PROMPT_STATUS_POSITION_TOP = 0,
        COM_UTIL_PINNED_PROMPT_STATUS_POSITION_BOTTOM = 1
    } com_util_pinned_prompt_status_position_t;

    /**
     *  @brief  Status area alignment.
     */
    typedef enum
    {
        COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_LEFT = 0,
        COM_UTIL_PINNED_PROMPT_STATUS_ALIGN_RIGHT = 1
    } com_util_pinned_prompt_status_align_t;

    /**
     *  @brief  Pinned prompt creation options.
     */
    typedef struct
    {
        /**
         *  @brief  Reserved for future flags. Set to 0.
         */
        unsigned int flags;

        /**
         *  @brief  Reserved for structure alignment. Set to 0.
         */
        unsigned int reserved;

        /**
         *  @brief  Input and history options.
         */
        com_util_prompt_options_t input;
    } com_util_pinned_prompt_options_t;

    /**
     *  @brief      Create a pinned prompt.
     *  @param[in]  options  Creation options. NULL uses default options.
     *  @return     Non-NULL handle on success, NULL on failure.
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部に共有状態を持ちません。各呼び出しは独立したハンドルを生成します。
     */
    COM_UTIL_EXPORT com_util_pinned_prompt_t *COM_UTIL_API
    com_util_pinned_prompt_create(const com_util_pinned_prompt_options_t *options);

    /**
     *  @brief      Dispose a pinned prompt.
     *  @param[in]      screen  Handle returned by com_util_pinned_prompt_create(). NULL is allowed.
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  解放対象の @p screen を他スレッドが使用していないことを呼び出し側で保証してください。
     */
    COM_UTIL_EXPORT void COM_UTIL_API com_util_pinned_prompt_dispose(com_util_pinned_prompt_t *screen);

/**
 *  @brief      Read one command line with a bottom-fixed prompt.
 *  @param[in]      screen      Pinned prompt handle.
 *  @param[out]     buf         Destination buffer. The terminating newline is not included.
 *  @param[in]      buf_size    Destination buffer size.
 *  @param[in]      prompt_str  Prompt string. NULL is treated as an empty string.
 *  @return     1 when a line is accepted, 0 on EOF, Ctrl+C, or invalid arguments.
 */
#define com_util_pinned_prompt_readline(screen, buf, buf_size, prompt_str) \
    _com_util_pinned_prompt_readline((screen), (buf), (buf_size), (prompt_str), __FILE__, __LINE__)

/**
 *  @brief      Read one command line with a formatted bottom-fixed prompt.
 *  @param[in]      screen    Pinned prompt handle.
 *  @param[out]     buf       Destination buffer. The terminating newline is not included.
 *  @param[in]      buf_size  Destination buffer size.
 *  @param[in]      fmt       printf style format string. NULL is treated as an empty string.
 *  @param[in]      ...       Format arguments.
 *  @return     1 when a line is accepted, 0 on EOF, Ctrl+C, or invalid arguments.
 */
#define com_util_pinned_prompt_readline_fmt(screen, buf, buf_size, fmt, ...) \
    _com_util_pinned_prompt_readline_fmt((screen), (buf), (buf_size), __FILE__, __LINE__, (fmt), ##__VA_ARGS__)

    /**
     *  @brief  com_util_pinned_prompt_readline() implementation.
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフではありません。\n
     *  同一 @p screen への並行呼び出しは未定義動作です。入力は 1 スレッドから行ってください。
     */
    COM_UTIL_EXPORT int COM_UTIL_API _com_util_pinned_prompt_readline(com_util_pinned_prompt_t *screen, char *buf,
                                                                      size_t buf_size, const char *prompt_str,
                                                                      const char *file, int line);

    /**
     *  @brief  com_util_pinned_prompt_readline_fmt() implementation.
     */
    COM_UTIL_EXPORT int COM_UTIL_API _com_util_pinned_prompt_readline_fmt(com_util_pinned_prompt_t *screen, char *buf,
                                                                          size_t buf_size, const char *file, int line,
                                                                          const char *fmt, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 6, 7)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          Write output above the bottom-fixed prompt.
     *  @param[in]      screen   Pinned prompt handle.
     *  @param[in]      channel  Output channel.
     *  @param[in]      data     Data to write. NULL is allowed only when size is 0.
     *  @param[in]      size     Data size in bytes.
     *  @note       ANSI CSI SGR escape sequences are passed through for coloring.
     *  @return     Number of bytes written to the target stream.
     *
     *  The function writes exactly the supplied bytes and does not add a newline.
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部のミューテックスで保護されており、同一 @p screen に対して複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT size_t COM_UTIL_API com_util_pinned_prompt_write(com_util_pinned_prompt_t *screen,
                                                                     com_util_pinned_prompt_channel_t channel,
                                                                     const void *data, size_t size);

    /**
     *  @brief          Write formatted output above the bottom-fixed prompt.
     *  @param[in]      screen   Pinned prompt handle.
     *  @param[in]      channel  Output channel.
     *  @param[in]      fmt      printf style format string. NULL is treated as an empty string.
     *  @param[in]      ...      Format arguments.
     *  @note       ANSI CSI SGR escape sequences are passed through for coloring.
     *  @return     Number of bytes written to the target stream.
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_pinned_prompt_printf(com_util_pinned_prompt_t *screen,
                                                                   com_util_pinned_prompt_channel_t channel,
                                                                   const char *fmt, ...)
#if defined(COMPILER_GCC)
        __attribute__((format(printf, 3, 4)))
#endif /* COMPILER_GCC */
        ;

    /**
     *  @brief          Enable or disable status area.
     *  @param[in]      screen    Pinned prompt handle.
     *  @param[in]      position  Status area position (top or bottom).
     *  @param[in]      enable    Non-zero to enable, zero to disable.
     *  @return     0 on success, -1 on failure.
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部のミューテックスで保護されており、同一 @p screen に対して複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_pinned_prompt_status_enable(
        com_util_pinned_prompt_t *screen, com_util_pinned_prompt_status_position_t position, int enable);

    /**
     *  @brief          Set status area content.
     *  @param[in]      screen    Pinned prompt handle.
     *  @param[in]      position  Status area position (top or bottom).
     *  @param[in]      align     Alignment (left or right).
     *  @param[in]      content   Content string. NULL clears the content.
     *  @note       ANSI CSI SGR escape sequences are passed through for coloring and
     *              counted as display width 0 for status layout.
     *  @return     0 on success, -1 on failure.
     *
     *  @par            スレッド セーフ
     *  本関数はスレッド セーフです。\n
     *  内部のミューテックスで保護されており、同一 @p screen に対して複数スレッドから同時に呼び出せます。
     */
    COM_UTIL_EXPORT int COM_UTIL_API com_util_pinned_prompt_status_set(
        com_util_pinned_prompt_t *screen, com_util_pinned_prompt_status_position_t position,
        com_util_pinned_prompt_status_align_t align, const char *content);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* COM_UTIL_PINNED_PROMPT_H */
