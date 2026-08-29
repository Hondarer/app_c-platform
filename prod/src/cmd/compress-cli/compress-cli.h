#ifndef COMPRESS_CLI_PRIVATE_H
#define COMPRESS_CLI_PRIVATE_H

typedef enum compress_cli_mode
{
    COMPRESS_CLI_MODE_NONE = 0,
    COMPRESS_CLI_MODE_COMPRESS,
    COMPRESS_CLI_MODE_DECOMPRESS
} compress_cli_mode;

typedef struct compress_cli_options
{
    compress_cli_mode mode;
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

#endif /* COMPRESS_CLI_PRIVATE_H */
