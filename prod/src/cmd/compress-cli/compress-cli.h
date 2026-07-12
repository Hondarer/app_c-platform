#ifndef COM_UTIL_COMPRESS_CLI_H
#define COM_UTIL_COMPRESS_CLI_H

typedef enum compress_cli_mode_t
{
    COMPRESS_CLI_MODE_NONE = 0,
    COMPRESS_CLI_MODE_COMPRESS,
    COMPRESS_CLI_MODE_DECOMPRESS
} compress_cli_mode_t;

typedef struct compress_cli_options
{
    compress_cli_mode_t mode;
    int need_help;
    const char *input_path;
    const char *output_path;
} compress_cli_options;

#ifdef __cplusplus
extern "C"
{
#endif

    void compress_cli_options_init(compress_cli_options *options);

#ifdef __cplusplus
}
#endif

#endif /* COM_UTIL_COMPRESS_CLI_H */
