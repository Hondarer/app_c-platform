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

TEST_F(crt_fileTest, init_and_close_are_safe_for_unopened_handle)
{
    com_util_file file;

    com_util_file_init(&file);
    com_util_file_close(&file);
    com_util_file_close(&file);
}

TEST_F(crt_fileTest, append_open_reports_existing_size)
{
    std::string path = make_path("append_size.log");
    com_util_file file;
    size_t size = 0;

    write_text_file(path, "hello");
    com_util_file_init(&file);

    ASSERT_EQ(0, com_util_file_open(&file, path.c_str(),
                                    COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND |
                                        COM_UTIL_FILE_OPEN_WRITE_THROUGH));
    ASSERT_EQ(0, com_util_file_get_size(&file, &size));
    EXPECT_EQ((size_t)5, size);

    com_util_file_close(&file);
    std::remove(path.c_str());
}

TEST_F(crt_fileTest, truncate_open_resets_existing_file_size)
{
    std::string path = make_path("truncate.log");
    com_util_file file;
    size_t size = 99;

    write_text_file(path, "existing-data");
    com_util_file_init(&file);

    ASSERT_EQ(0, com_util_file_open(&file, path.c_str(),
                                    COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_TRUNCATE |
                                        COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH));
    ASSERT_EQ(0, com_util_file_get_size(&file, &size));
    EXPECT_EQ((size_t)0, size);

    com_util_file_close(&file);
    EXPECT_EQ(std::string(), read_text_file(path));
    std::remove(path.c_str());
}

TEST_F(crt_fileTest, write_persists_buffer_and_allows_reopen)
{
    std::string path = make_path("write_and_reopen.log");
    com_util_file file;
    size_t size = 0;

    std::remove(path.c_str());
    com_util_file_init(&file);

    ASSERT_EQ(0, com_util_file_open(&file, path.c_str(),
                                    COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_TRUNCATE |
                                        COM_UTIL_FILE_OPEN_APPEND | COM_UTIL_FILE_OPEN_WRITE_THROUGH));
    ASSERT_EQ(0, com_util_file_write(&file, "abc", 3));
    ASSERT_EQ(0, com_util_file_get_size(&file, &size));
    EXPECT_EQ((size_t)3, size);

    com_util_file_close(&file);

    ASSERT_EQ(0, com_util_file_open(&file, path.c_str(),
                                    COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND |
                                        COM_UTIL_FILE_OPEN_WRITE_THROUGH));
    ASSERT_EQ(0, com_util_file_write(&file, "def", 3));
    com_util_file_close(&file);

    EXPECT_EQ(std::string("abcdef"), read_text_file(path));
    std::remove(path.c_str());
}

TEST_F(crt_fileTest, invalid_arguments_fail)
{
    com_util_file file;
    size_t size = 0;

    com_util_file_init(&file);

    EXPECT_EQ(-1, com_util_file_open(NULL, "x", COM_UTIL_FILE_OPEN_CREATE));
    EXPECT_EQ(-1, com_util_file_open(&file, NULL, COM_UTIL_FILE_OPEN_CREATE));
    EXPECT_EQ(-1, com_util_file_write(&file, "abc", 3));
    EXPECT_EQ(-1, com_util_file_get_size(&file, &size));
    EXPECT_EQ(-1, com_util_file_get_size(NULL, &size));
    EXPECT_EQ(-1, com_util_file_get_size(&file, NULL));
}

TEST_F(crt_fileTest, file_id_matches_between_handle_and_path)
{
    std::string path = make_path("file_id.log");
    com_util_file file;
    com_util_file_id handle_id;
    com_util_file_id path_id;

    std::remove(path.c_str());
    com_util_file_init(&file);

    ASSERT_EQ(0, com_util_file_open(&file, path.c_str(),
                                    COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND |
                                        COM_UTIL_FILE_OPEN_WRITE_THROUGH | COM_UTIL_FILE_OPEN_SHARE_READ |
                                        COM_UTIL_FILE_OPEN_SHARE_DELETE | COM_UTIL_FILE_OPEN_SHARE_WRITE));
    ASSERT_EQ(0, com_util_file_get_id(&file, &handle_id));
    ASSERT_EQ(0, com_util_file_get_path_id(path.c_str(), &path_id));

    // 同じ実体を指している間は同一性が一致する
    EXPECT_EQ(handle_id.volume, path_id.volume);
    EXPECT_EQ(handle_id.index, path_id.index);

    com_util_file_close(&file);
    std::remove(path.c_str());
}

TEST_F(crt_fileTest, file_id_differs_after_path_is_recreated)
{
    std::string path = make_path("file_id_recreate.log");
    std::string renamed = make_path("file_id_recreate.log.1");
    com_util_file file;
    com_util_file_id handle_id;
    com_util_file_id path_id;

    std::remove(path.c_str());
    std::remove(renamed.c_str());
    com_util_file_init(&file);

    ASSERT_EQ(0, com_util_file_open(&file, path.c_str(),
                                    COM_UTIL_FILE_OPEN_CREATE | COM_UTIL_FILE_OPEN_APPEND |
                                        COM_UTIL_FILE_OPEN_WRITE_THROUGH | COM_UTIL_FILE_OPEN_SHARE_READ |
                                        COM_UTIL_FILE_OPEN_SHARE_DELETE | COM_UTIL_FILE_OPEN_SHARE_WRITE));
    ASSERT_EQ(0, com_util_file_get_id(&file, &handle_id));

    // ローテーション相当の操作: path をリネームして同じ path に別ファイルを作る
    ASSERT_EQ(0, std::rename(path.c_str(), renamed.c_str()));
    write_text_file(path, "recreated");

    ASSERT_EQ(0, com_util_file_get_path_id(path.c_str(), &path_id));

    // path は別実体を指すため、開いているハンドルの同一性とは一致しない
    EXPECT_FALSE(handle_id.volume == path_id.volume && handle_id.index == path_id.index);

    com_util_file_close(&file);
    std::remove(path.c_str());
    std::remove(renamed.c_str());
}

TEST_F(crt_fileTest, file_id_invalid_arguments_fail)
{
    com_util_file file;
    com_util_file_id id;

    com_util_file_init(&file);

    EXPECT_EQ(-1, com_util_file_get_id(&file, &id)); // 未オープンのハンドル
    EXPECT_EQ(-1, com_util_file_get_id(NULL, &id));
    EXPECT_EQ(-1, com_util_file_get_path_id(NULL, &id));
    EXPECT_EQ(-1, com_util_file_get_path_id("crt_fileTest_no_such_file.log", &id)); // 存在しないパス
    EXPECT_EQ(-1, com_util_file_get_path_id("x", NULL));
}
