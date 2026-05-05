#include "dlms/transport/transport_status.hpp"

#include <gtest/gtest.h>

namespace {

using dlms::transport::ToString;
using dlms::transport::TransportStatus;

TEST(TransportStatus, ToStringReturnsStableNames)
{
  EXPECT_STREQ("Ok", ToString(TransportStatus::Ok));
  EXPECT_STREQ("InvalidArgument", ToString(TransportStatus::InvalidArgument));
  EXPECT_STREQ("NotOpen", ToString(TransportStatus::NotOpen));
  EXPECT_STREQ("AlreadyOpen", ToString(TransportStatus::AlreadyOpen));
  EXPECT_STREQ("OpenFailed", ToString(TransportStatus::OpenFailed));
  EXPECT_STREQ("ReadFailed", ToString(TransportStatus::ReadFailed));
  EXPECT_STREQ("WriteFailed", ToString(TransportStatus::WriteFailed));
  EXPECT_STREQ("Timeout", ToString(TransportStatus::Timeout));
  EXPECT_STREQ("ConnectionClosed", ToString(TransportStatus::ConnectionClosed));
  EXPECT_STREQ("WouldBlock", ToString(TransportStatus::WouldBlock));
  EXPECT_STREQ("OutputBufferTooSmall", ToString(TransportStatus::OutputBufferTooSmall));
  EXPECT_STREQ("UnsupportedFeature", ToString(TransportStatus::UnsupportedFeature));
  EXPECT_STREQ("InternalError", ToString(TransportStatus::InternalError));
}

TEST(TransportStatus, ToStringReturnsUnknownForInvalidValue)
{
  EXPECT_STREQ("Unknown", ToString(static_cast<TransportStatus>(255)));
}

} // namespace
