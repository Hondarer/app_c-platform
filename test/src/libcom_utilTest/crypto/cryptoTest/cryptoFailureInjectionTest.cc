#if defined(PLATFORM_LINUX)

#include <testfw.h>
#include <mock_openssl.h>
#include <com_util/base/result.h>
#include <com_util/crypto/crypto.h>

#include <cstring>
#include <vector>

using testing::_;
using testing::DoDefault;
using testing::NiceMock;
using testing::Return;

class cryptoFailureInjectionTest : public Test
{
  protected:
    uint8_t key_[COM_UTIL_CRYPTO_KEY_SIZE];
    uint8_t nonce_[COM_UTIL_CRYPTO_NONCE_SIZE];
    uint8_t plain_[16];
    uint8_t pad_[4]; // cipher_ の 8 バイト境界を明示する。
    std::vector<uint8_t> cipher_;
    size_t cipher_len_ = 0u;

    void SetUp() override
    {
        std::memset(key_, 0x11, sizeof(key_));
        std::memset(nonce_, 0x22, sizeof(nonce_));
        std::memset(plain_, 0x33, sizeof(plain_));
        cipher_.assign(sizeof(plain_) + COM_UTIL_CRYPTO_TAG_SIZE, 0u);
        cipher_len_ = cipher_.size();
    }

    int encrypt()
    {
        cipher_len_ = cipher_.size();
        return com_util_encrypt(cipher_.data(), &cipher_len_, plain_, sizeof(plain_), key_, nonce_, NULL, 0u);
    }
};

// 暗号化コンテキストの確保に失敗した場合にメモリ不足が返ることの確認
TEST_F(cryptoFailureInjectionTest, encrypt_returns_out_of_memory_when_context_allocation_fails)
{
    // Arrange
    NiceMock<Mock_openssl> mock_openssl;

    // Pre-Assert
    EXPECT_CALL(mock_openssl, EVP_CIPHER_CTX_new(_, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - EVP_CIPHER_CTX_new が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は NULL を返却し、以降は本物へ委譲する。

    // Act
    int rtc = encrypt(); // [手順] - com_util_encrypt を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_MEMORY,
              rtc); // [確認_異常系] - com_util_encrypt の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
}

// 暗号化の初期化に失敗した場合に通知されることの確認
TEST_F(cryptoFailureInjectionTest, encrypt_returns_unknown_when_init_fails)
{
    // Arrange
    NiceMock<Mock_openssl> mock_openssl;

    // Pre-Assert
    EXPECT_CALL(mock_openssl, EVP_EncryptInit_ex(_, _, _, _, _, _, _, _))
        .WillOnce(Return(0))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - EVP_EncryptInit_ex が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は失敗を示す 0 を返却し、以降は本物へ委譲する。

    // Act
    int rtc = encrypt(); // [手順] - com_util_encrypt を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - com_util_encrypt の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// ノンス長の設定に失敗した場合に通知されることの確認
TEST_F(cryptoFailureInjectionTest, encrypt_returns_unknown_when_nonce_length_setting_fails)
{
    // Arrange
    NiceMock<Mock_openssl> mock_openssl;

    // Pre-Assert
    EXPECT_CALL(mock_openssl, EVP_CIPHER_CTX_ctrl(_, _, _, _, _, _, _))
        .WillOnce(Return(0))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - EVP_CIPHER_CTX_ctrl が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は失敗を示す 0 を返却し、以降は本物へ委譲する。

    // Act
    int rtc = encrypt(); // [手順] - com_util_encrypt を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - com_util_encrypt の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// 暗号文の生成に失敗した場合に通知されることの確認
TEST_F(cryptoFailureInjectionTest, encrypt_returns_unknown_when_update_fails)
{
    // Arrange
    NiceMock<Mock_openssl> mock_openssl;

    // Pre-Assert
    EXPECT_CALL(mock_openssl, EVP_EncryptUpdate(_, _, _, _, _, _, _, _))
        .WillOnce(Return(0))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - EVP_EncryptUpdate が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は失敗を示す 0 を返却し、以降は本物へ委譲する。

    // Act
    int rtc = encrypt(); // [手順] - com_util_encrypt を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - com_util_encrypt の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// 暗号化の終端処理に失敗した場合に通知されることの確認
TEST_F(cryptoFailureInjectionTest, encrypt_returns_unknown_when_final_fails)
{
    // Arrange
    NiceMock<Mock_openssl> mock_openssl;

    // Pre-Assert
    EXPECT_CALL(mock_openssl, EVP_EncryptFinal_ex(_, _, _, _, _, _))
        .WillOnce(Return(0))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - EVP_EncryptFinal_ex が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は失敗を示す 0 を返却し、以降は本物へ委譲する。

    // Act
    int rtc = encrypt(); // [手順] - com_util_encrypt を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - com_util_encrypt の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

// 復号コンテキストの確保に失敗した場合にメモリ不足が返ることの確認
TEST_F(cryptoFailureInjectionTest, decrypt_returns_out_of_memory_when_context_allocation_fails)
{
    // Arrange
    std::vector<uint8_t> restored(sizeof(plain_));
    size_t restored_len = restored.size();

    ASSERT_EQ(COM_UTIL_OK, encrypt()); // [状態] - 復号対象の暗号文を用意する。

    NiceMock<Mock_openssl> mock_openssl;

    // Pre-Assert
    EXPECT_CALL(mock_openssl, EVP_CIPHER_CTX_new(_, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - EVP_CIPHER_CTX_new が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は NULL を返却し、以降は本物へ委譲する。

    // Act
    int rtc = com_util_decrypt(restored.data(), &restored_len, cipher_.data(), cipher_len_, key_, nonce_, NULL,
                               0u); // [手順] - com_util_decrypt を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_MEMORY,
              rtc); // [確認_異常系] - com_util_decrypt の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
}

// ダイジェスト コンテキストの確保に失敗した場合にメモリ不足が返ることの確認
TEST_F(cryptoFailureInjectionTest, passphrase_to_key_returns_out_of_memory_when_context_allocation_fails)
{
    // Arrange
    NiceMock<Mock_openssl> mock_openssl;
    uint8_t derived[COM_UTIL_CRYPTO_KEY_SIZE]; // [状態] - 導出結果の格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_openssl, EVP_MD_CTX_new(_, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - EVP_MD_CTX_new が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は NULL を返却し、以降は本物へ委譲する。

    // Act
    int rtc = com_util_passphrase_to_key(derived, (const uint8_t *)"secret",
                                         6u); // [手順] - com_util_passphrase_to_key を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_OUT_OF_MEMORY,
              rtc); // [確認_異常系] - com_util_passphrase_to_key の戻り値が COM_UTIL_ERR_OUT_OF_MEMORY であること。
}

// ダイジェストの計算に失敗した場合に通知されることの確認
TEST_F(cryptoFailureInjectionTest, passphrase_to_key_returns_unknown_when_digest_fails)
{
    // Arrange
    NiceMock<Mock_openssl> mock_openssl;
    uint8_t derived[COM_UTIL_CRYPTO_KEY_SIZE]; // [状態] - 導出結果の格納先を用意する。

    // Pre-Assert
    EXPECT_CALL(mock_openssl, EVP_DigestInit_ex(_, _, _, _, _, _))
        .WillOnce(Return(0))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - EVP_DigestInit_ex が 1 回目に呼び出されること。
                                      // [Pre-Assert手順] - 1 回目は失敗を示す 0 を返却し、以降は本物へ委譲する。

    // Act
    int rtc = com_util_passphrase_to_key(derived, (const uint8_t *)"secret",
                                         6u); // [手順] - com_util_passphrase_to_key を呼び出す。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              rtc); // [確認_異常系] - com_util_passphrase_to_key の戻り値が COM_UTIL_ERR_UNKNOWN であること。
}

#endif /* PLATFORM_LINUX */
