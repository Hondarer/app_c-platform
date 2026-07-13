#include <testfw.h>
#include <mock_stdio.h>
#include <mock_com_util.h>
#include "compress-cli.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>

using testing::_;
using testing::DoAll;
using testing::HasSubstr;
using testing::NiceMock;
using testing::Return;
using testing::SetArgPointee;
using testing::StrEq;

namespace
{

const uint8_t kPlainInput[] = {'a', 'b', 'c', 'd', 'e', 'f'};
const uint8_t kCompressedPayload[] = {0x00, 0x00, 0x00, 0x06, 0x78, 0x9c, 0x4b};
const uint8_t kDecompressedOutput[] = {'A', 'B', 'C', 'D', 'E', 'F'};

static int return_full_path(char *path_out, size_t path_size, int *errno_out, const char *text)
{
    size_t len;

    (void)errno_out;
    if (path_out == nullptr || text == nullptr)
    {
        return -1;
    }

    len = std::strlen(text);
    if (len + 1u > path_size)
    {
        return -1;
    }

    std::memcpy(path_out, text, len + 1u);
    return 0;
}

} // namespace

class compress_cliTest : public Test
{
  protected:
    NiceMock<Mock_stdio> mock_stdio_;
    NiceMock<Mock_com_util> mock_com_util_;
};

// 未対応オプション指定時に main() が失敗終了することの確認
TEST_F(compress_cliTest, main_rejects_unknown_option)
{
    // Arrange
    const char *argv[] = {
        "compress-cli", "--unknown", "input.bin",
        "output.bin"}; // [状態] - main() に与える引数を未対応オプション "--unknown" を含む 4 つとする。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_console_init())
        .WillOnce(
            Return()); // [Pre-Assert確認_正常系] - main() 呼び出し時に com_util_console_init が 1 回呼び出されること。

    // Act
    int rc = __real_main(4, (char **)&argv); // [手順] - main() に引数を与えて呼び出す。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, rc); // [確認_異常系] - 戻り値が EXIT_SUCCESS 以外であること。
}

// help オプション指定時に usage を表示して正常終了することの確認
TEST_F(compress_cliTest, main_prints_usage_on_help)
{
    // Arrange
    const char *argv[] = {"compress-cli", "--help"}; // [状態] - main() に与える引数を "--help" のみとする。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_console_init())
        .WillOnce(
            Return()); // [Pre-Assert確認_正常系] - main() 呼び出し時に com_util_console_init が 1 回呼び出されること。

    // Act
    int rc = __real_main(2, (char **)&argv); // [手順] - main() に引数を与えて呼び出す。

    // Assert
    EXPECT_EQ(EXIT_SUCCESS, rc); // [確認_正常系] - 必須位置引数がなくても戻り値が EXIT_SUCCESS であること。
}

// 正規化後に同一となる入出力パスが拒否されることの確認
TEST_F(compress_cliTest, main_rejects_same_normalized_path)
{
    // Arrange
    int argc = 4;
    const char *argv[] = {
        "compress-cli", "--compress", "./data.bin",
        "subdir/../data.bin"}; // [状態] - 入出力パスを正規化すると同一になる "./data.bin" と "subdir/../data.bin" とする。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_console_init())
        .WillOnce(
            Return()); // [Pre-Assert確認_正常系] - main() 呼び出し時に com_util_console_init が 1 回呼び出されること。
    EXPECT_CALL(mock_com_util_, com_util_paths_equal(StrEq("./data.bin"), StrEq("subdir/../data.bin"), _))
        .WillOnce(Return(1)); // [Pre-Assert確認_異常系] - com_util_paths_equal で入出力パスの比較が行われること。
                              // [Pre-Assert手順] - com_util_paths_equal から 1 (同一) を返却する。
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - 同一パス検出時は com_util_path_get_full が呼び出されないこと。
    EXPECT_CALL(mock_com_util_, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - 同一パス検出時は com_util_fopen が呼び出されないこと。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("同じパス")))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - "同じパス" を含むエラーが表示されること。

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - main() に引数を与えて呼び出す。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, rc); // [確認_異常系] - 戻り値が EXIT_SUCCESS 以外であること。
}

// 圧縮モードで入力が読み込まれ圧縮結果が出力ファイルへ書き込まれることの確認
TEST_F(compress_cliTest, main_compresses_input_and_writes_output)
{
    // Arrange
    int argc = 4;
    const char *argv[] = {"compress-cli", "--compress", "input.bin",
                          "output.bin"}; // [状態] - 圧縮モードで入力 "input.bin"、出力 "output.bin" とする。
    FILE *input_file = (FILE *)(uintptr_t)0x1010;
    FILE *output_file = (FILE *)(uintptr_t)0x2020;

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_console_init())
        .WillOnce(
            Return()); // [Pre-Assert確認_正常系] - main() 呼び出し時に com_util_console_init が 1 回呼び出されること。
    EXPECT_CALL(mock_com_util_, com_util_paths_equal(StrEq("input.bin"), StrEq("output.bin"), _))
        .WillOnce(Return(0)); // [Pre-Assert手順] - com_util_paths_equal から 0 (不一致) を返却する。
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("input.bin")))
        .WillOnce(
            [](char *path_out, size_t path_size, int *errno_out, const char *)
            {
                return return_full_path(path_out, path_size, errno_out, "/tmp/input.bin");
            }); // [Pre-Assert手順] - 入力パスの正規化結果として "/tmp/input.bin" を返却する。
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("output.bin")))
        .WillOnce(
            [](char *path_out, size_t path_size, int *errno_out, const char *)
            {
                return return_full_path(path_out, path_size, errno_out, "/tmp/output.bin");
            }); // [Pre-Assert手順] - 出力パスの正規化結果として "/tmp/output.bin" を返却する。
    EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq("/tmp/input.bin"), StrEq("rb"), _))
        .WillOnce(Return(input_file)); // [Pre-Assert確認_正常系] - 入力ファイルがモード "rb" で開かれること。
    EXPECT_CALL(mock_com_util_, com_util_fseek(input_file, 0, SEEK_END)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_ftell(input_file)).WillOnce(Return((int64_t)sizeof(kPlainInput)));
    EXPECT_CALL(mock_com_util_, com_util_fseek(input_file, 0, SEEK_SET)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_fread(_, 1u, sizeof(kPlainInput), input_file))
        .WillOnce(
            [](void *ptr, size_t, size_t count, FILE *)
            {
                std::memcpy(ptr, kPlainInput, sizeof(kPlainInput));
                return count;
            }); // [Pre-Assert手順] - 入力ファイルの読み込みで平文データ 6 byte を返却する。
    EXPECT_CALL(mock_com_util_, com_util_fclose(input_file)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_compress(_, _, _, sizeof(kPlainInput)))
        .WillOnce(
            [](uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t)
            {
                EXPECT_EQ(
                    0,
                    std::memcmp(
                        src, kPlainInput,
                        sizeof(
                            kPlainInput))); // [Pre-Assert確認_正常系] - 圧縮 API に読み込んだ平文データがそのまま渡ること。
                EXPECT_GE(*dst_len, sizeof(kCompressedPayload));
                std::memcpy(dst, kCompressedPayload, sizeof(kCompressedPayload));
                *dst_len = sizeof(kCompressedPayload);
                return 0;
            }); // [Pre-Assert確認_正常系] - com_util_compress が入力サイズ 6 byte で 1 回呼び出されること。
                // [Pre-Assert手順] - com_util_compress から圧縮済みデータ 7 byte を返却する。
    EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq("/tmp/output.bin"), StrEq("wb"), _))
        .WillOnce(Return(output_file)); // [Pre-Assert確認_正常系] - 出力ファイルがモード "wb" で開かれること。
    EXPECT_CALL(mock_com_util_, com_util_fwrite(_, 1u, sizeof(kCompressedPayload), output_file))
        .WillOnce(
            [](const void *ptr, size_t, size_t count, FILE *)
            {
                EXPECT_EQ(
                    0,
                    std::memcmp(
                        ptr, kCompressedPayload,
                        sizeof(
                            kCompressedPayload))); // [Pre-Assert確認_正常系] - 出力ファイルへ圧縮済みデータが書き込まれること。
                return count;
            });
    EXPECT_CALL(mock_com_util_, com_util_fclose(output_file)).WillOnce(Return(0));

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - main() に引数を与えて呼び出す。

    // Assert
    EXPECT_EQ(EXIT_SUCCESS, rc); // [確認_正常系] - 戻り値が EXIT_SUCCESS であること。
}

// 展開時にヘッダーの元サイズが小さすぎる入力が拒否されることの確認
TEST_F(compress_cliTest, main_rejects_decompress_input_when_header_size_is_too_small)
{
    // Arrange
    int argc = 4;
    const char *argv[] = {"compress-cli", "--decompress", "input.bin",
                          "output.bin"}; // [状態] - 展開モードで入力 "input.bin"、出力 "output.bin" とする。
    FILE *input_file = (FILE *)(uintptr_t)0x3030;
    const uint8_t invalid_input[] = {
        0x00, 0x00, 0x00, 0x04, 0x10}; // [状態] - ヘッダーの元サイズが 4 byte と小さすぎる不正入力 5 byte を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_console_init()).WillOnce(Return());
    EXPECT_CALL(mock_com_util_, com_util_paths_equal(StrEq("input.bin"), StrEq("output.bin"), _)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("input.bin")))
        .WillOnce([](char *path_out, size_t path_size, int *errno_out, const char *)
                  { return return_full_path(path_out, path_size, errno_out, "/tmp/input.bin"); });
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("output.bin")))
        .WillOnce([](char *path_out, size_t path_size, int *errno_out, const char *)
                  { return return_full_path(path_out, path_size, errno_out, "/tmp/output.bin"); });
    EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq("/tmp/input.bin"), StrEq("rb"), _)).WillOnce(Return(input_file));
    EXPECT_CALL(mock_com_util_, com_util_fseek(input_file, 0, SEEK_END)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_ftell(input_file)).WillOnce(Return((int64_t)sizeof(invalid_input)));
    EXPECT_CALL(mock_com_util_, com_util_fseek(input_file, 0, SEEK_SET)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_fread(_, 1u, sizeof(invalid_input), input_file))
        .WillOnce(
            [&](void *ptr, size_t, size_t count, FILE *)
            {
                std::memcpy(ptr, invalid_input, sizeof(invalid_input));
                return count;
            }); // [Pre-Assert手順] - 入力ファイルの読み込みで不正入力 5 byte を返却する。
    EXPECT_CALL(mock_com_util_, com_util_fclose(input_file)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_decompress(_, _, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - ヘッダー不正時は com_util_decompress が呼び出されないこと。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("ヘッダの元サイズ")))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - "ヘッダの元サイズ" を含むエラーが表示されること。

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - main() に引数を与えて呼び出す。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, rc); // [確認_異常系] - 戻り値が EXIT_SUCCESS 以外であること。
}

// 展開後サイズがヘッダー値と一致しない場合に出力せず失敗終了することの確認
TEST_F(compress_cliTest, main_rejects_decompress_output_when_size_mismatches_header)
{
    // Arrange
    int argc = 4;
    const char *argv[] = {"compress-cli", "--decompress", "input.bin",
                          "output.bin"}; // [状態] - 展開モードで入力 "input.bin"、出力 "output.bin" とする。
    FILE *input_file = (FILE *)(uintptr_t)0x4040;

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_console_init()).WillOnce(Return());
    EXPECT_CALL(mock_com_util_, com_util_paths_equal(StrEq("input.bin"), StrEq("output.bin"), _)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("input.bin")))
        .WillOnce([](char *path_out, size_t path_size, int *errno_out, const char *)
                  { return return_full_path(path_out, path_size, errno_out, "/tmp/input.bin"); });
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("output.bin")))
        .WillOnce([](char *path_out, size_t path_size, int *errno_out, const char *)
                  { return return_full_path(path_out, path_size, errno_out, "/tmp/output.bin"); });
    EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq("/tmp/input.bin"), StrEq("rb"), _)).WillOnce(Return(input_file));
    EXPECT_CALL(mock_com_util_, com_util_fseek(input_file, 0, SEEK_END)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_ftell(input_file)).WillOnce(Return((int64_t)sizeof(kCompressedPayload)));
    EXPECT_CALL(mock_com_util_, com_util_fseek(input_file, 0, SEEK_SET)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_fread(_, 1u, sizeof(kCompressedPayload), input_file))
        .WillOnce(
            [](void *ptr, size_t, size_t count, FILE *)
            {
                std::memcpy(ptr, kCompressedPayload, sizeof(kCompressedPayload));
                return count;
            }); // [Pre-Assert手順] - 入力ファイルの読み込みで圧縮済みデータ 7 byte を返却する。
    EXPECT_CALL(mock_com_util_, com_util_fclose(input_file)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_decompress(_, _, _, sizeof(kCompressedPayload)))
        .WillOnce(
            [](uint8_t *dst, size_t *dst_len, const uint8_t *, size_t)
            {
                EXPECT_EQ(
                    (size_t)6u,
                    *dst_len); // [Pre-Assert確認_正常系] - 展開 API にヘッダー値 6 byte が出力サイズとして渡ること。
                std::memcpy(dst, kDecompressedOutput, sizeof(kDecompressedOutput));
                *dst_len = sizeof(kDecompressedOutput) - 1u;
                return 0;
            }); // [Pre-Assert確認_異常系] - com_util_decompress が 1 回呼び出されること。
                // [Pre-Assert手順] - 展開後サイズをヘッダー値より 1 byte 少ない 5 byte に設定して返却する。
    EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq("/tmp/output.bin"), StrEq("wb"), _))
        .Times(0); // [Pre-Assert確認_異常系] - サイズ不一致時は出力ファイルが開かれないこと。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("展開後サイズがヘッダ値と一致")))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - "展開後サイズがヘッダ値と一致" を含むエラーが表示されること。

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - main() に引数を与えて呼び出す。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, rc); // [確認_異常系] - 戻り値が EXIT_SUCCESS 以外であること。
}

// 出力の書き込みが失敗した場合に部分出力ファイルが削除されることの確認
TEST_F(compress_cliTest, main_removes_partial_output_when_write_fails)
{
    // Arrange
    int argc = 4;
    const char *argv[] = {"compress-cli", "--decompress", "input.bin",
                          "output.bin"}; // [状態] - 展開モードで入力 "input.bin"、出力 "output.bin" とする。
    FILE *input_file = (FILE *)(uintptr_t)0x5050;
    FILE *output_file = (FILE *)(uintptr_t)0x6060;

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_console_init()).WillOnce(Return());
    EXPECT_CALL(mock_com_util_, com_util_paths_equal(StrEq("input.bin"), StrEq("output.bin"), _)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("input.bin")))
        .WillOnce([](char *path_out, size_t path_size, int *errno_out, const char *)
                  { return return_full_path(path_out, path_size, errno_out, "/tmp/input.bin"); });
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("output.bin")))
        .WillOnce([](char *path_out, size_t path_size, int *errno_out, const char *)
                  { return return_full_path(path_out, path_size, errno_out, "/tmp/output.bin"); });
    EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq("/tmp/input.bin"), StrEq("rb"), _)).WillOnce(Return(input_file));
    EXPECT_CALL(mock_com_util_, com_util_fseek(input_file, 0, SEEK_END)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_ftell(input_file)).WillOnce(Return((int64_t)sizeof(kCompressedPayload)));
    EXPECT_CALL(mock_com_util_, com_util_fseek(input_file, 0, SEEK_SET)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_fread(_, 1u, sizeof(kCompressedPayload), input_file))
        .WillOnce(
            [](void *ptr, size_t, size_t count, FILE *)
            {
                std::memcpy(ptr, kCompressedPayload, sizeof(kCompressedPayload));
                return count;
            }); // [Pre-Assert手順] - 入力ファイルの読み込みで圧縮済みデータ 7 byte を返却する。
    EXPECT_CALL(mock_com_util_, com_util_fclose(input_file)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_decompress(_, _, _, sizeof(kCompressedPayload)))
        .WillOnce(
            [](uint8_t *dst, size_t *dst_len, const uint8_t *, size_t)
            {
                std::memcpy(dst, kDecompressedOutput, sizeof(kDecompressedOutput));
                *dst_len = sizeof(kDecompressedOutput);
                return 0;
            }); // [Pre-Assert手順] - com_util_decompress から展開済みデータ 6 byte を返却する。
    EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq("/tmp/output.bin"), StrEq("wb"), _)).WillOnce(Return(output_file));
    EXPECT_CALL(mock_com_util_, com_util_fwrite(_, 1u, sizeof(kDecompressedOutput), output_file))
        .WillOnce(Return(
            sizeof(kDecompressedOutput) -
            1u)); // [Pre-Assert手順] - 出力ファイルの書き込みで要求より 1 byte 少ない 5 byte を返却し、書き込み失敗を発生させる。
    EXPECT_CALL(mock_com_util_, com_util_fclose(output_file)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_remove(StrEq("/tmp/output.bin")))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - 部分出力ファイル "/tmp/output.bin" が削除されること。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("書き込みに失敗")))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - "書き込みに失敗" を含むエラーが表示されること。

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - main() に引数を与えて呼び出す。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, rc); // [確認_異常系] - 戻り値が EXIT_SUCCESS 以外であること。
}

// パス比較 API が失敗した場合にファイルを開かず失敗終了することの確認
TEST_F(compress_cliTest, main_fails_when_path_comparison_fails)
{
    // Arrange
    int argc = 4;
    const char *argv[] = {"compress-cli", "--compress", "input.bin",
                          "output.bin"}; // [状態] - 圧縮モードで入力 "input.bin"、出力 "output.bin" とする。

    // Pre-Assert
    EXPECT_CALL(mock_com_util_, com_util_console_init()).WillOnce(Return());
    EXPECT_CALL(mock_com_util_, com_util_paths_equal(StrEq("input.bin"), StrEq("output.bin"), _))
        .WillOnce(
            DoAll(SetArgPointee<2>(EIO),
                  Return(-1))); // [Pre-Assert確認_異常系] - com_util_paths_equal で入出力パスの比較が行われること。
                                // [Pre-Assert手順] - errno_out に EIO を設定して -1 を返却する。
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - 比較失敗時は com_util_path_get_full が呼び出されないこと。
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("input.bin"))).Times(0);
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("output.bin"))).Times(0);
    EXPECT_CALL(mock_com_util_, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - 比較失敗時は com_util_fopen が呼び出されないこと。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("比較に失敗")))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - "比較に失敗" を含むエラーが表示されること。

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - main() に引数を与えて呼び出す。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, rc); // [確認_異常系] - 戻り値が EXIT_SUCCESS 以外であること。
}
