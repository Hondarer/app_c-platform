#include <testfw.h>
#include <com_util/crt/stdlib.h>
#include <mock_stdlib.h>

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
    int rtc_getenv = com_util_getenv("COM_UTIL_ENV_TEST", buf, sizeof(buf), &exists,
                                     NULL); // [手順] - COM_UTIL_ENV_TEST の値を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc_setenv); // [確認_正常系] - com_util_setenv の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, rtc_getenv); // [確認_正常系] - com_util_getenv の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(1, exists);               // [確認_正常系] - 環境変数が設定済みとして報告されること。
    EXPECT_STREQ("value1", buf);        // [確認_正常系] - 取得した値が "value1" であること。

    // Cleanup
    com_util_unsetenv("COM_UTIL_ENV_TEST", NULL);
}

// overwrite に 0 を指定した com_util_setenv が既存の値を保持することの確認
TEST_F(envTest, setenv_without_overwrite_keeps_existing_value)
{
    // Arrange
    char buf[64];

    memset(buf, 0, sizeof(buf));
    ASSERT_EQ(COM_UTIL_OK,
              com_util_setenv("COM_UTIL_ENV_TEST", "first", 1, NULL)); // [状態] - 事前に値 "first" を設定しておく。

    // Pre-Assert

    // Act
    int rtc_setenv = com_util_setenv("COM_UTIL_ENV_TEST", "second", 0,
                                     NULL); // [手順] - overwrite に 0 を指定して "second" を設定する。
    int rtc_getenv =
        com_util_getenv("COM_UTIL_ENV_TEST", buf, sizeof(buf), NULL, NULL); // [手順] - 設定後の値を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_setenv); // [確認_正常系] - overwrite が 0 の com_util_setenv の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, rtc_getenv); // [確認_正常系] - com_util_getenv の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("first", buf);         // [確認_正常系] - 既存の値 "first" が保持されていること。

    // Cleanup
    com_util_unsetenv("COM_UTIL_ENV_TEST", NULL);
}

// overwrite に 0 を指定しても未設定の環境変数には値が設定されることの確認
// Windows の com_util_setenv は overwrite が 0 のとき com_util_getenv で既存を確認してから
// _putenv_s へ進むため、未設定時に設定が行われる経路をこのテストで通す
TEST_F(envTest, setenv_without_overwrite_sets_value_when_absent)
{
    // Arrange
    char buf[64];

    memset(buf, 0, sizeof(buf));
    ASSERT_EQ(COM_UTIL_OK, com_util_unsetenv("COM_UTIL_ENV_TEST",
                                             NULL)); // [状態] - 対象の環境変数を未設定の状態にしておく。

    // Pre-Assert

    // Act
    int rtc_setenv = com_util_setenv("COM_UTIL_ENV_TEST", "created", 0,
                                     NULL); // [手順] - overwrite に 0 を指定して "created" を設定する。
    int rtc_getenv =
        com_util_getenv("COM_UTIL_ENV_TEST", buf, sizeof(buf), NULL, NULL); // [手順] - 設定後の値を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc_setenv); // [確認_正常系] - com_util_setenv の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, rtc_getenv); // [確認_正常系] - com_util_getenv の戻り値が COM_UTIL_OK であること。
    EXPECT_STREQ("created", buf);       // [確認_正常系] - 未設定だったため "created" が設定されること。

    // Cleanup
    com_util_unsetenv("COM_UTIL_ENV_TEST", NULL);
}

// com_util_unsetenv が環境変数を削除することの確認
TEST_F(envTest, unsetenv_removes_variable)
{
    // Arrange
    int exists = 1;

    ASSERT_EQ(COM_UTIL_OK,
              com_util_setenv("COM_UTIL_ENV_TEST", "value1", 1, NULL)); // [状態] - 事前に値 "value1" を設定しておく。

    // Pre-Assert

    // Act
    int rtc_unsetenv = com_util_unsetenv("COM_UTIL_ENV_TEST", NULL); // [手順] - COM_UTIL_ENV_TEST を削除する。
    int rtc_getenv = com_util_getenv("COM_UTIL_ENV_TEST", NULL, 0, &exists,
                                     NULL); // [手順] - 削除後に COM_UTIL_ENV_TEST の有無を取得する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc_unsetenv); // [確認_正常系] - com_util_unsetenv の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, rtc_getenv);   // [確認_正常系] - com_util_getenv の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0, exists);                 // [確認_正常系] - 環境変数が未設定として報告されること。
}

// com_util_setenv が不正な変数名と値を EINVAL で拒否することの確認
TEST_F(envTest, setenv_rejects_invalid_name_and_value)
{
    // Arrange
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert

    // Act
    int rtc_null_name = com_util_setenv(NULL, "value1", 1, &detail);   // [手順] - 変数名に NULL を指定して呼び出す。
    int rtc_empty_name = com_util_setenv("", "value1", 1, NULL);       // [手順] - 変数名に空文字列を指定して呼び出す。
    int rtc_equal_in_name = com_util_setenv("A=B", "value1", 1, NULL); // [手順] - 変数名に '=' を含めて呼び出す。
    int rtc_null_value = com_util_setenv("COM_UTIL_ENV_TEST", NULL, 1, NULL); // [手順] - 値に NULL を指定して呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_name); // [確認_異常系] - 変数名が NULL のとき com_util_setenv の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_ERRNO,
              com_util_error_get_domain(&detail)); // [確認_異常系] - detail のドメインが errno であること。
    EXPECT_EQ(
        EINVAL,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EINVAL であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_empty_name); // [確認_異常系] - 変数名が空文字列のとき com_util_setenv の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_equal_in_name); // [確認_異常系] - 変数名に '=' を含むとき com_util_setenv の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_value); // [確認_異常系] - 値が NULL のとき com_util_setenv の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// com_util_unsetenv が不正な変数名を EINVAL で拒否することの確認
TEST_F(envTest, unsetenv_rejects_invalid_name)
{
    // Arrange
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert

    // Act
    int rtc_null_name = com_util_unsetenv(NULL, &detail);   // [手順] - 変数名に NULL を指定して呼び出す。
    int rtc_empty_name = com_util_unsetenv("", NULL);       // [手順] - 変数名に空文字列を指定して呼び出す。
    int rtc_equal_in_name = com_util_unsetenv("A=B", NULL); // [手順] - 変数名に '=' を含めて呼び出す。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_null_name); // [確認_異常系] - 変数名が NULL のとき com_util_unsetenv の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        EINVAL,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EINVAL であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_empty_name); // [確認_異常系] - 変数名が空文字列のとき com_util_unsetenv の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_equal_in_name); // [確認_異常系] - 変数名に '=' を含むとき com_util_unsetenv の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// com_util_getenv が変数名に NULL を渡された場合に EINVAL を返すことの確認
TEST_F(envTest, getenv_returns_einval_for_null_name)
{
    // Arrange
    char buf[64];
    int exists = 1;        // [状態] - 未設定への書き換えを確認するため 1 で初期化する。
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    memset(buf, 0, sizeof(buf));

    // Pre-Assert

    // Act
    int rtc = com_util_getenv(NULL, buf, sizeof(buf), &exists,
                              &detail); // [手順] - 変数名に NULL を指定して com_util_getenv を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc);       // [確認_異常系] - com_util_getenv の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(0, exists); // [確認_異常系] - 変数名の検査より前に exists_out が 0 へ初期化されること。
    EXPECT_EQ(
        EINVAL,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EINVAL であること。
}

// 未設定の環境変数に対して com_util_getenv が出力バッファーを空文字列にすることの確認
TEST_F(envTest, getenv_clears_buffer_when_variable_is_absent)
{
    // Arrange
    char buf[64];
    int exists = 1; // [状態] - 未設定への書き換えを確認するため 1 で初期化する。

    memset(buf, 'X', sizeof(buf)); // [状態] - 出力バッファーを 'X' で埋めておく。
    ASSERT_EQ(COM_UTIL_OK, com_util_unsetenv("COM_UTIL_ENV_TEST",
                                             NULL)); // [状態] - 対象の環境変数を未設定の状態にしておく。

    // Pre-Assert

    // Act
    int rtc = com_util_getenv("COM_UTIL_ENV_TEST", buf, sizeof(buf), &exists,
                              NULL); // [手順] - 未設定の環境変数に対して com_util_getenv を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_getenv の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0, exists);        // [確認_正常系] - 環境変数が未設定として報告されること。
    EXPECT_STREQ("", buf);       // [確認_正常系] - 出力バッファーが空文字列になること。
}

// 未設定の環境変数に対して buf_size が 0 の場合に com_util_getenv が書き込まないことの確認
TEST_F(envTest, getenv_does_not_write_when_buffer_size_is_zero)
{
    // Arrange
    char buf[4];
    int exists = 1; // [状態] - 未設定への書き換えを確認するため 1 で初期化する。

    memset(buf, 'X', sizeof(buf)); // [状態] - 出力バッファーを 'X' で埋めておく。
    ASSERT_EQ(COM_UTIL_OK, com_util_unsetenv("COM_UTIL_ENV_TEST",
                                             NULL)); // [状態] - 対象の環境変数を未設定の状態にしておく。

    // Pre-Assert

    // Act
    int rtc = com_util_getenv("COM_UTIL_ENV_TEST", buf, 0u, &exists,
                              NULL); // [手順] - buf は非 NULL、buf_size に 0 を指定して com_util_getenv を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_getenv の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0, exists);        // [確認_正常系] - 環境変数が未設定として報告されること。
    EXPECT_EQ('X', buf[0]);      // [確認_正常系] - buf_size が 0 のため出力バッファーへ書き込まないこと。
}

// 未設定の環境変数を出力バッファーなしで照会できることの確認
TEST_F(envTest, getenv_accepts_null_buffer_when_variable_is_absent)
{
    // Arrange
    int exists = 1; // [状態] - 未設定への書き換えを確認するため 1 で初期化する。

    ASSERT_EQ(COM_UTIL_OK, com_util_unsetenv("COM_UTIL_ENV_TEST",
                                             NULL)); // [状態] - 対象の環境変数を未設定の状態にしておく。

    // Pre-Assert

    // Act
    int rtc = com_util_getenv("COM_UTIL_ENV_TEST", NULL, 0u, &exists,
                              NULL); // [手順] - 未設定の環境変数を出力バッファーなしで照会する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_getenv の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(0, exists);        // [確認_正常系] - 環境変数が未設定として報告されること。
}

// 設定済みの環境変数を出力バッファーなしで照会できることの確認
TEST_F(envTest, getenv_accepts_null_buffer_when_variable_exists)
{
    // Arrange
    int exists = 0; // [状態] - 設定済みへの書き換えを確認するため 0 で初期化する。

    ASSERT_EQ(COM_UTIL_OK,
              com_util_setenv("COM_UTIL_ENV_TEST", "value1", 1, NULL)); // [状態] - 対象の環境変数へ値を設定しておく。

    // Pre-Assert

    // Act
    int rtc = com_util_getenv("COM_UTIL_ENV_TEST", NULL, 0u, &exists,
                              NULL); // [手順] - 設定済みの環境変数を出力バッファーなしで照会する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc); // [確認_正常系] - com_util_getenv の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(1, exists);        // [確認_正常系] - 環境変数が設定済みとして報告されること。

    // Cleanup
    com_util_unsetenv("COM_UTIL_ENV_TEST", NULL);
}

// 出力バッファーが値の長さに満たない場合に com_util_getenv が ERANGE を返すことの確認
TEST_F(envTest, getenv_returns_erange_when_buffer_too_small)
{
    // Arrange
    char buf[4];           // [状態] - "value1" (終端込みで 7 バイト必要) に対し 4 バイトのバッファーを用意する。
    int exists = 0;        // [状態] - 設定済みへの書き換えを確認するため 0 で初期化する。
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    memset(buf, 0, sizeof(buf));
    ASSERT_EQ(COM_UTIL_OK,
              com_util_setenv("COM_UTIL_ENV_TEST", "value1", 1, NULL)); // [状態] - 事前に値 "value1" を設定しておく。

    // Pre-Assert

    // Act
    int rtc = com_util_getenv("COM_UTIL_ENV_TEST", buf, sizeof(buf), &exists,
                              &detail); // [手順] - 4 バイトのバッファーを指定して com_util_getenv を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_BUFFER_TOO_SMALL,
              rtc);       // [確認_異常系] - com_util_getenv の戻り値が COM_UTIL_ERR_BUFFER_TOO_SMALL であること。
    EXPECT_EQ(1, exists); // [確認_異常系] - バッファー不足でも環境変数が設定済みとして報告されること。
    EXPECT_EQ(
        ERANGE,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が ERANGE であること。

    // Cleanup
    com_util_unsetenv("COM_UTIL_ENV_TEST", NULL);
}

#if defined(PLATFORM_LINUX)

// setenv の失敗が errno とともに通知されることの確認
// Windows の com_util_setenv は _putenv_s を使うため、この失敗経路は Linux のみに存在する
TEST_F(envTest, setenv_reports_errno_when_platform_setenv_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, setenv(_, _, _, StrEq("COM_UTIL_ENV_TEST"), StrEq("value1"), 1))
        .WillOnce(DoAll(
            Assign(&errno, ENOMEM),
            Return(
                -1))); // [Pre-Assert確認_異常系] - setenv が名前 "COM_UTIL_ENV_TEST"、値 "value1"、overwrite 1 を指定して 1 回呼び出されること。
                       // [Pre-Assert手順] - errno に ENOMEM を設定し、setenv から -1 を返却する。

    // Act
    int rtc = com_util_setenv("COM_UTIL_ENV_TEST", "value1", 1, &detail); // [手順] - com_util_setenv を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_MEMORY,
              rtc); // [確認_異常系] - com_util_setenv の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
    EXPECT_EQ(
        ENOMEM,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が ENOMEM であること。
}

// unsetenv の失敗が errno とともに通知されることの確認
// Windows の com_util_unsetenv は _putenv_s を使うため、この失敗経路は Linux のみに存在する
TEST_F(envTest, unsetenv_reports_errno_when_platform_unsetenv_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    com_util_error detail; // [状態] - 詳細エラーの格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, unsetenv(_, _, _, StrEq("COM_UTIL_ENV_TEST")))
        .WillOnce(DoAll(
            Assign(&errno, EINVAL),
            Return(
                -1))); // [Pre-Assert確認_異常系] - unsetenv が名前 "COM_UTIL_ENV_TEST" を指定して 1 回呼び出されること。
                       // [Pre-Assert手順] - errno に EINVAL を設定し、unsetenv から -1 を返却する。

    // Act
    int rtc = com_util_unsetenv("COM_UTIL_ENV_TEST", &detail); // [手順] - com_util_unsetenv を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              rtc); // [確認_異常系] - com_util_unsetenv の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
    EXPECT_EQ(
        EINVAL,
        com_util_error_get_errno(&detail)); // [確認_異常系] - com_util_error_get_errno の戻り値が EINVAL であること。
}

#endif /* PLATFORM_LINUX */
