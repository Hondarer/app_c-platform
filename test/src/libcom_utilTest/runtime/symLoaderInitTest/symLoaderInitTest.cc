#include <testfw.h>
#include <com_util/crt/stdio.h>
#include <com_util/runtime/sym_loader.h>
#include <mock_cjson.h>
#include <mock_com_util.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

class symLoaderInitTest : public Test
{
  protected:
    // MSVC では Mock_com_util 生成により mock_com_util の弱参照 obj がリンクに取り込まれ、
    // com_util_fopen 等の /ALTERNATENAME が有効になる。既定動作は実 libcom_util への委譲。
    NiceMock<Mock_com_util> mock_com_util_;

    std::string make_path(const char *name)
    {
        std::string root = findWorkspaceRoot();
        std::filesystem::path dir =
            std::filesystem::path(root) / "app/com_util/test/src/libcom_utilTest/runtime/symLoaderInitTest/results";

        std::filesystem::create_directories(dir);
        return (dir / name).generic_string();
    }

    void write_file(const std::string &path, const char *content)
    {
        com_util_remove(path.c_str(), NULL);

        FILE *fp = com_util_fopen(path.c_str(), "wb", NULL);
        ASSERT_NE((FILE *)NULL, fp) << "設定ファイルの作成に失敗しました: " << path;
        std::fwrite(content, 1u, std::strlen(content), fp);
        com_util_fclose(fp, NULL);
    }
};

// // および /* */ コメント付き JSON が解析されることの確認
TEST_F(symLoaderInitTest, applies_json_with_comments)
{
    // Arrange
    std::string path = make_path("with_comments.json");
    write_file(path,
               "// header comment\n"
               "{\n"
               "  /* block comment */\n"
               "  \"sample_func\": { // trailing line comment\n"
               "    \"lib\": \"liboverride\",\n"
               "    \"func\": \"override_func\"\n"
               "  }\n"
               "}\n");
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - コメント付き JSON と sample_func エントリを用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_init(entries, 1u, path.c_str()); // [手順] - コメント付き JSON 設定ファイルを読み込む。

    // Assert
    EXPECT_STREQ("liboverride", entry.lib_name);     // [確認_正常系] - lib_name が liboverride であること。
    EXPECT_STREQ("override_func", entry.func_name); // [確認_正常系] - func_name が override_func であること。

    // Cleanup
    com_util_remove(path.c_str(), NULL);
}

// JSON 設定の lib / func が一致エントリへ反映されることの確認
TEST_F(symLoaderInitTest, applies_matching_func_key)
{
    // Arrange
    std::string path = make_path("apply_matching.json");
    write_file(path, "{\"sample_func\":{\"lib\":\"liboverride\",\"func\":\"override_func\"}}");
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - sample_func エントリを 1 件用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_init(entries, 1u, path.c_str()); // [手順] - JSON 設定ファイルを読み込む。

    // Assert
    EXPECT_STREQ("liboverride", entry.lib_name);     // [確認_正常系] - lib_name が liboverride であること。
    EXPECT_STREQ("override_func", entry.func_name); // [確認_正常系] - func_name が override_func であること。

    // Cleanup
    com_util_remove(path.c_str(), NULL);
}

// 明示的デフォルト (lib/func がともに default) が反映されることの確認
TEST_F(symLoaderInitTest, applies_explicit_default)
{
    // Arrange
    std::string path = make_path("explicit_default.json");
    write_file(path, "{\"sample_func\":{\"lib\":\"default\",\"func\":\"default\"}}");
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - sample_func エントリを 1 件用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_init(entries, 1u, path.c_str()); // [手順] - 明示的デフォルトの JSON を読み込む。

    // Assert
    EXPECT_STREQ("default", entry.lib_name);  // [確認_正常系] - lib_name が default であること。
    EXPECT_STREQ("default", entry.func_name); // [確認_正常系] - func_name が default であること。

    // Cleanup
    com_util_remove(path.c_str(), NULL);
}

// 設定ファイルが存在しない場合にエントリが未設定のままであることの確認
TEST_F(symLoaderInitTest, ignores_missing_file)
{
    // Arrange
    std::string path = make_path("missing.json");
    com_util_remove(path.c_str(), NULL); // [状態] - 設定ファイルを置かない。
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry};

    // Pre-Assert

    // Act
    com_util_sym_loader_init(entries, 1u, path.c_str()); // [手順] - 存在しないパスで初期化する。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_正常系] - lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_正常系] - func_name が空のままであること。
}

// 不正な JSON を無視してエントリが未設定のままであることの確認
TEST_F(symLoaderInitTest, ignores_invalid_json)
{
    // Arrange
    std::string path = make_path("invalid.json");
    write_file(path, "{not-json");
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 不正 JSON の設定ファイルを用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_init(entries, 1u, path.c_str()); // [手順] - 不正 JSON を読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_正常系] - lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_正常系] - func_name が空のままであること。

    // Cleanup
    com_util_remove(path.c_str(), NULL);
}

// ルートが object でない場合にエントリが未設定のままであることの確認
TEST_F(symLoaderInitTest, ignores_non_object_root)
{
    // Arrange
    std::string path = make_path("array_root.json");
    write_file(path, "[{\"lib\":\"x\",\"func\":\"y\"}]");
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 配列ルートの JSON を用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_init(entries, 1u, path.c_str()); // [手順] - 配列ルートの JSON を読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_正常系] - lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_正常系] - func_name が空のままであること。

    // Cleanup
    com_util_remove(path.c_str(), NULL);
}

// 必須フィールド欠落のエントリを無視することの確認
TEST_F(symLoaderInitTest, ignores_missing_required_fields)
{
    // Arrange
    std::string path = make_path("missing_fields.json");
    write_file(path, "{\"sample_func\":{\"lib\":\"liboverride\"}}");
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - func 欠落の JSON を用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_init(entries, 1u, path.c_str()); // [手順] - 必須フィールド欠落の JSON を読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_正常系] - lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_正常系] - func_name が空のままであること。

    // Cleanup
    com_util_remove(path.c_str(), NULL);
}

// 未知の func_key を無視し、一致キーだけ適用することの確認
TEST_F(symLoaderInitTest, ignores_unknown_func_key_and_applies_known)
{
    // Arrange
    std::string path = make_path("partial.json");
    write_file(path,
               "{\"unknown_func\":{\"lib\":\"libx\",\"func\":\"fx\"},"
               "\"sample_func\":{\"lib\":\"liboverride\",\"func\":\"override_func\"}}");
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 既知キーと未知キーを含む JSON を用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_init(entries, 1u, path.c_str()); // [手順] - 複数キーの JSON を読み込む。

    // Assert
    EXPECT_STREQ("liboverride", entry.lib_name);     // [確認_正常系] - 既知キーの lib_name が反映されること。
    EXPECT_STREQ("override_func", entry.func_name); // [確認_正常系] - 既知キーの func_name が反映されること。

    // Cleanup
    com_util_remove(path.c_str(), NULL);
}

// 名称長超過のエントリを無視することの確認
TEST_F(symLoaderInitTest, ignores_name_too_long)
{
    // Arrange
    std::string long_name(COM_UTIL_SYM_LOADER_NAME_MAX, 'a'); // 終端込み MAX バイト分の文字
    std::string json = std::string("{\"sample_func\":{\"lib\":\"") + long_name + "\",\"func\":\"override_func\"}}";
    std::string path = make_path("name_too_long.json");
    write_file(path, json.c_str());
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - lib が上限超過の JSON を用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_init(entries, 1u, path.c_str()); // [手順] - 名称長超過の JSON を読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_正常系] - lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_正常系] - func_name が空のままであること。

    // Cleanup
    com_util_remove(path.c_str(), NULL);
}

// 複数エントリへそれぞれ対応する設定が反映されることの確認
TEST_F(symLoaderInitTest, applies_multiple_entries)
{
    // Arrange
    std::string path = make_path("multi.json");
    write_file(path,
               "{\"sample_func\":{\"lib\":\"liba\",\"func\":\"fa\"},"
               "\"other_func\":{\"lib\":\"libb\",\"func\":\"fb\"}}");
    com_util_sym_loader_entry entry_a = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry entry_b = COM_UTIL_SYM_LOADER_ENTRY_INIT("other_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry_a, &entry_b}; // [状態] - 2 件のエントリを用意する。

    // Pre-Assert

    // Act
    com_util_sym_loader_init(entries, 2u, path.c_str()); // [手順] - 複数エントリ向け JSON を読み込む。

    // Assert
    EXPECT_STREQ("liba", entry_a.lib_name);  // [確認_正常系] - sample_func の lib_name が liba であること。
    EXPECT_STREQ("fa", entry_a.func_name);   // [確認_正常系] - sample_func の func_name が fa であること。
    EXPECT_STREQ("libb", entry_b.lib_name);  // [確認_正常系] - other_func の lib_name が libb であること。
    EXPECT_STREQ("fb", entry_b.func_name);   // [確認_正常系] - other_func の func_name が fb であること。

    // Cleanup
    com_util_remove(path.c_str(), NULL);
}

// cJSON_Parse の失敗を注入した場合に設定を反映しないことの確認
TEST_F(symLoaderInitTest, ignores_document_when_cjson_parse_fails)
{
    // Arrange
    NiceMock<Mock_cjson> mock_cjson;
    const char *json = "{\"sample_func\":{\"lib\":\"liboverride\",\"func\":\"override_func\"}}";
    std::string path = make_path("injected_parse_failure.json");
    write_file(path, json);
    com_util_sym_loader_entry entry = COM_UTIL_SYM_LOADER_ENTRY_INIT("sample_func", void (*)(void));
    com_util_sym_loader_entry *entries[] = {&entry}; // [状態] - 正常な JSON と未設定の sample_func エントリを用意する。

    // Pre-Assert
    EXPECT_CALL(mock_cjson, cJSON_Parse(StrEq(json)))
        .WillOnce(Return(
            nullptr)); // [Pre-Assert確認_異常系] - cJSON_Parse が正常な JSON 文字列を指定して 1 回呼び出されること。
                       // [Pre-Assert手順] - cJSON_Parse から NULL を返却する。
    EXPECT_CALL(
        mock_cjson,
        cJSON_Delete(nullptr)); // [Pre-Assert確認_異常系] - cJSON_Delete が NULL を指定して 1 回呼び出されること。

    // Act
    com_util_sym_loader_init(entries, 1u,
                             path.c_str()); // [手順] - cJSON_Parse の失敗を注入して JSON 設定ファイルを読み込む。

    // Assert
    EXPECT_STREQ("", entry.lib_name);  // [確認_異常系] - lib_name が空のままであること。
    EXPECT_STREQ("", entry.func_name); // [確認_異常系] - func_name が空のままであること。

    // Cleanup
    com_util_remove(path.c_str(), NULL);
}
