#include <testfw.h>
#include <com_util/prompt/prompt.h>
#include <com_util/prompt/prompt_edit.h>
#include <mock_stdlib.h>

#include <cstdlib>
#include <cstring>

class promptEditTest : public Test
{
};

/*
 * com_util_prompt_edit_utf8_prev_boundary
 */

// 位置 0 からは移動しないことの確認
TEST_F(promptEditTest, prev_boundary_stays_at_zero)
{
    // Arrange
    const char text[] = "abc"; // [状態] - ASCII のみの文字列を用意する。

    // Pre-Assert

    // Act
    size_t pos = com_util_prompt_edit_utf8_prev_boundary(text, 0u); // [手順] - 位置 0 を指定して呼び出す。

    // Assert
    EXPECT_EQ(0u, pos); // [確認_正常系] - com_util_prompt_edit_utf8_prev_boundary の戻り値が 0 であること。
}

// ASCII 文字を 1 つ戻ることの確認
TEST_F(promptEditTest, prev_boundary_moves_one_byte_for_ascii)
{
    // Arrange
    const char text[] = "abc"; // [状態] - ASCII のみの文字列を用意する。

    // Pre-Assert

    // Act
    size_t pos = com_util_prompt_edit_utf8_prev_boundary(text, 3u); // [手順] - 終端の位置 3 を指定して呼び出す。

    // Assert
    EXPECT_EQ(2u, pos); // [確認_正常系] - 1 バイト戻った位置 2 が返ること。
}

// マルチバイト文字の先頭まで戻ることの確認
TEST_F(promptEditTest, prev_boundary_skips_continuation_bytes)
{
    // Arrange
    const char text[] = "a\xE3\x81\x82"; // [状態] - ASCII 1 文字と 3 バイトの日本語 1 文字を含む文字列を用意する。

    // Pre-Assert

    // Act
    size_t pos = com_util_prompt_edit_utf8_prev_boundary(text, 4u); // [手順] - 終端の位置 4 を指定して呼び出す。

    // Assert
    EXPECT_EQ(1u, pos); // [確認_正常系] - 継続バイトを読み飛ばして日本語文字の先頭である位置 1 が返ること。
}

/*
 * com_util_prompt_edit_utf8_next_boundary
 */

// 終端以降では長さを返すことの確認
TEST_F(promptEditTest, next_boundary_returns_len_at_end)
{
    // Arrange
    const char text[] = "abc"; // [状態] - 長さ 3 の文字列を用意する。

    // Pre-Assert

    // Act
    size_t pos = com_util_prompt_edit_utf8_next_boundary(text, 3u, 3u); // [手順] - 終端の位置 3 を指定して呼び出す。

    // Assert
    EXPECT_EQ(3u, pos); // [確認_正常系] - 長さと同じ 3 が返ること。
}

// ASCII 文字を 1 つ進むことの確認
TEST_F(promptEditTest, next_boundary_moves_one_byte_for_ascii)
{
    // Arrange
    const char text[] = "abc"; // [状態] - ASCII のみの文字列を用意する。

    // Pre-Assert

    // Act
    size_t pos = com_util_prompt_edit_utf8_next_boundary(text, 3u, 0u); // [手順] - 先頭の位置 0 を指定して呼び出す。

    // Assert
    EXPECT_EQ(1u, pos); // [確認_正常系] - 1 バイト進んだ位置 1 が返ること。
}

// マルチバイト文字の次の境界まで進むことの確認
TEST_F(promptEditTest, next_boundary_skips_continuation_bytes)
{
    // Arrange
    const char text[] = "\xE3\x81\x82" "a"; // [状態] - 3 バイトの日本語 1 文字と ASCII 1 文字を含む文字列を用意する。

    // Pre-Assert

    // Act
    size_t pos = com_util_prompt_edit_utf8_next_boundary(text, 4u, 0u); // [手順] - 先頭の位置 0 を指定して呼び出す。

    // Assert
    EXPECT_EQ(3u, pos); // [確認_正常系] - 継続バイトを読み飛ばして次の文字の先頭である位置 3 が返ること。
}

/*
 * com_util_prompt_edit_utf8_sanitize_boundary
 */

// 長さを超える位置が長さへ丸められることの確認
TEST_F(promptEditTest, sanitize_boundary_clamps_to_len)
{
    // Arrange
    const char text[] = "abc"; // [状態] - 長さ 3 の文字列を用意する。

    // Pre-Assert

    // Act
    size_t pos = com_util_prompt_edit_utf8_sanitize_boundary(text, 3u, 10u); // [手順] - 長さを超える位置 10 を指定する。

    // Assert
    EXPECT_EQ(3u, pos); // [確認_正常系] - 長さと同じ 3 へ丸められること。
}

// 文字の途中を指す位置が先頭へ戻されることの確認
TEST_F(promptEditTest, sanitize_boundary_moves_back_to_character_head)
{
    // Arrange
    const char text[] = "\xE3\x81\x82" "a"; // [状態] - 3 バイトの日本語 1 文字と ASCII 1 文字を含む文字列を用意する。

    // Pre-Assert

    // Act
    size_t pos = com_util_prompt_edit_utf8_sanitize_boundary(text, 4u,
                                                             2u); // [手順] - 日本語文字の途中を指す位置 2 を指定する。

    // Assert
    EXPECT_EQ(0u, pos); // [確認_正常系] - 文字の先頭である位置 0 まで戻されること。
}

// 境界上の位置が変化しないことの確認
TEST_F(promptEditTest, sanitize_boundary_keeps_valid_position)
{
    // Arrange
    const char text[] = "\xE3\x81\x82" "a"; // [状態] - 3 バイトの日本語 1 文字と ASCII 1 文字を含む文字列を用意する。

    // Pre-Assert

    // Act
    size_t pos = com_util_prompt_edit_utf8_sanitize_boundary(text, 4u, 3u); // [手順] - 境界上の位置 3 を指定する。

    // Assert
    EXPECT_EQ(3u, pos); // [確認_正常系] - 位置 3 のまま変化しないこと。
}

/*
 * com_util_prompt_edit_ensure_capacity
 */

// 既存容量で足りる場合に再確保しないことの確認
TEST_F(promptEditTest, ensure_capacity_keeps_buffer_when_enough)
{
    // Arrange
    char *buf = static_cast<char *>(std::malloc(16u));
    size_t cap = 16u; // [状態] - 16 byte を確保済みのバッファーを用意する。

    ASSERT_NE(nullptr, buf);

    // Pre-Assert

    // Act
    int rtc = com_util_prompt_edit_ensure_capacity(&buf, &cap, 64u, 8u); // [手順] - 必要量 8 を指定して呼び出す。

    // Assert
    EXPECT_EQ(0, rtc);   // [確認_正常系] - com_util_prompt_edit_ensure_capacity の戻り値が 0 であること。
    EXPECT_EQ(16u, cap); // [確認_正常系] - 容量が 16 のまま変化しないこと。

    // Cleanup
    std::free(buf);
}

// 容量が 2 倍ずつ拡張されることの確認
TEST_F(promptEditTest, ensure_capacity_grows_by_doubling)
{
    // Arrange
    char *buf = static_cast<char *>(std::malloc(4u));
    size_t cap = 4u; // [状態] - 4 byte を確保済みのバッファーを用意する。

    ASSERT_NE(nullptr, buf);

    // Pre-Assert

    // Act
    int rtc = com_util_prompt_edit_ensure_capacity(&buf, &cap, 64u, 17u); // [手順] - 必要量 17 を指定して呼び出す。

    // Assert
    EXPECT_EQ(0, rtc);   // [確認_正常系] - com_util_prompt_edit_ensure_capacity の戻り値が 0 であること。
    EXPECT_EQ(32u, cap); // [確認_正常系] - 4 から 2 倍ずつ拡張されて 32 になること。

    // Cleanup
    std::free(buf);
}

// 上限で頭打ちになることの確認
TEST_F(promptEditTest, ensure_capacity_caps_at_max_bytes)
{
    // Arrange
    char *buf = static_cast<char *>(std::malloc(4u));
    size_t cap = 4u; // [状態] - 4 byte を確保済みのバッファーを用意する。

    ASSERT_NE(nullptr, buf);

    // Pre-Assert

    // Act
    int rtc = com_util_prompt_edit_ensure_capacity(&buf, &cap, 20u,
                                                   20u); // [手順] - 上限 20、必要量 20 を指定して呼び出す。

    // Assert
    EXPECT_EQ(0, rtc);   // [確認_正常系] - com_util_prompt_edit_ensure_capacity の戻り値が 0 であること。
    EXPECT_EQ(20u, cap); // [確認_正常系] - 2 倍では上限を超えるため上限の 20 で頭打ちになること。

    // Cleanup
    std::free(buf);
}

// 必要量が上限を超える場合に拒否されることの確認
TEST_F(promptEditTest, ensure_capacity_rejects_required_over_max)
{
    // Arrange
    char *buf = static_cast<char *>(std::malloc(4u));
    size_t cap = 4u; // [状態] - 4 byte を確保済みのバッファーを用意する。

    ASSERT_NE(nullptr, buf);

    // Pre-Assert

    // Act
    int rtc = com_util_prompt_edit_ensure_capacity(&buf, &cap, 16u,
                                                   17u); // [手順] - 上限 16 を超える必要量 17 を指定して呼び出す。

    // Assert
    EXPECT_EQ(-1, rtc); // [確認_異常系] - com_util_prompt_edit_ensure_capacity の戻り値が -1 であること。
    EXPECT_EQ(4u, cap); // [確認_異常系] - 容量が変化しないこと。

    // Cleanup
    std::free(buf);
}

// buf と cap に NULL を渡した場合に拒否されることの確認
TEST_F(promptEditTest, ensure_capacity_rejects_null_arguments)
{
    // Arrange
    char *buf = NULL;
    size_t cap = 0u;

    // Pre-Assert

    // Act
    int rtc_null_buf = com_util_prompt_edit_ensure_capacity(NULL, &cap, 16u, 8u); // [手順] - buf に NULL を指定する。
    int rtc_null_cap = com_util_prompt_edit_ensure_capacity(&buf, NULL, 16u, 8u); // [手順] - cap に NULL を指定する。

    // Assert
    EXPECT_EQ(-1, rtc_null_buf); // [確認_異常系] - buf が NULL のとき戻り値が -1 であること。
    EXPECT_EQ(-1, rtc_null_cap); // [確認_異常系] - cap が NULL のとき戻り値が -1 であること。
}

/*
 * com_util_prompt_edit_resolve_options
 */

// 0 を指定した項目に既定値が入ることの確認
TEST_F(promptEditTest, resolve_options_applies_defaults_for_zero)
{
    // Arrange
    size_t history_max = 0u;
    size_t initial_capacity = 0u;
    size_t max_bytes = 0u; // [状態] - 解決結果の格納先を用意する。

    // Pre-Assert

    // Act
    com_util_prompt_edit_resolve_options(0u, 0u, 0u, 128u, &history_max, &initial_capacity,
                                         &max_bytes); // [手順] - 要求値をすべて 0、既定初期容量を 128 として呼び出す。

    // Assert
    EXPECT_EQ((size_t)COM_UTIL_PROMPT_HISTORY_DEFAULT, history_max); // [確認_正常系] - 履歴上限に既定値が入ること。
    EXPECT_EQ((size_t)COM_UTIL_PROMPT_INPUT_BYTES_DEFAULT, max_bytes); // [確認_正常系] - 入力上限に既定値が入ること。
    EXPECT_EQ(128u, initial_capacity); // [確認_正常系] - 初期容量に引数で与えた既定値が入ること。
}

// 下限未満の指定が 2 へ引き上げられることの確認
TEST_F(promptEditTest, resolve_options_raises_values_below_minimum)
{
    // Arrange
    size_t history_max = 0u;
    size_t initial_capacity = 0u;
    size_t max_bytes = 0u; // [状態] - 解決結果の格納先を用意する。

    // Pre-Assert

    // Act
    com_util_prompt_edit_resolve_options(4u, 1u, 1u, 128u, &history_max, &initial_capacity,
                                         &max_bytes); // [手順] - 初期容量と入力上限に 1 を指定して呼び出す。

    // Assert
    EXPECT_EQ(4u, history_max);       // [確認_正常系] - 履歴上限は指定値 4 のままであること。
    EXPECT_EQ(2u, max_bytes);         // [確認_正常系] - 入力上限が下限の 2 へ引き上げられること。
    EXPECT_EQ(2u, initial_capacity);  // [確認_正常系] - 初期容量が下限の 2 へ引き上げられること。
}

// 初期容量が入力上限へ丸められることの確認
TEST_F(promptEditTest, resolve_options_clamps_initial_capacity_to_max_bytes)
{
    // Arrange
    size_t history_max = 0u;
    size_t initial_capacity = 0u;
    size_t max_bytes = 0u; // [状態] - 解決結果の格納先を用意する。

    // Pre-Assert

    // Act
    com_util_prompt_edit_resolve_options(4u, 64u, 16u, 128u, &history_max, &initial_capacity,
                                         &max_bytes); // [手順] - 初期容量 64、入力上限 16 を指定して呼び出す。

    // Assert
    EXPECT_EQ(16u, max_bytes);        // [確認_正常系] - 入力上限が指定値 16 であること。
    EXPECT_EQ(16u, initial_capacity); // [確認_正常系] - 初期容量が入力上限の 16 へ丸められること。
}

// 出力先に NULL を渡してもクラッシュしないことの確認
TEST_F(promptEditTest, resolve_options_accepts_null_outputs)
{
    // Arrange

    // Pre-Assert

    // Act
    com_util_prompt_edit_resolve_options(4u, 8u, 16u, 128u, NULL, NULL,
                                         NULL); // [手順] - 出力先をすべて NULL にして呼び出す。

    // Assert
    // [確認_正常系] - クラッシュせずに完了すること。
}
