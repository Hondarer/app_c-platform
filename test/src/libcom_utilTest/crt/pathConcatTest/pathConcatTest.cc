#include <testfw.h>
#include <com_util/crt/path.h>
#include <cerrno>
#include <cstring>

class pathConcatTest : public Test
{
};

// パス断片が指定順に連結されることの確認
TEST_F(pathConcatTest, concatenates_path_fragments)
{
    // Arrange
    char path[PLATFORM_PATH_MAX] = {}; // [状態] - 出力バッファーを用意する。
    int err = 0;

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(
        0, com_util_path_concat(path, sizeof(path), &err, "tmp", PLATFORM_PATH_SEP,
                                "libbase_extdef.txt")); // [手順] - "tmp"、セパレータ、"libbase_extdef.txt" を連結する。
                                                        // [確認_正常系] - 戻り値が 0 であること。
    EXPECT_STREQ("tmp/libbase_extdef.txt", path);       // [確認_正常系] - 断片が指定順に連結されること。
}

// 空文字列の断片がそのまま扱えることの確認
TEST_F(pathConcatTest, keeps_empty_fragment)
{
    // Arrange
    char path[32] = {}; // [状態] - 出力バッファーを用意する。
    int err = 0;

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(0, com_util_path_concat(path, sizeof(path), &err, "", "abc")); // [手順] - 空文字列と "abc" を連結する。
                                                                             // [確認_正常系] - 戻り値が 0 であること。
    EXPECT_STREQ("abc", path); // [確認_正常系] - 空文字断片もそのまま扱え "abc" になること。
}

// サポート上限の 16 断片を連結できることの確認
TEST_F(pathConcatTest, accepts_sixteen_fragments)
{
    // Arrange
    char path[32] = {}; // [状態] - 出力バッファーを用意する。
    int err = 0;

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(0, com_util_path_concat(path, sizeof(path), &err, "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k",
                                      "l", "m", "n", "o", "p")); // [手順] - "a" から "p" までの 16 断片を連結する。
                                                                 // [確認_正常系] - 戻り値が 0 であること。
    EXPECT_STREQ("abcdefghijklmnop", path); // [確認_正常系] - サポート上限の 16 断片が連結されること。
}

// 断片数 0 の場合に EINVAL で失敗することの確認
TEST_F(pathConcatTest, returns_einval_for_zero_part_count)
{
    // Arrange
    char path[8] = {}; // [状態] - 出力バッファーを用意する。
    int err = 0;

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(-1, com_util_path_concat_n(path, sizeof(path), &err,
                                         0u)); // [手順] - part_count に 0 を渡して com_util_path_concat_n を呼び出す。
                                               // [確認_異常系] - 戻り値が -1 であること。
    EXPECT_EQ(EINVAL, err);                    // [確認_異常系] - errno_out に EINVAL が返ること。
}

// NULL 断片が含まれる場合に EINVAL で失敗することの確認
TEST_F(pathConcatTest, returns_einval_for_null_fragment)
{
    // Arrange
    char path[8] = {}; // [状態] - 出力バッファーを用意する。
    int err = 0;

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(
        -1, com_util_path_concat_n(
                path, sizeof(path), &err, 2u, "ab",
                (const char *)NULL)); // [手順] - 2 断片のうち 1 つに NULL を渡して com_util_path_concat_n を呼び出す。
                                      // [確認_異常系] - 戻り値が -1 であること。
    EXPECT_EQ(EINVAL, err);           // [確認_異常系] - errno_out に EINVAL が返ること。
}

// 連結結果がバッファーに収まらない場合に ENAMETOOLONG で失敗することの確認
TEST_F(pathConcatTest, returns_enametoolong_when_result_does_not_fit)
{
    // Arrange
    char path[5];
    int err = 0;

    std::memset(path, 'x',
                sizeof(path)); // [状態] - 5 バイトの出力バッファーを 'x' で埋め、クリアを検出できるようにする。

    // Pre-Assert

    // Act
    // Assert
    EXPECT_EQ(-1, com_util_path_concat(path, sizeof(path), &err, "ab", "cd",
                                       "e")); // [手順] - 連結結果が 5 文字 (+NUL) となる断片を渡して呼び出す。
                                              // [確認_異常系] - 戻り値が -1 であること。
    EXPECT_EQ(ENAMETOOLONG, err);             // [確認_異常系] - errno_out に ENAMETOOLONG が返ること。
    EXPECT_EQ('\0', path[0]);                 // [確認_異常系] - 失敗時は空文字列に初期化されること。
}
