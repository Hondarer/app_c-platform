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

TEST_F(compress_cliTest, main_rejects_unknown_option)
{
    // Arrange
    const char *argv[] = {"compress-cli", "--unknown", "input.bin",
                          "output.bin"}; // [状態] - 未対応オプションを指定する。
    EXPECT_CALL(mock_com_util_, com_util_console_init()).WillOnce(Return());

    // Pre-Assert

    // Act
    int rc = __real_main(4, (char **)&argv); // [手順] - 未対応オプションで main() を呼び出す。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, rc); // [確認_異常系] - 未対応オプションは拒否されること。
}

TEST_F(compress_cliTest, main_prints_usage_on_help)
{
    // Arrange
    const char *argv[] = {"compress-cli", "--help"}; // [状態] - help オプションだけを指定する。
    EXPECT_CALL(mock_com_util_, com_util_console_init()).WillOnce(Return());

    // Pre-Assert

    // Act
    int rc = __real_main(2, (char **)&argv); // [手順] - help オプションで main() を呼び出す。

    // Assert
    EXPECT_EQ(EXIT_SUCCESS, rc); // [確認_正常系] - 必須位置引数なしでも正常終了すること。
}

TEST_F(compress_cliTest, main_rejects_same_normalized_path)
{
    // Arrange
    int argc = 4;
    const char *argv[] = {"compress-cli", "--compress", "./data.bin", "subdir/../data.bin"};

    EXPECT_CALL(mock_com_util_, com_util_console_init())
        .WillOnce(Return()); // [Pre-Assert確認_正常系] - main 起動時に console 初期化が呼ばれること。
    EXPECT_CALL(mock_com_util_, com_util_paths_equal(StrEq("./data.bin"), StrEq("subdir/../data.bin"), _))
        .WillOnce(Return(1)); // [Pre-Assert確認_異常系] - 同一パス比較が com_util 側 API で行われること。
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - 同一パス検出時は個別の正規化を行わないこと。
    EXPECT_CALL(mock_com_util_, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - 同一パス検出時はファイルを開かないこと。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("同じパス")))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - 同一パスのエラーが表示されること。

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - 正規化後に同一となる入出力パスで起動する。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, rc); // [確認_異常系] - 同一パスは失敗終了すること。
}

TEST_F(compress_cliTest, main_compresses_input_and_writes_output)
{
    // Arrange
    int argc = 4;
    const char *argv[] = {"compress-cli", "--compress", "input.bin", "output.bin"};
    FILE *input_file = (FILE *)(uintptr_t)0x1010;
    FILE *output_file = (FILE *)(uintptr_t)0x2020;

    EXPECT_CALL(mock_com_util_, com_util_console_init())
        .WillOnce(Return()); // [Pre-Assert確認_正常系] - main 起動時に console 初期化が呼ばれること。
    EXPECT_CALL(mock_com_util_, com_util_paths_equal(StrEq("input.bin"), StrEq("output.bin"), _))
        .WillOnce(Return(0)); // [Pre-Assert確認_正常系] - 異なるパスは不一致と判定されること。
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("input.bin")))
        .WillOnce([](char *path_out, size_t path_size, int *errno_out, const char *)
                  { return return_full_path(path_out, path_size, errno_out, "/tmp/input.bin"); });
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("output.bin")))
        .WillOnce([](char *path_out, size_t path_size, int *errno_out, const char *)
                  { return return_full_path(path_out, path_size, errno_out, "/tmp/output.bin"); });
    EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq("/tmp/input.bin"), StrEq("rb"), _))
        .WillOnce(Return(input_file)); // [Pre-Assert確認_正常系] - 入力ファイルがバイナリ読み込みで開かれること。
    EXPECT_CALL(mock_com_util_, com_util_fseek(input_file, 0, SEEK_END)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_ftell(input_file)).WillOnce(Return((int64_t)sizeof(kPlainInput)));
    EXPECT_CALL(mock_com_util_, com_util_fseek(input_file, 0, SEEK_SET)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_fread(_, 1u, sizeof(kPlainInput), input_file))
        .WillOnce(
            [](void *ptr, size_t, size_t count, FILE *)
            {
                std::memcpy(ptr, kPlainInput, sizeof(kPlainInput));
                return count;
            });
    EXPECT_CALL(mock_com_util_, com_util_fclose(input_file)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_compress(_, _, _, sizeof(kPlainInput)))
        .WillOnce(
            [](uint8_t *dst, size_t *dst_len, const uint8_t *src, size_t)
            {
                EXPECT_EQ(0, std::memcmp(src, kPlainInput, sizeof(kPlainInput)));
                EXPECT_GE(*dst_len, sizeof(kCompressedPayload));
                std::memcpy(dst, kCompressedPayload, sizeof(kCompressedPayload));
                *dst_len = sizeof(kCompressedPayload);
                return 0;
            }); // [Pre-Assert確認_正常系] - 圧縮 API が入力データで呼ばれること。
    EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq("/tmp/output.bin"), StrEq("wb"), _))
        .WillOnce(Return(output_file)); // [Pre-Assert確認_正常系] - 出力ファイルがバイナリ書き込みで開かれること。
    EXPECT_CALL(mock_com_util_, com_util_fwrite(_, 1u, sizeof(kCompressedPayload), output_file))
        .WillOnce(
            [](const void *ptr, size_t, size_t count, FILE *)
            {
                EXPECT_EQ(0, std::memcmp(ptr, kCompressedPayload, sizeof(kCompressedPayload)));
                return count;
            });
    EXPECT_CALL(mock_com_util_, com_util_fclose(output_file)).WillOnce(Return(0));

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - 圧縮モードで main を呼び出す。

    // Assert
    EXPECT_EQ(EXIT_SUCCESS, rc); // [確認_正常系] - 圧縮成功時は正常終了すること。
}

TEST_F(compress_cliTest, main_rejects_decompress_input_when_header_size_is_too_small)
{
    // Arrange
    int argc = 4;
    const char *argv[] = {"compress-cli", "--decompress", "input.bin", "output.bin"};
    FILE *input_file = (FILE *)(uintptr_t)0x3030;
    const uint8_t invalid_input[] = {0x00, 0x00, 0x00, 0x04, 0x10};

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
            });
    EXPECT_CALL(mock_com_util_, com_util_fclose(input_file)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_decompress(_, _, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - ヘッダー不正時は展開 API を呼ばないこと。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("ヘッダの元サイズ"))).WillOnce(Return(0));

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - ヘッダー値が小さすぎる入力で展開する。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, rc); // [確認_異常系] - 不正ヘッダーで失敗終了すること。
}

TEST_F(compress_cliTest, main_rejects_decompress_output_when_size_mismatches_header)
{
    // Arrange
    int argc = 4;
    const char *argv[] = {"compress-cli", "--decompress", "input.bin", "output.bin"};
    FILE *input_file = (FILE *)(uintptr_t)0x4040;

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
            });
    EXPECT_CALL(mock_com_util_, com_util_fclose(input_file)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_decompress(_, _, _, sizeof(kCompressedPayload)))
        .WillOnce(
            [](uint8_t *dst, size_t *dst_len, const uint8_t *, size_t)
            {
                EXPECT_EQ((size_t)6u, *dst_len);
                std::memcpy(dst, kDecompressedOutput, sizeof(kDecompressedOutput));
                *dst_len = sizeof(kDecompressedOutput) - 1u;
                return 0;
            }); // [Pre-Assert確認_異常系] - 展開後サイズ不一致を模擬する。
    EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq("/tmp/output.bin"), StrEq("wb"), _))
        .Times(0); // [Pre-Assert確認_異常系] - サイズ不一致時は出力ファイルを開かないこと。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("展開後サイズがヘッダ値と一致"))).WillOnce(Return(0));

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - ヘッダー値と展開後サイズが一致しない入力で展開する。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, rc); // [確認_異常系] - サイズ不一致で失敗終了すること。
}

TEST_F(compress_cliTest, main_removes_partial_output_when_write_fails)
{
    // Arrange
    int argc = 4;
    const char *argv[] = {"compress-cli", "--decompress", "input.bin", "output.bin"};
    FILE *input_file = (FILE *)(uintptr_t)0x5050;
    FILE *output_file = (FILE *)(uintptr_t)0x6060;

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
            });
    EXPECT_CALL(mock_com_util_, com_util_fclose(input_file)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_decompress(_, _, _, sizeof(kCompressedPayload)))
        .WillOnce(
            [](uint8_t *dst, size_t *dst_len, const uint8_t *, size_t)
            {
                std::memcpy(dst, kDecompressedOutput, sizeof(kDecompressedOutput));
                *dst_len = sizeof(kDecompressedOutput);
                return 0;
            });
    EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq("/tmp/output.bin"), StrEq("wb"), _)).WillOnce(Return(output_file));
    EXPECT_CALL(mock_com_util_, com_util_fwrite(_, 1u, sizeof(kDecompressedOutput), output_file))
        .WillOnce(Return(sizeof(kDecompressedOutput) -
                         1u)); // [Pre-Assert手順_異常系] - 途中までしか書き込めない状態を模擬する。
    EXPECT_CALL(mock_com_util_, com_util_fclose(output_file)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util_, com_util_remove(StrEq("/tmp/output.bin")))
        .WillOnce(Return(0)); // [Pre-Assert確認_異常系] - 部分出力ファイルが削除されること。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("書き込みに失敗"))).WillOnce(Return(0));

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - 出力書き込み失敗を模擬して展開する。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, rc); // [確認_異常系] - 書き込み失敗で異常終了すること。
}

TEST_F(compress_cliTest, main_fails_when_path_comparison_fails)
{
    // Arrange
    int argc = 4;
    const char *argv[] = {"compress-cli", "--compress", "input.bin", "output.bin"};

    EXPECT_CALL(mock_com_util_, com_util_console_init()).WillOnce(Return());
    EXPECT_CALL(mock_com_util_, com_util_paths_equal(StrEq("input.bin"), StrEq("output.bin"), _))
        .WillOnce(
            DoAll(SetArgPointee<2>(EIO), Return(-1))); // [Pre-Assert確認_異常系] - パス比較 API の失敗を模擬する。
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - 比較失敗時は個別の正規化を行わないこと。
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("input.bin"))).Times(0);
    EXPECT_CALL(mock_com_util_, com_util_path_get_full(_, _, _, StrEq("output.bin"))).Times(0);
    EXPECT_CALL(mock_com_util_, com_util_fopen(_, _, _))
        .Times(0); // [Pre-Assert確認_異常系] - 比較失敗時はファイルを開かないこと。
    EXPECT_CALL(mock_stdio_, fprintf(_, _, _, _, HasSubstr("比較に失敗"))).WillOnce(Return(0));

    // Act
    int rc = __real_main(argc, (char **)&argv); // [手順] - パス比較 API が失敗する状態で起動する。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, rc); // [確認_異常系] - パス比較失敗で異常終了すること。
}
