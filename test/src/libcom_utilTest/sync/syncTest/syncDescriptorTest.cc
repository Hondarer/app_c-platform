#include <testfw.h>

#include <com_util/sync/sync_descriptor.h>

#include <mock_stdlib.h>

using testing::_;
using testing::DoDefault;
using testing::NiceMock;
using testing::Return;

#include "syncTestHelper.h"

TEST(syncDescriptorTest, rejects_invalid_export_and_import_arguments)
{
    // Arrange
    unsigned char descriptor[64] = {0};
    size_t descriptor_size = sizeof(descriptor);
    char *identity = NULL;

    // Pre-Assert

    // Act
    int export_identity_result = interprocess_sync_descriptor_export(
        NULL, 1U, 1U, descriptor, &descriptor_size); // [手順] - NULL identity で descriptor を出力する。
    int export_size_result = interprocess_sync_descriptor_export(
        "identity", 1U, 1U, descriptor, NULL); // [手順] - NULL サイズ格納先で descriptor を出力する。
    int import_descriptor_result = interprocess_sync_descriptor_import(
        NULL, sizeof(descriptor), 1U, 1U, &identity); // [手順] - NULL descriptor を import する。
    int import_identity_result = interprocess_sync_descriptor_import(
        descriptor, sizeof(descriptor), 1U, 1U, NULL); // [手順] - NULL identity 格納先で import する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              export_identity_result); // [確認_異常系] - NULL identity が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              export_size_result); // [確認_異常系] - NULL サイズ格納先が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              import_descriptor_result); // [確認_異常系] - NULL descriptor が INVALID_ARGUMENT になること。
    EXPECT_EQ(COM_UTIL_ERR_INVALID_ARGUMENT,
              import_identity_result); // [確認_異常系] - NULL identity 格納先が INVALID_ARGUMENT になること。
}

TEST(syncDescriptorTest, rejects_zero_and_mismatched_identity_lengths)
{
    // Arrange
    unsigned char descriptor[64] = {0};
    size_t descriptor_size = sizeof(descriptor);
    char *identity = NULL;

    // Pre-Assert
    ASSERT_EQ(COM_UTIL_OK, interprocess_sync_descriptor_export("identity", 1U, 1U, descriptor, &descriptor_size));

    // Act
    descriptor[8] = 0U;
    descriptor[9] = 0U;
    descriptor[10] = 0U;
    descriptor[11] = 0U;
    int zero_length_result = interprocess_sync_descriptor_import(
        descriptor, 20U, 1U, 1U, &identity); // [手順] - identity length 0 の descriptor を import する。
    int short_descriptor_result = interprocess_sync_descriptor_import(
        descriptor, descriptor_size, 1U, 1U,
        &identity); // [手順] - identity length とサイズが一致しない descriptor を import する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              zero_length_result); // [確認_異常系] - identity length 0 が CORRUPT_DESCRIPTOR になること。
    EXPECT_EQ(COM_UTIL_ERR_CORRUPT_DESCRIPTOR,
              short_descriptor_result); // [確認_異常系] - サイズ不一致が CORRUPT_DESCRIPTOR になること。
}

TEST(syncDescriptorTest, reports_unknown_when_identity_allocation_fails)
{
    // Arrange
    NiceMock<Mock_stdlib> mock_stdlib;
    unsigned char descriptor[64] = {0};
    size_t descriptor_size = sizeof(descriptor);
    char *identity = NULL;
    ASSERT_EQ(COM_UTIL_OK, interprocess_sync_descriptor_export("identity", 1U, 1U, descriptor, &descriptor_size));

    // Pre-Assert
    EXPECT_CALL(mock_stdlib, malloc(_, _, _, _))
        .WillOnce(Return(nullptr))
        .WillRepeatedly(DoDefault()); // [Pre-Assert確認_異常系] - identity 確保が 1 回目に失敗すること。

    // Act
    int result = interprocess_sync_descriptor_import(descriptor, descriptor_size, 1U, 1U,
                                                     &identity); // [手順] - descriptor を import する。

    // Assert
    EXPECT_EQ(COM_UTIL_ERR_UNKNOWN,
              result);         // [確認_異常系] - import の戻り値が COM_UTIL_ERR_UNKNOWN になること。
    EXPECT_EQ(NULL, identity); // [確認_異常系] - identity が設定されないこと。
}
