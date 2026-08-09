#include <testfw.h>
#include <com_util/crt/stdlib.h>

#include <errno.h>
#include <string.h>

class envTest : public Test
{
};

// com_util_setenv で設定した値が com_util_getenv で取得できることの確認
TEST_F(envTest, setenv_value_is_readable)
{
    // Arrange
    char buf[64];
    int exists = 0;

    memset(buf, 0, sizeof(buf)); // [状態] - 64 バイトの値格納先を 0 で初期化する。

    // Pre-Assert

    // Act
    int rtc_setenv =
        com_util_setenv("COM_UTIL_ENV_TEST", "value1", 1, NULL); // [手順] - COM_UTIL_ENV_TEST に "value1" を設定する。

    // Assert
    EXPECT_EQ(0, rtc_setenv); // [確認_正常系] - com_util_setenv の戻り値が 0 であること。
    ASSERT_EQ(0, com_util_getenv("COM_UTIL_ENV_TEST", buf, sizeof(buf), &exists, NULL));
    EXPECT_EQ(1, exists);        // [確認_正常系] - 環境変数が設定済みとして報告されること。
    EXPECT_STREQ("value1", buf); // [確認_正常系] - 取得した値が "value1" であること。

    // Cleanup
    com_util_unsetenv("COM_UTIL_ENV_TEST", NULL);
}

// overwrite に 0 を指定した com_util_setenv が既存の値を保持することの確認
TEST_F(envTest, setenv_without_overwrite_keeps_existing_value)
{
    // Arrange
    char buf[64];
    int exists = 0;

    memset(buf, 0, sizeof(buf));
    ASSERT_EQ(0, com_util_setenv("COM_UTIL_ENV_TEST", "first", 1, NULL)); // [状態] - 事前に値 "first" を設定しておく。

    // Pre-Assert

    // Act
    int rtc_setenv = com_util_setenv("COM_UTIL_ENV_TEST", "second", 0,
                                     NULL); // [手順] - overwrite に 0 を指定して "second" を設定する。

    // Assert
    EXPECT_EQ(0, rtc_setenv); // [確認_正常系] - overwrite が 0 の com_util_setenv の戻り値が 0 であること。
    ASSERT_EQ(0, com_util_getenv("COM_UTIL_ENV_TEST", buf, sizeof(buf), &exists, NULL));
    EXPECT_STREQ("first", buf); // [確認_正常系] - 既存の値 "first" が保持されていること。

    // Cleanup
    com_util_unsetenv("COM_UTIL_ENV_TEST", NULL);
}

// com_util_unsetenv が環境変数を削除することの確認
TEST_F(envTest, unsetenv_removes_variable)
{
    // Arrange
    int exists = 1;

    ASSERT_EQ(0,
              com_util_setenv("COM_UTIL_ENV_TEST", "value1", 1, NULL)); // [状態] - 事前に値 "value1" を設定しておく。

    // Pre-Assert

    // Act
    int rtc_unsetenv = com_util_unsetenv("COM_UTIL_ENV_TEST", NULL); // [手順] - COM_UTIL_ENV_TEST を削除する。

    // Assert
    EXPECT_EQ(0, rtc_unsetenv); // [確認_正常系] - com_util_unsetenv の戻り値が 0 であること。
    ASSERT_EQ(0, com_util_getenv("COM_UTIL_ENV_TEST", NULL, 0, &exists, NULL));
    EXPECT_EQ(0, exists); // [確認_正常系] - 環境変数が未設定として報告されること。
}

// 不正な変数名が EINVAL で拒否されることの確認
TEST_F(envTest, invalid_name_is_rejected)
{
    // Arrange
    com_util_error detail;

    // Pre-Assert

    // Act
    int rtc_null_name = com_util_setenv(NULL, "value1", 1, &detail);   // [手順] - 変数名に NULL を指定して呼び出す。
    int rtc_empty_name = com_util_setenv("", "value1", 1, NULL);       // [手順] - 変数名に空文字列を指定して呼び出す。
    int rtc_equal_in_name = com_util_setenv("A=B", "value1", 1, NULL); // [手順] - 変数名に '=' を含めて呼び出す。
    int rtc_null_value = com_util_setenv("COM_UTIL_ENV_TEST", NULL, 1, NULL); // [手順] - 値に NULL を指定して呼び出す。
    int rtc_unset_null =
        com_util_unsetenv(NULL, NULL); // [手順] - 変数名に NULL を指定して com_util_unsetenv を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, rtc_null_name);
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_ERRNO,
              com_util_error_get_domain(&detail)); // [確認_異常系] - detail のドメインが errno であること。
    EXPECT_EQ(
        EINVAL,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EINVAL であること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, rtc_empty_name);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, rtc_equal_in_name);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, rtc_null_value);
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT, rtc_unset_null);
}
