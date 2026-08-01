#include <testfw.h>
#include <com_util/base/result.h>
#include <com_util/crt/file.h>
#include <com_util/crt/stdio.h>
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
    com_util_file_close(&file, NULL); // [手順] - 未オープンのまま com_util_file_close を呼び出す。
    com_util_file_close(&file, NULL); // [手順] - 続けてもう一度 com_util_file_close を呼び出す。

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
        &file, path.c_str(), COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH,
        NULL); // [手順] - CREATE | APPEND | WRITE_THROUGH でオープンする。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_open); // [確認_正常系] - CREATE | APPEND | WRITE_THROUGH でオープンした com_util_file_open の戻り値が COM_UTIL_OK であること。
    int rtc_file_get_size =
        com_util_file_get_size(&file, &size, NULL); // [手順] - com_util_file_get_size でサイズを取得する。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_get_size); // [確認_正常系] - com_util_file_get_size でサイズを取得した結果が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)5, size); // [確認_正常系] - 既存サイズ 5 が報告されること。

    // Cleanup
    com_util_file_close(&file, NULL);
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
    int rtc_file_open = com_util_file_open(&file, path.c_str(),
                                           COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_TRUNCATE |
                                               COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH,
                                           NULL); // [手順] - TRUNCATE を含むフラグでオープンする。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_open); // [確認_正常系] - TRUNCATE を含むフラグでオープンした com_util_file_open の戻り値が COM_UTIL_OK であること。
    int rtc_file_get_size =
        com_util_file_get_size(&file, &size, NULL); // [手順] - com_util_file_get_size でサイズを取得する。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_get_size); // [確認_正常系] - com_util_file_get_size でサイズを取得した結果が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)0, size); // [確認_正常系] - サイズが 0 に切り詰められていること。

    com_util_file_close(&file, NULL);
    EXPECT_EQ(std::string(), read_text_file(path)); // [確認_正常系] - クローズ後のファイル内容が空であること。

    // Cleanup
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
    int rtc_file_open = com_util_file_open(&file, path.c_str(),
                                           COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_TRUNCATE |
                                               COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH,
                                           NULL); // [手順] - 新規作成でオープンする。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_open); // [確認_正常系] - 新規作成でオープンした com_util_file_open の戻り値が COM_UTIL_OK であること。
    int rtc_file_write = com_util_file_write(&file, "abc", 3, NULL); // [手順] - "abc" 3 バイトを書き込む。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_write); // [確認_正常系] - "abc" 3 バイトを書き込んだ com_util_file_write の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_file_get_size(&file, &size, NULL));
    EXPECT_EQ((size_t)3, size); // [確認_正常系] - 書き込み後のサイズが 3 であること。

    com_util_file_close(&file, NULL);

    int rtc_file_open_2 = com_util_file_open(
        &file, path.c_str(), COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH,
        NULL); // [手順] - クローズ後に追記モードで再オープンする。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_open_2); // [確認_正常系] - クローズ後に追記モードで再オープンした com_util_file_open の戻り値が COM_UTIL_OK であること。
    int rtc_file_write_2 = com_util_file_write(&file, "def", 3, NULL); // [手順] - "def" 3 バイトを追記する。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_write_2); // [確認_正常系] - "def" 3 バイトを追記した com_util_file_write の戻り値が COM_UTIL_OK であること。
    com_util_file_close(&file, NULL);

    EXPECT_EQ(std::string("abcdef"),
              read_text_file(path)); // [確認_正常系] - ファイル内容が "abcdef" になっていること。

    // Cleanup
    std::remove(path.c_str());
}

// 不正な引数で各関数が COM_UTIL_ERR_INVALID_ARGUMENT を返すことの確認
TEST_F(crt_fileTest, invalid_arguments_fail)
{
    // Arrange
    com_util_file file;
    size_t size = 0;

    com_util_file_init(&file); // [状態] - 未オープンのハンドルを初期化して用意する。

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_file_open(NULL, "x", COM_UTIL_FILE_OPEN_CREATE,
                           NULL)); // [確認_異常系] - open (file NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_file_open(&file, NULL, COM_UTIL_FILE_OPEN_CREATE,
                           NULL)); // [確認_異常系] - open (path NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_file_write(&file, "abc", 3,
                            NULL)); // [確認_異常系] - write (未オープン) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_file_get_size(
            &file, &size, NULL)); // [確認_異常系] - get_size (未オープン) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_file_get_size(
            NULL, &size, NULL)); // [確認_異常系] - get_size (file NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_file_get_size(
            &file, NULL, NULL)); // [確認_異常系] - get_size (size NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
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
    int rtc_file_open = com_util_file_open(
        &file, path.c_str(), COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH,
        NULL); // [手順] - オープンする (常時フル共有のため共有フラグ指定は不要)。

    // Assert
    ASSERT_EQ(COM_UTIL_OK, rtc_file_open); // [確認_正常系] - com_util_file_open の戻り値が COM_UTIL_OK であること。
    int rtc_file_get_id = com_util_file_get_id(&file, &handle_id, NULL); // [手順] - ハンドルから同一性 ID を取得する。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_get_id); // [確認_正常系] - ハンドルから同一性 ID を取得した com_util_file_get_id の戻り値が COM_UTIL_OK であること。
    int rtc_file_get_path_id =
        com_util_file_get_path_id(path.c_str(), &path_id, NULL); // [手順] - パスから同一性 ID を取得する。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_get_path_id); // [確認_正常系] - パスから同一性 ID を取得した com_util_file_get_path_id の戻り値が COM_UTIL_OK であること。

    /* 同じ実体を指している間は同一性が一致する */
    EXPECT_EQ(handle_id.volume, path_id.volume); // [確認_正常系] - volume が一致すること。
    EXPECT_EQ(handle_id.index, path_id.index);   // [確認_正常系] - index が一致すること。

    // Cleanup
    com_util_file_close(&file, NULL);
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
    int rtc_file_open = com_util_file_open(
        &file, path.c_str(), COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH,
        NULL); // [手順] - オープンする (常時フル共有のため共有フラグ指定は不要)。

    // Assert
    ASSERT_EQ(COM_UTIL_OK, rtc_file_open); // [確認_正常系] - com_util_file_open の戻り値が COM_UTIL_OK であること。
    int rtc_file_get_id = com_util_file_get_id(&file, &handle_id, NULL); // [手順] - ハンドルから同一性 ID を取得する。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_get_id); // [確認_正常系] - ハンドルから同一性 ID を取得した com_util_file_get_id の戻り値が COM_UTIL_OK であること。

    /* ローテーション相当の操作: path をリネームして同じ path に別ファイルを作る */
    int rtc_rename = std::rename(path.c_str(), renamed.c_str()); // [手順] - ファイルをリネームする。
    ASSERT_EQ(COM_UTIL_OK, rtc_rename); // [確認_正常系] - ファイルをリネームする場合の戻り値が 0 であること。
    write_text_file(path, "recreated"); // [手順] - 同じパスに別ファイルを作成する。

    int rtc_file_get_path_id =
        com_util_file_get_path_id(path.c_str(), &path_id, NULL); // [手順] - 差し替え後のパスから同一性 ID を取得する。
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_get_path_id); // [確認_正常系] - 差し替え後のパスから同一性 ID を取得した com_util_file_get_path_id の戻り値が COM_UTIL_OK であること。

    /* path は別実体を指すため、開いているハンドルの同一性とは一致しない */
    EXPECT_FALSE(handle_id.volume == path_id.volume &&
                 handle_id.index == path_id.index); // [確認_正常系] - volume と index の組が一致しないこと。

    // Cleanup
    com_util_file_close(&file, NULL);
    std::remove(path.c_str());
    std::remove(renamed.c_str());
}

// 同一性 ID 取得が不正な引数で COM_UTIL_ERR_INVALID_ARGUMENT を、存在しないパスで COM_UTIL_ERR_UNKNOWN を返すことの確認
TEST_F(crt_fileTest, file_id_invalid_arguments_fail)
{
    // Arrange
    com_util_file file;
    com_util_file_id id;

    com_util_file_init(&file); // [状態] - 未オープンのハンドルを初期化して用意する。

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_file_get_id(
                  &file, &id,
                  NULL)); // [確認_異常系] - get_id (未オープンのハンドル) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              com_util_file_get_id(
                  NULL, &id, NULL)); // [確認_異常系] - get_id (file NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_file_get_path_id(
            NULL, &id, NULL)); // [確認_異常系] - get_path_id (path NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              com_util_file_get_path_id(
                  "crt_fileTest_no_such_file.log", &id,
                  NULL)); // [確認_異常系] - get_path_id (存在しないパス) が COM_UTIL_ERR_UNKNOWN を返すこと。
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_file_get_path_id(
            "x", NULL, NULL)); // [確認_異常系] - get_path_id (id NULL) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
}

// READ/WRITE フラグを指定しない場合に、既定で書き込み専用としてオープンすることの確認
TEST_F(crt_fileTest, default_access_remains_write_only)
{
    // Arrange
    std::string path = make_path("default_access.log");
    com_util_file file;

    std::remove(path.c_str()); // [状態] - 既存ファイルを削除しておく。
    com_util_file_init(&file);

    // Pre-Assert

    // Act
    int rtc_file_open = com_util_file_open(&file, path.c_str(), COM_UTIL_FILE_OPEN_CREATE,
                                           NULL); // [手順] - READ/WRITE を指定せず CREATE のみでオープンする。
    int rtc_file_write = com_util_file_write(&file, "abc", 3, NULL); // [手順] - "abc" 3 バイトを書き込む。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_open); // [確認_正常系] - READ/WRITE 無指定で呼び出した com_util_file_open の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, rtc_file_write); // [確認_正常系] - 既定 (書き込み専用) で書き込みが成功すること。

    // Cleanup
    com_util_file_close(&file, NULL);
    std::remove(path.c_str());
}

// COM_UTIL_FILE_OPEN_READ のみを指定した場合に書き込みが失敗することの確認 (読み取り専用オープン)
TEST_F(crt_fileTest, read_only_open_rejects_write)
{
    // Arrange
    std::string path = make_path("read_only.log");
    com_util_file file;

    write_text_file(path, "hello"); // [状態] - 既存ファイルとして "hello" を書き込んでおく。
    com_util_file_init(&file);

    // Pre-Assert

    // Act
    int rtc_file_open = com_util_file_open(&file, path.c_str(), COM_UTIL_FILE_OPEN_READ,
                                           NULL); // [手順] - COM_UTIL_FILE_OPEN_READ のみでオープンする。
    int rtc_file_write = com_util_file_write(&file, "x", 1, NULL); // [手順] - 1 バイトの書き込みを試みる。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_open); // [確認_正常系] - 読み取り専用で呼び出した com_util_file_open の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        rtc_file_write); // [確認_異常系] - 読み取り専用ハンドルへの com_util_file_write の戻り値が COM_UTIL_ERR_UNKNOWN であること。

    // Cleanup
    com_util_file_close(&file, NULL);
    std::remove(path.c_str());
}

// COM_UTIL_FILE_OPEN_READ が存在しないファイルに対して失敗することの確認 (CREATE を伴わない)
TEST_F(crt_fileTest, read_only_open_fails_for_missing_file)
{
    // Arrange
    std::string path = make_path("read_only_missing.log");
    com_util_file file;

    std::remove(path.c_str()); // [状態] - 対象ファイルが存在しないことを保証する。
    com_util_file_init(&file);

    // Pre-Assert

    // Act
    int rtc_file_open = com_util_file_open(&file, path.c_str(), COM_UTIL_FILE_OPEN_READ,
                                           NULL); // [手順] - CREATE を伴わずに READ のみでオープンを試みる。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc_file_open); // [確認_異常系] - 存在しないファイルに対するオープンが COM_UTIL_ERR_UNKNOWN を返すこと。
}

// READ | WRITE を指定した場合に読み書き両用でオープンできることの確認
TEST_F(crt_fileTest, read_write_open_allows_write_and_reports_size)
{
    // Arrange
    std::string path = make_path("read_write.log");
    com_util_file file;
    size_t size = 0;

    std::remove(path.c_str()); // [状態] - 既存ファイルを削除しておく。
    com_util_file_init(&file);

    // Pre-Assert

    // Act
    int rtc_file_open = com_util_file_open(
        &file, path.c_str(), COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE,
        NULL); // [手順] - CREATE | READ | WRITE でオープンする。
    int rtc_file_write = com_util_file_write(&file, "abcde", 5, NULL);  // [手順] - "abcde" 5 バイトを書き込む。
    int rtc_file_get_size = com_util_file_get_size(&file, &size, NULL); // [手順] - サイズを取得する。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_file_open); // [確認_正常系] - CREATE | READ | WRITE で呼び出した com_util_file_open の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_OK, rtc_file_write); // [確認_正常系] - 読み書き両用ハンドルへの書き込みが成功すること。
    ASSERT_EQ(COM_UTIL_OK,
              rtc_file_get_size); // [確認_正常系] - com_util_file_get_size の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)5, size);   // [確認_正常系] - 書き込み後のサイズが 5 であること。

    // Cleanup
    com_util_file_close(&file, NULL);
    std::remove(path.c_str());
}

// CREATE | CREATE_NEW で新規ファイルの作成に成功することの確認
TEST_F(crt_fileTest, create_new_succeeds_for_absent_file)
{
    // Arrange
    std::string path = make_path("create_new_absent.log");
    com_util_file file;

    std::remove(path.c_str()); // [状態] - 対象ファイルが存在しないことを保証する。
    com_util_file_init(&file);

    // Pre-Assert

    // Act
    int rtc_file_open = com_util_file_open(&file, path.c_str(),
                                           COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_CREATE_NEW |
                                               COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE,
                                           NULL); // [手順] - CREATE | CREATE_NEW | READ | WRITE でオープンする。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc_file_open); // [確認_正常系] - 存在しないファイルへの CREATE_NEW オープンが成功すること。

    // Cleanup
    com_util_file_close(&file, NULL);
    std::remove(path.c_str());
}

// CREATE | CREATE_NEW が既存ファイルに対して失敗することの確認
TEST_F(crt_fileTest, create_new_fails_for_existing_file)
{
    // Arrange
    std::string path = make_path("create_new_existing.log");
    com_util_file file;

    write_text_file(path, "existing"); // [状態] - 既存ファイルとして "existing" を書き込んでおく。
    com_util_file_init(&file);

    // Pre-Assert

    // Act
    int rtc_file_open = com_util_file_open(
        &file, path.c_str(),
        COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_CREATE_NEW | COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE,
        NULL); // [手順] - 既存ファイルに対して CREATE | CREATE_NEW でオープンを試みる。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_UNKNOWN,
        rtc_file_open); // [確認_異常系] - 既存ファイルに対する CREATE_NEW オープンが COM_UTIL_ERR_UNKNOWN を返すこと。

    // Cleanup
    std::remove(path.c_str());
}

// CREATE_NEW を CREATE なしで指定すると COM_UTIL_ERR_INVALID_ARGUMENT を返すことの確認
TEST_F(crt_fileTest, create_new_without_create_fails)
{
    // Arrange
    std::string path = make_path("create_new_without_create.log");
    com_util_file file;

    std::remove(path.c_str()); // [状態] - 対象ファイルが存在しないことを保証する。
    com_util_file_init(&file);

    // Pre-Assert

    // Act
    int rtc_file_open = com_util_file_open(
        &file, path.c_str(), COM_UTIL_FILE_OPEN_CREATE_NEW | COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE,
        NULL); // [手順] - CREATE を指定せず CREATE_NEW | READ | WRITE でオープンを試みる。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_file_open); // [確認_異常系] - CREATE なしの CREATE_NEW オープンが COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
}

// com_util_file_set_size がファイル サイズを拡張・縮小できることの確認 (マルチ フェーズ テスト)
TEST_F(crt_fileTest, set_size_extends_and_truncates_file)
{
    // Arrange
    std::string path = make_path("set_size.log");
    com_util_file file;
    size_t size = 0;

    std::remove(path.c_str()); // [状態] - 既存ファイルを削除しておく。
    com_util_file_init(&file);
    ASSERT_EQ(COM_UTIL_OK,
              com_util_file_open(&file, path.c_str(),
                                 COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_READ | COM_UTIL_FILE_OPEN_WRITE,
                                 NULL)); // [状態] - CREATE | READ | WRITE でオープンする。

    // Pre-Assert

    // Act
    int rtc_set_size_1 = com_util_file_set_size(&file, 128, NULL); // [手順] - サイズを 128 バイトへ拡張する。

    // Assert
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_set_size_1); // [確認_正常系] - 128 バイトへ拡張する com_util_file_set_size の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_file_get_size(&file, &size, NULL));
    EXPECT_EQ((size_t)128, size); // [確認_正常系] - サイズが 128 バイトへ拡張されていること。

    // Act_2
    int rtc_set_size_2 = com_util_file_set_size(&file, 16, NULL); // [手順] - サイズを 16 バイトへ縮小する。

    // Assert_2
    ASSERT_EQ(
        COM_UTIL_OK,
        rtc_set_size_2); // [確認_正常系] - 16 バイトへ縮小する com_util_file_set_size の戻り値が COM_UTIL_OK であること。
    ASSERT_EQ(COM_UTIL_OK, com_util_file_get_size(&file, &size, NULL));
    EXPECT_EQ((size_t)16, size); // [確認_正常系] - サイズが 16 バイトへ縮小されていること。

    // Cleanup
    com_util_file_close(&file, NULL);
    std::remove(path.c_str());
}

// com_util_file_set_size が不正な引数で COM_UTIL_ERR_INVALID_ARGUMENT を返すことの確認
TEST_F(crt_fileTest, set_size_invalid_arguments_fail)
{
    // Arrange
    com_util_file file;

    com_util_file_init(&file); // [状態] - 未オープンのハンドルを初期化して用意する。

    // Pre-Assert

    // Act

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        com_util_file_set_size(
            &file, 16,
            NULL)); // [確認_異常系] - set_size (未オープンのハンドル) が COM_UTIL_ERR_INVALID_ARGUMENT を返すこと。
}

// com_util_file_read が書き込んだ内容を読み取れることの確認
TEST_F(crt_fileTest, read_returns_written_content)
{
    // Arrange
    com_util_file file;
    std::string path = make_path("read_content.bin");
    char buf[16];
    size_t read_len = 0;

    com_util_file_init(&file);
    write_text_file(path, "abcde"); // [状態] - 内容が "abcde" (5 バイト) のファイルを用意する。
    memset(buf, 0, sizeof(buf));

    // Pre-Assert
    ASSERT_EQ(COM_UTIL_OK, com_util_file_open(&file, path.c_str(), COM_UTIL_FILE_OPEN_READ, NULL));

    // Act
    int rtc_read =
        com_util_file_read(&file, buf, sizeof(buf), &read_len, NULL); // [手順] - 16 バイトを要求して読み取る。

    // Assert
    EXPECT_EQ(COM_UTIL_OK, rtc_read); // [確認_正常系] - com_util_file_read の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)5, read_len);   // [確認_正常系] - 読み取ったバイト数が 5 であること。
    EXPECT_STREQ("abcde", buf);       // [確認_正常系] - 読み取った内容が "abcde" であること。

    // Cleanup
    com_util_file_close(&file, NULL);
    std::remove(path.c_str());
}

// com_util_file_read がファイル終端で 0 バイトを返すことの確認
TEST_F(crt_fileTest, read_at_end_of_file_returns_zero_length)
{
    // Arrange
    com_util_file file;
    std::string path = make_path("read_eof.bin");
    char buf[16];
    size_t read_len = 0;

    com_util_file_init(&file);
    write_text_file(path, "ab"); // [状態] - 内容が "ab" (2 バイト) のファイルを用意する。
    memset(buf, 0, sizeof(buf));

    // Pre-Assert
    ASSERT_EQ(COM_UTIL_OK, com_util_file_open(&file, path.c_str(), COM_UTIL_FILE_OPEN_READ, NULL));
    ASSERT_EQ(COM_UTIL_OK, com_util_file_read(&file, buf, sizeof(buf), &read_len, NULL));
    ASSERT_EQ((size_t)2, read_len);

    // Act
    int rtc_read_eof =
        com_util_file_read(&file, buf, sizeof(buf), &read_len, NULL); // [手順] - 終端到達後に再度読み取りを行う。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              rtc_read_eof); // [確認_正常系] - 終端到達後の com_util_file_read の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ((size_t)0, read_len); // [確認_正常系] - 読み取ったバイト数が 0 であること。

    // Cleanup
    com_util_file_close(&file, NULL);
    std::remove(path.c_str());
}

// com_util_file_read が不正な引数で COM_UTIL_ERR_INVALID_ARGUMENT を返すことの確認
TEST_F(crt_fileTest, read_invalid_arguments_fail)
{
    // Arrange
    com_util_file file;
    char buf[16];
    size_t read_len = 0;

    com_util_file_init(&file); // [状態] - 未オープンのハンドルを初期化して用意する。
    memset(buf, 0, sizeof(buf));

    // Pre-Assert

    // Act
    int rtc_not_open =
        com_util_file_read(&file, buf, sizeof(buf), &read_len, NULL); // [手順] - 未オープンのハンドルで読み取りを行う。

    // Assert
    EXPECT_EQ(
        COM_UTIL_ERR_INVALID_ARGUMENT,
        rtc_not_open); // [確認_異常系] - 未オープンのハンドルを渡した com_util_file_read の戻り値が COM_UTIL_ERR_INVALID_ARGUMENT であること。
}

// com_util_file_flush が書き込み済みファイルを永続記憶装置へ反映することの確認
TEST_F(crt_fileTest, flush_reports_success)
{
    // Arrange
    com_util_file file;
    com_util_error detail;
    std::string path = make_path("flush.bin");
    const char data[] = "data";

    com_util_file_init(&file);

    // Pre-Assert
    ASSERT_EQ(COM_UTIL_OK,
              com_util_file_open(&file, path.c_str(), COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_TRUNCATE, NULL));
    ASSERT_EQ(COM_UTIL_OK, com_util_file_write(&file, data, sizeof(data), NULL));

    // Act
    int result = com_util_file_flush(&file, &detail); // [手順] - 書き込み済みファイルを永続記憶装置へ反映する。

    // Assert
    EXPECT_EQ(COM_UTIL_OK,
              result); // [確認_正常系] - com_util_file_flush の戻り値が COM_UTIL_OK であること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_NONE,
              com_util_error_get_domain(&detail)); // [確認_正常系] - detail が空の値であること。

    // Cleanup
    com_util_file_close(&file, NULL);
    std::remove(path.c_str());
}

// FILE ラッパーが読み書き、反映、クローズの成功を報告することの確認
TEST_F(crt_fileTest, stdio_wrappers_report_success)
{
    // Arrange
    std::string path = make_path("stdio_wrappers.bin");
    FILE *stream = NULL;
    com_util_error detail;
    char read_buffer[5] = {0};
    const char write_buffer[] = "data";
#if defined(PLATFORM_WINDOWS)
    errno_t open_result;
#endif /* PLATFORM_WINDOWS */

    std::remove(path.c_str());
#if defined(PLATFORM_LINUX)
    stream = std::fopen(path.c_str(), "w+b");
#elif defined(PLATFORM_WINDOWS)
    open_result = fopen_s(&stream, path.c_str(), "w+b");
#endif /* PLATFORM_ */

    // Pre-Assert
    ASSERT_NE(nullptr, stream);
#if defined(PLATFORM_WINDOWS)
    ASSERT_EQ(0, open_result);
#endif /* PLATFORM_WINDOWS */

    // Act
    size_t written = com_util_fwrite(write_buffer, 1u, 4u, stream, &detail); // [手順] - 4 バイトを書き込む。

    // Assert
    EXPECT_EQ(4u, written); // [確認_正常系] - com_util_fwrite の戻り値が 4 であること。
    EXPECT_EQ(COM_UTIL_ERROR_DOMAIN_NONE, com_util_error_get_domain(&detail));

    // Act_2
    int flush_result = com_util_fflush(stream, &detail); // [手順] - ストリームのバッファーを反映する。
    ASSERT_EQ(0, com_util_fseek(stream, 0, SEEK_SET));
    size_t read_count = com_util_fread(read_buffer, 1u, 4u, stream, &detail); // [手順] - 4 バイトを読み取る。

    // Assert_2
    EXPECT_EQ(0, flush_result);        // [確認_正常系] - com_util_fflush の戻り値が 0 であること。
    EXPECT_EQ(4u, read_count);         // [確認_正常系] - com_util_fread の戻り値が 4 であること。
    EXPECT_STREQ("data", read_buffer); // [確認_正常系] - 読み取った内容が "data" であること。

    // Act_3
    int close_result = com_util_fclose(stream, &detail); // [手順] - ストリームを閉じる。

    // Assert_3
    EXPECT_EQ(0, close_result); // [確認_正常系] - com_util_fclose の戻り値が 0 であること。

    // Cleanup
    std::remove(path.c_str());
}
