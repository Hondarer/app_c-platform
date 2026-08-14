#include <testfw.h>
#include <com_util/crt/stdio.h>
#include <com_util/runtime/sym_loader.h>
#include <mock_cjson.h>
#include <mock_com_util.h>
#include <mock_stdlib.h>
#include <cstdint>
#include <cstring>
#include <string>

using testing::_;
using testing::DoDefault;
using testing::NiceMock;
using testing::Return;
using testing::StrEq;

class symLoaderInitTest : public Test
{
  protected:
    // MSVC では Mock_com_util 生成により mock_com_util の弱参照 obj がリンクに取り込まれ、
    // com_util_fopen 等の /ALTERNATENAME が有効になる。既定動作は実 libcom_util への委譲。
    NiceMock<Mock_com_util> mock_com_util_;
    FILE *const file_ = reinterpret_cast<FILE *>(static_cast<uintptr_t>(0x70));

    void expect_config_read(const char *path, const char *content)
    {
        const size_t size = std::strlen(content);
        EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq(path), StrEq("rb"), nullptr)).WillOnce(Return(file_));
        EXPECT_CALL(mock_com_util_, com_util_fseek(file_, 0, SEEK_END)).WillOnce(Return(0));
        EXPECT_CALL(mock_com_util_, com_util_ftell(file_)).WillOnce(Return(static_cast<int64_t>(size)));
        EXPECT_CALL(mock_com_util_, com_util_fseek(file_, 0, SEEK_SET)).WillOnce(Return(0));
        EXPECT_CALL(mock_com_util_, com_util_fread(_, 1u, size, file_, nullptr))
            .WillOnce(
                [content, size](void *buffer, size_t, size_t, FILE *, com_util_error *)
                {
                    std::memcpy(buffer, content, size);
                    return size;
                });
        EXPECT_CALL(mock_com_util_, com_util_fclose(file_, nullptr)).WillOnce(Return(0));
    }

    void expect_missing_file(const char *path)
    {
        EXPECT_CALL(mock_com_util_, com_util_fopen(StrEq(path), StrEq("rb"), nullptr)).WillOnce(Return(nullptr));
    }
};

// // および /* */ コメント付き JSON が解析されることの確認
TEST_F(symLoaderInitTest, applies_json_with_comments)
{
    // Arrange
    const char *json = "// header comment\n"
                       "{\n"
                       "  /* block comment */\n"
                       "  \"sample_func\": { // trailing line comment\n"
                       "    \"lib\": \"liboverride\",\n"
                       "    \"func\": \"override_func\"\n"
                       "  }\n"
                       "}\n";
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - コメント付き JSON と sample_func エントリを用意する。

    // Pre-Assert
    expect_config_read("with_comments.json",
                       json); // [Pre-Assert確認_正常系] - コメント付き JSON の読取が呼び出されること。
                              // [Pre-Assert手順] - コメント付き JSON 本文を返却する。

    // Act
    com_util_sym_loader_init(entries, 1u, "with_comments.json"); // [手順] - コメント付き JSON 設定を読み込む。

    // Assert
    EXPECT_STREQ("liboverride", entry.lib_name);    // [確認_正常系] - lib_name が liboverride であること。
    EXPECT_STREQ("override_func", entry.func_name); // [確認_正常系] - func_name が override_func であること。
}

// JSON 設定の lib / func が一致エントリへ反映されることの確認
TEST_F(symLoaderInitTest, applies_matching_func_key)
{
    // Arrange
    const char *json = "{\"sample_func\":{\"lib\":\"liboverride\",\"func\":\"override_func\"}}";
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - sample_func エントリを 1 件用意する。

    // Pre-Assert
    expect_config_read("apply_matching.json",
                       json); // [Pre-Assert確認_正常系] - 一致キー JSON の読取が呼び出されること。
                              // [Pre-Assert手順] - 一致キー JSON 本文を返却する。

    // Act
    com_util_sym_loader_init(entries, 1u, "apply_matching.json"); // [手順] - JSON 設定を読み込む。

    // Assert
    EXPECT_STREQ("liboverride", entry.lib_name);    // [確認_正常系] - lib_name が liboverride であること。
    EXPECT_STREQ("override_func", entry.func_name); // [確認_正常系] - func_name が override_func であること。
}

// 明示的デフォルト (lib/func がともに default) が反映されることの確認
TEST_F(symLoaderInitTest, applies_explicit_default)
{
    // Arrange
    const char *json = "{\"sample_func\":{\"lib\":\"default\",\"func\":\"default\"}}";
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - sample_func エントリを 1 件用意する。

    // Pre-Assert
    expect_config_read("explicit_default.json",
                       json); // [Pre-Assert確認_正常系] - 明示的デフォルト JSON の読取が呼び出されること。
                              // [Pre-Assert手順] - 明示的デフォルト JSON 本文を返却する。

    // Act
    com_util_sym_loader_init(entries, 1u, "explicit_default.json"); // [手順] - 明示的デフォルトの JSON を読み込む。

    // Assert
    EXPECT_STREQ("default", entry.lib_name);  // [確認_正常系] - lib_name が default であること。
    EXPECT_STREQ("default", entry.func_name); // [確認_正常系] - func_name が default であること。
}

// 設定ファイルが存在しない場合にエントリが未設定のままであることの確認
TEST_F(symLoaderInitTest, ignores_missing_file)
{
    // Arrange
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 設定ファイルを置かない。

    // Pre-Assert
    expect_missing_file("missing.json"); // [Pre-Assert確認_正常系] - 存在しない設定の fopen が呼び出されること。
                                         // [Pre-Assert手順] - fopen から NULL を返却する。

    // Act
    com_util_sym_loader_init(entries, 1u, "missing.json"); // [手順] - 存在しないパスで初期化する。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_正常系] - lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_正常系] - func_name が空のままであること。
}

// 不正な JSON を無視してエントリが未設定のままであることの確認
TEST_F(symLoaderInitTest, ignores_invalid_json)
{
    // Arrange
    const char *json = "{not-json";
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 不正 JSON の設定を用意する。

    // Pre-Assert
    expect_config_read("invalid.json", json); // [Pre-Assert確認_正常系] - 不正 JSON の読取が呼び出されること。
                                              // [Pre-Assert手順] - 不正 JSON 本文を返却する。

    // Act
    com_util_sym_loader_init(entries, 1u, "invalid.json"); // [手順] - 不正 JSON を読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_正常系] - lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_正常系] - func_name が空のままであること。
}

// ルートが object でない場合にエントリが未設定のままであることの確認
TEST_F(symLoaderInitTest, ignores_non_object_root)
{
    // Arrange
    const char *json = "[{\"lib\":\"x\",\"func\":\"y\"}]";
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 配列ルートの JSON を用意する。

    // Pre-Assert
    expect_config_read("array_root.json", json); // [Pre-Assert確認_正常系] - 配列ルート JSON の読取が呼び出されること。
                                                 // [Pre-Assert手順] - 配列ルート JSON 本文を返却する。

    // Act
    com_util_sym_loader_init(entries, 1u, "array_root.json"); // [手順] - 配列ルートの JSON を読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_正常系] - lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_正常系] - func_name が空のままであること。
}

// 必須フィールド欠落のエントリを無視することの確認
TEST_F(symLoaderInitTest, ignores_missing_required_fields)
{
    // Arrange
    const char *json = "{\"sample_func\":{\"lib\":\"liboverride\"}}";
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - func 欠落の JSON を用意する。

    // Pre-Assert
    expect_config_read("missing_fields.json",
                       json); // [Pre-Assert確認_正常系] - 必須フィールド欠落 JSON の読取が呼び出されること。
                              // [Pre-Assert手順] - func 欠落の JSON 本文を返却する。

    // Act
    com_util_sym_loader_init(entries, 1u, "missing_fields.json"); // [手順] - 必須フィールド欠落の JSON を読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_正常系] - lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_正常系] - func_name が空のままであること。
}

// 未知の func_key を無視し、一致キーだけ適用することの確認
TEST_F(symLoaderInitTest, ignores_unknown_func_key_and_applies_known)
{
    // Arrange
    const char *json = "{\"unknown_func\":{\"lib\":\"libx\",\"func\":\"fx\"},"
                       "\"sample_func\":{\"lib\":\"liboverride\",\"func\":\"override_func\"}}";
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 既知キーと未知キーを含む JSON を用意する。

    // Pre-Assert
    expect_config_read("partial.json", json); // [Pre-Assert確認_正常系] - 複数キー JSON の読取が呼び出されること。
                                              // [Pre-Assert手順] - 既知キーと未知キーを含む JSON 本文を返却する。

    // Act
    com_util_sym_loader_init(entries, 1u, "partial.json"); // [手順] - 複数キーの JSON を読み込む。

    // Assert
    EXPECT_STREQ("liboverride", entry.lib_name);    // [確認_正常系] - 既知キーの lib_name が反映されること。
    EXPECT_STREQ("override_func", entry.func_name); // [確認_正常系] - 既知キーの func_name が反映されること。
}

// 名称長超過のエントリを無視することの確認
TEST_F(symLoaderInitTest, ignores_name_too_long)
{
    // Arrange
    std::string long_name(COM_UTIL_SYM_LOADER_NAME_MAX, 'a'); // 終端込み MAX バイト分の文字
    std::string json = std::string("{\"sample_func\":{\"lib\":\"") + long_name + "\",\"func\":\"override_func\"}}";
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - lib が上限超過の JSON を用意する。

    // Pre-Assert
    expect_config_read("name_too_long.json",
                       json.c_str()); // [Pre-Assert確認_正常系] - 名称長超過 JSON の読取が呼び出されること。
                                      // [Pre-Assert手順] - 名称長超過 JSON 本文を返却する。

    // Act
    com_util_sym_loader_init(entries, 1u, "name_too_long.json"); // [手順] - 名称長超過の JSON を読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_正常系] - lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_正常系] - func_name が空のままであること。
}

// 複数エントリへそれぞれ対応する設定が反映されることの確認
TEST_F(symLoaderInitTest, applies_multiple_entries)
{
    // Arrange
    const char *json = "{\"sample_func\":{\"lib\":\"liba\",\"func\":\"fa\"},"
                       "\"other_func\":{\"lib\":\"libb\",\"func\":\"fb\"}}";
    com_util_sym_loader_entry entry_a = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry entry_b = COM_UTIL_SYM_LOADER_ENTRY_INIT("other_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry_a, &entry_b}; // [状態] - 2 件のエントリを用意する。

    // Pre-Assert
    expect_config_read("multi.json", json); // [Pre-Assert確認_正常系] - 複数エントリ JSON の読取が呼び出されること。
                                            // [Pre-Assert手順] - 複数エントリ JSON 本文を返却する。

    // Act
    com_util_sym_loader_init(entries, 2u, "multi.json"); // [手順] - 複数エントリ向け JSON を読み込む。

    // Assert
    EXPECT_STREQ("liba", entry_a.lib_name); // [確認_正常系] - sample_func の lib_name が liba であること。
    EXPECT_STREQ("fa", entry_a.func_name);  // [確認_正常系] - sample_func の func_name が fa であること。
    EXPECT_STREQ("libb", entry_b.lib_name); // [確認_正常系] - other_func の lib_name が libb であること。
    EXPECT_STREQ("fb", entry_b.func_name);  // [確認_正常系] - other_func の func_name が fb であること。
}

// cJSON_Parse の失敗を注入した場合に設定を反映しないことの確認
TEST_F(symLoaderInitTest, ignores_document_when_cjson_parse_fails)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    const char *json = "{\"sample_func\":{\"lib\":\"liboverride\",\"func\":\"override_func\"}}";
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 正常な JSON と未設定の sample_func エントリを用意する。

    // Pre-Assert
    expect_config_read("injected_parse_failure.json",
                       json); // [Pre-Assert確認_異常系] - 正常な JSON の読取が呼び出されること。
                              // [Pre-Assert手順] - 正常な JSON 本文を返却する。
    EXPECT_CALL(mock_cjson, cJSON_Parse(StrEq(json)))
        .WillOnce(Return(
            nullptr)); // [Pre-Assert確認_異常系] - cJSON_Parse が正常な JSON 文字列を指定して 1 回呼び出されること。
                       // [Pre-Assert手順] - cJSON_Parse から NULL を返却する。
    EXPECT_CALL(
        mock_cjson,
        cJSON_Delete(nullptr)); // [Pre-Assert確認_異常系] - cJSON_Delete が NULL を指定して 1 回呼び出されること。

    // Act
    com_util_sym_loader_init(
        entries, 1u,
        "injected_parse_failure.json"); // [手順] - cJSON_Parse の失敗を注入して JSON 設定を読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_異常系] - lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_異常系] - func_name が空のままであること。
}

// cJSON の文字列取得が NULL を返した場合に設定を反映しないことの確認
TEST_F(symLoaderInitTest, ignores_entry_when_cjson_string_value_is_null)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    const char *json = "{\"sample_func\":{\"lib\":\"liboverride\",\"func\":\"override_func\"}}";
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 正常な JSON と未設定の sample_func エントリを用意する。

    // Pre-Assert
    expect_config_read("injected_string_failure.json",
                       json); // [Pre-Assert確認_異常系] - 正常な JSON の読取が呼び出されること。
                              // [Pre-Assert手順] - 正常な JSON 本文を返却する。
    EXPECT_CALL(mock_cjson, cJSON_GetStringValue(_))
        .WillOnce(Return(nullptr))
        .WillOnce(
            DoDefault()); // [Pre-Assert確認_異常系] - cJSON_GetStringValue が lib と func で 2 回呼び出されること。
                          // [Pre-Assert手順] - lib の文字列値の取得で NULL を返却する。

    // Act
    com_util_sym_loader_init(
        entries, 1u,
        "injected_string_failure.json"); // [手順] - cJSON_GetStringValue の失敗を注入して設定を読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_異常系] - 文字列取得失敗時に lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_異常系] - 文字列取得失敗時に func_name が空のままであること。
}

// 設定パスが NULL または空文字列の場合に何も行わないことの確認
TEST_F(symLoaderInitTest, ignores_null_or_empty_config_path)
{
    // Arrange
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 未設定のエントリを用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_init(entries, 1u, NULL); // [手順] - NULL の設定パスで初期化する。
    com_util_sym_loader_init(entries, 1u, "");   // [手順] - 空文字列の設定パスで初期化する。

    // Assert
    EXPECT_STREQ("",
                 entry.lib_name); // [確認_正常系] - NULL または空文字列の設定パスで lib_name が未設定のままであること。
    EXPECT_STREQ(
        "", entry.func_name); // [確認_正常系] - NULL または空文字列の設定パスで func_name が未設定のままであること。
}

// ファイル操作の各失敗時にファイルを閉じて終了することの確認
TEST_F(symLoaderInitTest, closes_file_when_seek_or_size_operation_fails)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    FILE *file = reinterpret_cast<FILE *>(1);
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 擬似ファイルと未設定エントリを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("seek_error"), StrEq("rb"), nullptr)).WillOnce(Return(file));
    EXPECT_CALL(mock_com_util, com_util_fseek(file, 0, SEEK_END)).WillOnce(Return(-1));
    EXPECT_CALL(mock_com_util, com_util_fclose(file, nullptr)).WillOnce(Return(0));

    // Act
    com_util_sym_loader_init(entries, 1u, "seek_error"); // [手順] - 末尾への seek が失敗するファイルを読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name); // [確認_異常系] - seek 失敗時に lib_name が未設定のままであること。

    // Cleanup
    Mock::VerifyAndClearExpectations(&mock_com_util);

    // Arrange_2
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("size_error"), StrEq("rb"), nullptr)).WillOnce(Return(file));
    EXPECT_CALL(mock_com_util, com_util_fseek(file, 0, SEEK_END)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util, com_util_ftell(file)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util, com_util_fclose(file, nullptr)).WillOnce(Return(0));

    // Pre-Assert_2

    // Act_2
    com_util_sym_loader_init(entries, 1u, "size_error"); // [手順] - サイズが 0 のファイルを読み込む。

    // Assert_2
    EXPECT_STREQ("", entry.func_name); // [確認_異常系] - サイズ不正時に func_name が未設定のままであること。
}

// 読み込み前の seek、メモリ確保、読み込みの失敗時に後処理することの確認
TEST_F(symLoaderInitTest, releases_resources_when_read_setup_fails)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    NiceMock<Mock_stdlib> mock_stdlib;
    FILE *file = reinterpret_cast<FILE *>(1);
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 擬似ファイルと未設定エントリを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("reset_error"), StrEq("rb"), nullptr)).WillOnce(Return(file));
    EXPECT_CALL(mock_com_util, com_util_fseek(file, 0, SEEK_END)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util, com_util_ftell(file)).WillOnce(Return(4));
    EXPECT_CALL(mock_com_util, com_util_fseek(file, 0, SEEK_SET)).WillOnce(Return(-1));
    EXPECT_CALL(mock_com_util, com_util_fclose(file, nullptr)).WillOnce(Return(0));

    // Act
    com_util_sym_loader_init(entries, 1u,
                             "reset_error"); // [手順] - 読み込み開始位置への seek が失敗するファイルを読み込む。

    // Assert
    EXPECT_STREQ(
        "", entry.lib_name); // [確認_異常系] - 読み込み開始位置の seek 失敗時に lib_name が未設定のままであること。

    // Cleanup
    Mock::VerifyAndClearExpectations(&mock_com_util);

    // Arrange_2
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("alloc_error"), StrEq("rb"), nullptr)).WillOnce(Return(file));
    EXPECT_CALL(mock_com_util, com_util_fseek(file, 0, SEEK_END)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util, com_util_ftell(file)).WillOnce(Return(4));
    EXPECT_CALL(mock_com_util, com_util_fseek(file, 0, SEEK_SET)).WillOnce(Return(0));
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, 5u)).WillOnce(Return(nullptr)).WillRepeatedly(DoDefault());
    EXPECT_CALL(mock_com_util, com_util_fclose(file, nullptr)).WillOnce(Return(0));

    // Pre-Assert_2

    // Act_2
    com_util_sym_loader_init(entries, 1u, "alloc_error"); // [手順] - 設定バッファーの確保が失敗するファイルを読み込む。

    // Assert_2
    EXPECT_STREQ("", entry.func_name); // [確認_異常系] - 確保失敗時に func_name が未設定のままであること。

    // Cleanup
    Mock::VerifyAndClearExpectations(&mock_com_util);

    // Arrange_3
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("read_error"), StrEq("rb"), nullptr)).WillOnce(Return(file));
    EXPECT_CALL(mock_com_util, com_util_fseek(file, 0, SEEK_END)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util, com_util_ftell(file)).WillOnce(Return(4));
    EXPECT_CALL(mock_com_util, com_util_fseek(file, 0, SEEK_SET)).WillOnce(Return(0));
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, 5u)).WillOnce(DoDefault()).WillRepeatedly(DoDefault());
    EXPECT_CALL(mock_com_util, com_util_fread(_, 1u, 4u, file, nullptr)).WillOnce(Return(3u));
    EXPECT_CALL(mock_com_util, com_util_fclose(file, nullptr)).WillOnce(Return(0));

    // Pre-Assert_3

    // Act_3
    com_util_sym_loader_init(entries, 1u, "read_error"); // [手順] - 設定内容の読み込みが短くなるファイルを読み込む。

    // Assert_3
    EXPECT_STREQ("", entry.lib_name); // [確認_異常系] - 読み込み不足時に lib_name が未設定のままであること。
}

// JSON エントリの型、キー、値の境界条件を無視することの確認
TEST_F(symLoaderInitTest, ignores_invalid_json_entries)
{
    // Arrange
    const char *json = "{\"number\":1,\"\":{\"lib\":\"x\",\"func\":\"y\"},"
                       "\"bad_type\":{\"lib\":1,\"func\":\"y\"},"
                       "\"empty_value\":{\"lib\":\"\",\"func\":\"y\"}}";
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("number", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 不正な型、空キー、空値を含む JSON を用意する。

    // Pre-Assert
    expect_config_read("invalid_entries.json",
                       json); // [Pre-Assert確認_異常系] - 不正エントリ JSON の読取が呼び出されること。
                              // [Pre-Assert手順] - 不正エントリ JSON 本文を返却する。

    // Act
    com_util_sym_loader_init(entries, 1u,
                             "invalid_entries.json"); // [手順] - 不正な JSON エントリを含む設定を読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_異常系] - 不正な JSON エントリが lib_name に反映されないこと。
    EXPECT_STREQ("", entry.func_name); // [確認_異常系] - 不正な JSON エントリが func_name に反映されないこと。
}

// 配列が NULL の場合に一致設定を読み飛ばすことの確認
TEST_F(symLoaderInitTest, ignores_matching_entry_when_object_array_is_null)
{
    // Arrange
    const char *json =
        "{\"sample_func\":{\"lib\":\"liboverride\",\"func\":\"override_func\"}}"; // [状態] - 一致する設定を含む JSON を用意する。

    // Pre-Assert
    expect_config_read("null_array.json", json); // [Pre-Assert確認_正常系] - 一致設定 JSON の読取が呼び出されること。
                                                 // [Pre-Assert手順] - 一致設定 JSON 本文を返却する。

    // Act
    com_util_sym_loader_init(NULL, 0u, "null_array.json"); // [手順] - エントリ配列に NULL を指定して設定を読み込む。

    // Assert
    SUCCEED(); // [確認_正常系] - NULL 配列の設定読み込みが異常終了しないこと。
}

// NULL エントリと func_key NULL の配列要素を読み飛ばすことの確認
TEST_F(symLoaderInitTest, skips_null_cache_entries)
{
    // Arrange
    const char *json = "{\"sample_func\":{\"lib\":\"liboverride\",\"func\":\"override_func\"}}";
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    entry.func_key = NULL;
    com_util_sym_loader_entry *entries[] = {NULL, &entry}; // [状態] - NULL 要素と func_key が NULL の要素を用意する。

    // Pre-Assert
    expect_config_read("null_cache_entries.json",
                       json); // [Pre-Assert確認_異常系] - 一致設定 JSON の読取が呼び出されること。
                              // [Pre-Assert手順] - 一致設定 JSON 本文を返却する。

    // Act
    com_util_sym_loader_init(entries, 2u,
                             "null_cache_entries.json"); // [手順] - 不正なキャッシュ配列を指定して設定を読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name); // [確認_異常系] - 不正なキャッシュ要素へ設定が反映されないこと。
}

// 上限超過の設定ファイルを読み込まないことの確認
TEST_F(symLoaderInitTest, ignores_config_file_larger_than_limit)
{
    // Arrange
    NiceMock<Mock_com_util> mock_com_util;
    FILE *file = reinterpret_cast<FILE *>(1);
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry};

    // Pre-Assert
    EXPECT_CALL(mock_com_util, com_util_fopen(StrEq("large_file"), StrEq("rb"), nullptr)).WillOnce(Return(file));
    EXPECT_CALL(mock_com_util, com_util_fseek(file, 0, SEEK_END)).WillOnce(Return(0));
    EXPECT_CALL(mock_com_util, com_util_ftell(file)).WillOnce(Return(1024 * 1024 + 1));
    EXPECT_CALL(mock_com_util, com_util_fclose(file, nullptr)).WillOnce(Return(0));

    // Act
    com_util_sym_loader_init(entries, 1u,
                             "large_file"); // [手順] - 上限を 1 バイト超える設定ファイルを読み込む。

    // Assert
    EXPECT_STREQ("",
                 entry.lib_name); // [確認_異常系] - 上限超過の設定が lib_name に反映されないこと。
}

// func の NULL、空文字列、上限超過をそれぞれ無視することの確認
TEST_F(symLoaderInitTest, ignores_invalid_function_values)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    const char *valid_json = "{\"sample_func\":{\"lib\":\"liboverride\",\"func\":\"override_func\"}}";
    std::string long_func(256u, 'f');
    std::string boundary_json =
        "{\"empty\":{\"lib\":\"lib\",\"func\":\"\"},\"long\":{\"lib\":\"lib\",\"func\":\"" + long_func + "\"}}";
    com_util_sym_loader_entry null_entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry empty_entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("empty", void (*)(void));
    com_util_sym_loader_entry long_entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("long", void (*)(void));
    com_util_sym_loader_entry *null_entries[] = {&null_entry};
    com_util_sym_loader_entry *boundary_entries[] = {&empty_entry, &long_entry};

    // Pre-Assert
    expect_config_read("null_func.json",
                       valid_json); // [Pre-Assert確認_異常系] - func 取得失敗用 JSON の読取が呼び出されること。
                                    // [Pre-Assert手順] - 正常な JSON 本文を返却する。
    EXPECT_CALL(mock_cjson, cJSON_GetStringValue(_))
        .WillOnce(DoDefault())
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - func の文字列取得が 2 回目に呼び出されること。
                                      // [Pre-Assert手順] - func の文字列取得で NULL を返却する。

    // Act
    com_util_sym_loader_init(null_entries, 1u,
                             "null_func.json"); // [手順] - func の文字列取得失敗を注入して設定を読み込む。

    // Assert
    EXPECT_STREQ("", null_entry.func_name); // [確認_異常系] - NULL の func が反映されないこと。

    // Cleanup
    Mock::VerifyAndClearExpectations(&mock_cjson);
    Mock::VerifyAndClearExpectations(&mock_com_util_);

    // Arrange_2

    // Pre-Assert_2
    expect_config_read(
        "invalid_func_values.json",
        boundary_json.c_str()); // [Pre-Assert確認_異常系] - 空または上限超過 func の JSON 読取が呼び出されること。
                                // [Pre-Assert手順] - 境界値 JSON 本文を返却する。

    // Act_2
    com_util_sym_loader_init(boundary_entries, 2u,
                             "invalid_func_values.json"); // [手順] - 空または上限超過の func を含む設定を読み込む。

    // Assert_2
    EXPECT_STREQ("", empty_entry.func_name); // [確認_異常系] - 空の func が反映されないこと。
    EXPECT_STREQ("", long_entry.func_name);  // [確認_異常系] - 上限超過の func が反映されないこと。
}
