#include <testfw.h>

#include <mock_com_util.h>
#include <mock_stdio.h>

#include <cstdlib>

class hashtable_required_sizeTest : public Test
{
  protected:
    hashtable_required_sizeTest()
    {
        ON_CALL(mock_com_util_, com_util_shutdown_register(_, _)).WillByDefault(Return(COM_UTIL_OK));
    }

    NiceMock<Mock_stdio> mock_stdio_;
    NiceMock<Mock_com_util> mock_com_util_;
};

TEST_F(hashtable_required_sizeTest, main_prints_required_size)
{
    // Arrange
    const char *argv[] = {"hashtable-required-size", "--capacity=4", "--key-size=8", "--value-size=8"};
    const int argc = (int)(sizeof(argv) / sizeof(argv[0]));

    // Pre-Assert

    // Act
    int actual_ret = __real_main(argc, const_cast<char **>(argv)); // [手順] - 必須オプションを渡して main を呼び出す。

    // Assert
    EXPECT_EQ(EXIT_SUCCESS, actual_ret); // [確認_正常系] - 正常終了すること。
}

TEST_F(hashtable_required_sizeTest, main_accepts_record_timestamp_flag)
{
    // Arrange
    const char *argv[] = {"hashtable-required-size", "--capacity=4", "--key-size=8", "--value-size=8",
                          "--record-timestamp"};
    const int argc = (int)(sizeof(argv) / sizeof(argv[0]));

    // Pre-Assert

    // Act
    int actual_ret = __real_main(argc, const_cast<char **>(argv)); // [手順] - レコード時刻付きで main を呼び出す。

    // Assert
    EXPECT_EQ(EXIT_SUCCESS, actual_ret); // [確認_正常系] - --record-timestamp でも正常終了すること。
}

TEST_F(hashtable_required_sizeTest, main_rejects_missing_options)
{
    // Arrange
    const char *argv[] = {"hashtable-required-size"};
    const int argc = (int)(sizeof(argv) / sizeof(argv[0]));

    // Pre-Assert

    // Act
    int actual_ret = __real_main(argc, const_cast<char **>(argv)); // [手順] - オプション無しで main を呼び出す。

    // Assert
    EXPECT_NE(EXIT_SUCCESS, actual_ret); // [確認_異常系] - 失敗終了すること。
}
