#include <testfw.h>
#include <com_util/base/error_message_internal.h>
#include <com_util/base/result.h>

#include <errno.h>

#include <cstring>
#include <set>
#include <string>
#include <vector>

/* result.h が定義するエラー コードの一覧 (COM_UTIL_OK を除く)。 */
static std::vector<int> all_error_codes()
{
    return std::vector<int>{COM_UTIL_ERR_UNKNOWN,
                            COM_UTIL_ERR_INVALID_ARGUMENT,
                            COM_UTIL_ERR_UNSUPPORTED,
                            COM_UTIL_ERR_PERMISSION_DENIED,
                            COM_UTIL_ERR_DUPLICATE_DEFINITION,
                            COM_UTIL_ERR_OUT_OF_MEMORY,
                            COM_UTIL_ERR_BUSY,
                            COM_UTIL_ERR_TIMEOUT,
                            COM_UTIL_ERR_LIMIT_EXCEEDED,
                            COM_UTIL_ERR_BUFFER_TOO_SMALL,
                            COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
                            COM_UTIL_ERR_UNKNOWN_OPTION,
                            COM_UTIL_ERR_MISSING_VALUE,
                            COM_UTIL_ERR_UNEXPECTED_VALUE,
                            COM_UTIL_ERR_INVALID_INTEGER,
                            COM_UTIL_ERR_OUT_OF_RANGE,
                            COM_UTIL_ERR_MISSING_REQUIRED,
                            COM_UTIL_ERR_DUPLICATE_OPTION,
                            COM_UTIL_ERR_TOO_MANY_ARGUMENTS,
                            COM_UTIL_ERR_TOO_MANY_OCCURRENCES,
                            COM_UTIL_ERR_EOF,
                            COM_UTIL_ERR_CANCELED};
}

class errorMessageTest : public Test
{
};

// 既知の結果コードが固有の文字列へ変換されることの確認
TEST_F(errorMessageTest, known_codes_map_to_distinct_texts)
{
    // Arrange
    const std::vector<int> codes = all_error_codes(); // [状態] - COM_UTIL_OK を除く全エラー コードを列挙する。
    std::set<std::string> texts;

    // Pre-Assert

    // Act
    for (int code : codes)
    {
        texts.insert(std::string(com_util_result_to_string(code))); // [手順] - 各コードを文字列化して集合へ挿入する。
    }

    // Assert
    EXPECT_EQ(codes.size(),
              texts.size()); // [確認_正常系] - 文字列が重複せず、集合の要素数が列挙したコード数と一致すること。
    EXPECT_STREQ("success",
                 com_util_result_to_string(COM_UTIL_OK)); // [確認_正常系] - COM_UTIL_OK が "success" になること。
}

// 未知の値が既定の文字列になることの確認
TEST_F(errorMessageTest, unknown_code_maps_to_default_text)
{
    // Arrange

    // Pre-Assert

    // Act
    const char *text = com_util_result_to_string(-9999); // [手順] - 定義されていない値 -9999 を文字列化する。

    // Assert
    EXPECT_STREQ("unknown result code",
                 text); // [確認_異常系] - com_util_result_to_string の戻り値が "unknown result code" であること。
}

// errno がメッセージへ変換されることの確認
TEST_F(errorMessageTest, errno_is_converted_to_message)
{
    // Arrange
    char buf[256];

    memset(buf, 0, sizeof(buf)); // [状態] - 256 byte の格納先を 0 で初期化する。

    // Pre-Assert

    // Act
    int rtc = com_util_errno_message(buf, sizeof(buf), ENOENT); // [手順] - ENOENT を指定して呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_errno_message の戻り値が COM_UTIL_OK であること。
    EXPECT_LT(0U, strlen(buf));  // [確認_正常系] - 空でないメッセージが格納されること。
}

// 引数不正の場合に COM_UTIL_ERR_INVALID_ARGUMENT を返すことの確認
TEST_F(errorMessageTest, invalid_arguments_are_rejected)
{
    // Arrange
    char buf[16];

    memset(buf, 0, sizeof(buf)); // [状態] - 16 byte の格納先を 0 で初期化する。

    // Pre-Assert

    // Act
    int rtc_null_buf = com_util_errno_message(NULL, sizeof(buf), ENOENT); // [手順] - 格納先に NULL を指定して呼び出す。
    int rtc_zero_size = com_util_errno_message(buf, 0U, ENOENT);          // [手順] - サイズに 0 を指定して呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_buf); // [確認_異常系] - 格納先が NULL の場合に com_util_errno_message の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_zero_size); // [確認_異常系] - サイズが 0 の場合に com_util_errno_message の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}
