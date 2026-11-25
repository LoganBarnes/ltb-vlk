// ///////////////////////////////////////////////////////////////////////////////////////
// A Logan Thomas Barnes project
// ///////////////////////////////////////////////////////////////////////////////////////
#include "ltb/utils/file_utils.hpp"

// external
#include <gtest/gtest.h>

// standard
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace ltb::utils
{
namespace
{

class FileUtilsTest : public ::testing::Test
{
protected:
    void SetUp( ) override
    {
        auto const* const test_info = ::testing::UnitTest::GetInstance( )->current_test_info( );
        dir_ = std::filesystem::temp_directory_path( ) / "ltb_vlk_file_utils_tests"
             / test_info->name( );
        std::filesystem::create_directories( dir_ );
    }

    void TearDown( ) override
    {
        std::filesystem::remove_all( dir_ );
    }

    auto write_file( std::string const& name, std::string const& contents ) const
        -> std::filesystem::path
    {
        auto const path = dir_ / name;
        auto       file = std::ofstream( path, std::ios::binary );
        file << contents;
        return path;
    }

    std::filesystem::path dir_;
};

TEST_F( FileUtilsTest, MissingFileIsAnError )
{
    EXPECT_FALSE( get_binary_file_contents< char >( dir_ / "missing.bin" ).has_value( ) );
}

TEST_F( FileUtilsTest, EmptyFileIsEmpty )
{
    auto const result = get_binary_file_contents< std::uint32_t >( write_file( "empty", "" ) );
    ASSERT_TRUE( result.has_value( ) );
    EXPECT_TRUE( result->empty( ) );
}

TEST_F( FileUtilsTest, PartialTrailingElementIsZeroPadded )
{
    auto const result
        = get_binary_file_contents< std::uint32_t >( write_file( "five_bytes", "abcde" ) );
    ASSERT_TRUE( result.has_value( ) );
    ASSERT_EQ( result->size( ), 2U );

    auto const* const bytes = reinterpret_cast< char const* >( result->data( ) );
    EXPECT_EQ( std::string( bytes, 5U ), "abcde" );
    EXPECT_EQ( bytes[ 5 ], '\0' );
    EXPECT_EQ( bytes[ 7 ], '\0' );
}

TEST_F( FileUtilsTest, MultipleFilesFailIfAnyFails )
{
    auto const good = write_file( "good", "ok" );
    EXPECT_TRUE( get_binary_files_contents< char >( { good, good } ).has_value( ) );
    EXPECT_FALSE( get_binary_files_contents< char >( { good, dir_ / "missing" } ).has_value( ) );
}

} // namespace
} // namespace ltb::utils
