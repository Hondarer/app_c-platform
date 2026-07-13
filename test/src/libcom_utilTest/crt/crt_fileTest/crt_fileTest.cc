#include <testfw.h>
#include <com_util/crt/file.h>
#include <filesystem>
#include <cstdio>
#include <cstring>
#include <string>

class crt_fileTest : public Test
{
  protected:
    std::string make_path(const char *name)
    {
        std::string root = findWorkspaceRoot();
        std::filesystem::path dir =
            std::filesystem::path(root) / "app/com_util/test/src/libcom_utilTest/crt_fileTest/results";

        std::filesystem::create_directories(dir);
        return (dir / name).generic_string();
    }

    void write_text_file(const std::string &path, const char *text)
    {
#if defined(PLATFORM_LINUX)
        FILE *fp = std::fopen(path.c_str(), "wb");
#elif defined(PLATFORM_WINDOWS)
        FILE *fp = NULL;
        errno_t err = fopen_s(&fp, path.c_str(), "wb");
        ASSERT_EQ(0, err);
#endif /* PLATFORM_ */
        ASSERT_NE((FILE *)NULL, fp);
        ASSERT_EQ(std::strlen(text), std::fwrite(text, 1, std::strlen(text), fp));
        std::fclose(fp);
    }

    std::string read_text_file(const std::string &path)
    {
        FILE *fp = NULL;
#if defined(PLATFORM_LINUX)
        fp = std::fopen(path.c_str(), "rb");
#elif defined(PLATFORM_WINDOWS)
        errno_t err = fopen_s(&fp, path.c_str(), "rb");
        if (err != 0)
        {
            return std::string();
        }
#endif /* PLATFORM_ */
        char buf[128];
        size_t n;
        std::string out;

        if (fp == NULL)
        {
            return std::string();
        }

        while ((n = std::fread(buf, 1, sizeof(buf), fp)) > 0u)
        {
            out.append(buf, n);
        }

        std::fclose(fp);
        return out;
    }
};

// 未オープンのハンドルに対する init と多重 close が安全であることの確認
TEST_F(crt_fileTest, init_and_close_are_safe_for_unopened_handle)
{
    // Arrange
    com_util_file file; // [状態] - 未オープンのファイル ハンドルを用意する。

    // Pre-Assert

    // Act
    com_util_file_init(&file);  // [手順] - com_util_file_init で初期化する。
    com_util_file_close(&file); // [手順] - 未オープンのまま com_util_file_close を呼び出す。
    com_util_file_close(&file); // [手順] - 続けてもう一度 com_util_file_close を呼び出す。

    // Assert
    // [確認_正常系] - クラッシュせずに完了すること。
}

// 追記オープンで既存ファイルのサイズが報告されることの確認
TEST_F(crt_fileTest, append_open_reports_existing_size)
{
    // Arrange
    std::string path = make_path("append_size.log");
    com_util_file file;
    size_t size = 0;

    write_text_file(path, "hello"); // [状態] - 既存ファイルとして 5 バイトの "hello" を書き込んでおく。
    com_util_file_init(&file);

    // Pre-Assert

    // Act
    int rtc_file_open = com_util_file_open(
        &file, path.c_str(),
        COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND |
            COM_UTIL_FILE_OPEN_WRITE_THROUGH); // [手順] - CREATE | APPEND | WRITE_THROUGH でオープンする。

    // Assert
    ASSERT_EQ(0, rtc_file_open); // [確認_正常系] - CREATE | APPEND | WRITE_THROUGH でオープンした結果が 0 であること。
    int rtc_file_get_size =
        com_util_file_get_size(&file, &size); // [手順] - com_util_file_get_size でサイズを取得する。
    ASSERT_EQ(0, rtc_file_get_size); // [確認_正常系] - com_util_file_get_size でサイズを取得した結果が 0 であること。
    EXPECT_EQ((size_t)5, size);                           // [確認_正常系] - 既存サイズ 5 が報告されること。

    com_util_file_close(&file);
    std::remove(path.c_str());
}

// TRUNCATE 付きオープンで既存ファイルが空になることの確認
TEST_F(crt_fileTest, truncate_open_resets_existing_file_size)
{
    // Arrange
    std::string path = make_path("truncate.log");
    com_util_file file;
    size_t size = 99; // [状態] - サイズの受け取り先を初期値 99 とする。

    write_text_file(path, "existing-data"); // [状態] - 既存ファイルとして "existing-data" を書き込んでおく。
    com_util_file_init(&file);

    // Pre-Assert

    // Act
    int rtc_file_open =
        com_util_file_open(&file, path.c_str(),
                           COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_TRUNCATE | COM_UTIL_FILE_OPEN_APPEND |
                               COM_UTIL_FILE_OPEN_WRITE_THROUGH); // [手順] - TRUNCATE を含むフラグでオープンする。

    // Assert
    ASSERT_EQ(0, rtc_file_open); // [確認_正常系] - TRUNCATE を含むフラグでオープンした結果が 0 であること。
    int rtc_file_get_size =
        com_util_file_get_size(&file, &size); // [手順] - com_util_file_get_size でサイズを取得する。
    ASSERT_EQ(0, rtc_file_get_size); // [確認_正常系] - com_util_file_get_size でサイズを取得した結果が 0 であること。
    EXPECT_EQ((size_t)0, size);                         // [確認_正常系] - サイズが 0 に切り詰められていること。

    com_util_file_close(&file);
    EXPECT_EQ(std::string(), read_text_file(path)); // [確認_正常系] - クローズ後のファイル内容が空であること。
    std::remove(path.c_str());
}

// 書き込み内容が永続化され再オープンで追記できることの確認
TEST_F(crt_fileTest, write_persists_buffer_and_allows_reopen)
{
    // Arrange
    std::string path = make_path("write_and_reopen.log");
    com_util_file file;
    size_t size = 0;

    std::remove(path.c_str()); // [状態] - 既存ファイルを削除しておく。
    com_util_file_init(&file);

    // Pre-Assert

    // Act
    int rtc_file_open =
        com_util_file_open(&file, path.c_str(),
                           COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_TRUNCATE | COM_UTIL_FILE_OPEN_APPEND |
                               COM_UTIL_FILE_OPEN_WRITE_THROUGH); // [手順] - 新規作成でオープンする。

    // Assert
    ASSERT_EQ(0, rtc_file_open); // [確認_正常系] - 新規作成でオープンした結果が 0 であること。
    int rtc_file_write = com_util_file_write(&file, "abc", 3); // [手順] - "abc" 3 バイトを書き込む。
    ASSERT_EQ(0, rtc_file_write); // [確認_正常系] - "abc" 3 バイトを書き込んだ結果が 0 であること。
    ASSERT_EQ(0, com_util_file_get_size(&file, &size));
    EXPECT_EQ((size_t)3, size); // [確認_正常系] - 書き込み後のサイズが 3 であること。

    com_util_file_close(&file);

    int rtc_file_open_2 =
        com_util_file_open(&file, path.c_str(),
                           COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND |
                               COM_UTIL_FILE_OPEN_WRITE_THROUGH); // [手順] - クローズ後に追記モードで再オープンする。
    ASSERT_EQ(0, rtc_file_open_2); // [確認_正常系] - クローズ後に追記モードで再オープンした結果が 0 であること。
    int rtc_file_write_2 = com_util_file_write(&file, "def", 3); // [手順] - "def" 3 バイトを追記する。
    ASSERT_EQ(0, rtc_file_write_2); // [確認_正常系] - "def" 3 バイトを追記する場合の戻り値が 0 であること。
    com_util_file_close(&file);

    EXPECT_EQ(std::string("abcdef"),
              read_text_file(path)); // [確認_正常系] - ファイル内容が "abcdef" になっていること。
    std::remove(path.c_str());
}

// 不正な引数で各関数が -1 を返すことの確認
TEST_F(crt_fileTest, invalid_arguments_fail)
{
    // Arrange
    com_util_file file;
    size_t size = 0;

    com_util_file_init(&file); // [状態] - 未オープンのハンドルを初期化して用意する。

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(-1, com_util_file_open(NULL, "x",
                                     COM_UTIL_FILE_OPEN_CREATE)); // [確認_異常系] - open (file NULL) が -1 を返すこと。
    EXPECT_EQ(-1, com_util_file_open(&file, NULL,
                                     COM_UTIL_FILE_OPEN_CREATE)); // [確認_異常系] - open (path NULL) が -1 を返すこと。
    EXPECT_EQ(-1, com_util_file_write(&file, "abc", 3)); // [確認_異常系] - write (未オープン) が -1 を返すこと。
    EXPECT_EQ(-1, com_util_file_get_size(&file, &size)); // [確認_異常系] - get_size (未オープン) が -1 を返すこと。
    EXPECT_EQ(-1, com_util_file_get_size(NULL, &size));  // [確認_異常系] - get_size (file NULL) が -1 を返すこと。
    EXPECT_EQ(-1, com_util_file_get_size(&file, NULL));  // [確認_異常系] - get_size (size NULL) が -1 を返すこと。
}

// ハンドル由来とパス由来のファイル同一性 ID が一致することの確認
TEST_F(crt_fileTest, file_id_matches_between_handle_and_path)
{
    // Arrange
    std::string path = make_path("file_id.log");
    com_util_file file;
    com_util_file_id handle_id;
    com_util_file_id path_id;

    std::remove(path.c_str()); // [状態] - 既存ファイルを削除しておく。
    com_util_file_init(&file);

    // Pre-Assert

    // Act
    int rtc_file_open =
        com_util_file_open(&file, path.c_str(),
                           COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH |
                               COM_UTIL_FILE_OPEN_SHARE_READ | COM_UTIL_FILE_OPEN_SHARE_DELETE |
                               COM_UTIL_FILE_OPEN_SHARE_WRITE); // [手順] - 共有フラグ付きでオープンする。

    // Assert
    ASSERT_EQ(0, rtc_file_open); // [確認_正常系] - 共有フラグ付きでオープンした結果が 0 であること。
    int rtc_file_get_id = com_util_file_get_id(&file, &handle_id); // [手順] - ハンドルから同一性 ID を取得する。
    ASSERT_EQ(0, rtc_file_get_id); // [確認_正常系] - ハンドルから同一性 ID を取得した結果が 0 であること。
    int rtc_file_get_path_id =
        com_util_file_get_path_id(path.c_str(), &path_id); // [手順] - パスから同一性 ID を取得する。
    ASSERT_EQ(0, rtc_file_get_path_id); // [確認_正常系] - パスから同一性 ID を取得した結果が 0 であること。

    /* 同じ実体を指している間は同一性が一致する */
    EXPECT_EQ(handle_id.volume, path_id.volume); // [確認_正常系] - volume が一致すること。
    EXPECT_EQ(handle_id.index, path_id.index);   // [確認_正常系] - index が一致すること。

    com_util_file_close(&file);
    std::remove(path.c_str());
}

// パスが別実体に差し替わった後は同一性 ID が一致しないことの確認
TEST_F(crt_fileTest, file_id_differs_after_path_is_recreated)
{
    // Arrange
    std::string path = make_path("file_id_recreate.log");
    std::string renamed = make_path("file_id_recreate.log.1");
    com_util_file file;
    com_util_file_id handle_id;
    com_util_file_id path_id;

    std::remove(path.c_str());
    std::remove(renamed.c_str()); // [状態] - 既存ファイルを削除しておく。
    com_util_file_init(&file);

    // Pre-Assert

    // Act
    int rtc_file_open =
        com_util_file_open(&file, path.c_str(),
                           COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH |
                               COM_UTIL_FILE_OPEN_SHARE_READ | COM_UTIL_FILE_OPEN_SHARE_DELETE |
                               COM_UTIL_FILE_OPEN_SHARE_WRITE); // [手順] - 共有フラグ付きでオープンする。

    // Assert
    ASSERT_EQ(0, rtc_file_open); // [確認_正常系] - 共有フラグ付きでオープンした結果が 0 であること。
    int rtc_file_get_id = com_util_file_get_id(&file, &handle_id); // [手順] - ハンドルから同一性 ID を取得する。
    ASSERT_EQ(0, rtc_file_get_id); // [確認_正常系] - ハンドルから同一性 ID を取得した結果が 0 であること。

    /* ローテーション相当の操作: path をリネームして同じ path に別ファイルを作る */
    int rtc_rename = std::rename(path.c_str(), renamed.c_str()); // [手順] - ファイルをリネームする。
    ASSERT_EQ(0, rtc_rename); // [確認_正常系] - ファイルをリネームする場合の戻り値が 0 であること。
    write_text_file(path, "recreated");                       // [手順] - 同じパスに別ファイルを作成する。

    int rtc_file_get_path_id =
        com_util_file_get_path_id(path.c_str(), &path_id); // [手順] - 差し替え後のパスから同一性 ID を取得する。
    ASSERT_EQ(0, rtc_file_get_path_id); // [確認_正常系] - 差し替え後のパスから同一性 ID を取得した結果が 0 であること。

    /* path は別実体を指すため、開いているハンドルの同一性とは一致しない */
    EXPECT_FALSE(handle_id.volume == path_id.volume &&
                 handle_id.index == path_id.index); // [確認_正常系] - volume と index の組が一致しないこと。

    com_util_file_close(&file);
    std::remove(path.c_str());
    std::remove(renamed.c_str());
}

// 同一性 ID 取得が不正な引数で -1 を返すことの確認
TEST_F(crt_fileTest, file_id_invalid_arguments_fail)
{
    // Arrange
    com_util_file file;
    com_util_file_id id;

    com_util_file_init(&file); // [状態] - 未オープンのハンドルを初期化して用意する。

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(-1, com_util_file_get_id(&file, &id)); // [確認_異常系] - get_id (未オープンのハンドル) が -1 を返すこと。
    EXPECT_EQ(-1, com_util_file_get_id(NULL, &id));  // [確認_異常系] - get_id (file NULL) が -1 を返すこと。
    EXPECT_EQ(-1, com_util_file_get_path_id(NULL, &id)); // [確認_異常系] - get_path_id (path NULL) が -1 を返すこと。
    EXPECT_EQ(-1, com_util_file_get_path_id("crt_fileTest_no_such_file.log",
                                            &id)); // [確認_異常系] - get_path_id (存在しないパス) が -1 を返すこと。
    EXPECT_EQ(-1, com_util_file_get_path_id("x", NULL)); // [確認_異常系] - get_path_id (id NULL) が -1 を返すこと。
}
