#include "dlms/transport/iec62056_21_mode_e.hpp"

#include <gtest/gtest.h>

#include <string>

namespace {

using dlms::transport::BuildIec62056_21ModeEAck;
using dlms::transport::BuildIec62056_21SignOnRequest;
using dlms::transport::Iec62056_21BaudRate;
using dlms::transport::Iec62056_21BaudRateCode;
using dlms::transport::Iec62056_21BaudRateValue;
using dlms::transport::Iec62056_21Identification;
using dlms::transport::ParseIec62056_21Identification;
using dlms::transport::SelectIec62056_21ModeEBaudRate;
using dlms::transport::TransportStatus;

TEST(Iec62056_21ModeE, BuildsSignOnRequest)
{
  std::string request;

  EXPECT_EQ(TransportStatus::Ok, BuildIec62056_21SignOnRequest(request));
  EXPECT_EQ("/?!\r\n", request);
}

TEST(Iec62056_21ModeE, ParsesIdentificationOptions)
{
  Iec62056_21Identification identification;

  EXPECT_EQ(
    TransportStatus::Ok,
    ParseIec62056_21Identification("/ABC5METER\\W2\r\n", identification));

  EXPECT_TRUE(identification.supportsModeE);
  EXPECT_EQ(Iec62056_21BaudRate::Baud9600, identification.maxBaudRate);
  EXPECT_EQ("/ABC5METER\\W2\r\n", identification.raw);
}

TEST(Iec62056_21ModeE, SelectsRequestedBaudRate)
{
  Iec62056_21Identification identification;
  ASSERT_EQ(
    TransportStatus::Ok,
    ParseIec62056_21Identification("/ABC6METER\\W2\r\n", identification));

  Iec62056_21BaudRate selected = Iec62056_21BaudRate::Baud300;
  EXPECT_EQ(
    TransportStatus::Ok,
    SelectIec62056_21ModeEBaudRate(
      identification,
      Iec62056_21BaudRate::Baud9600,
      selected));
  EXPECT_EQ(Iec62056_21BaudRate::Baud9600, selected);

  std::string ack;
  EXPECT_EQ(TransportStatus::Ok, BuildIec62056_21ModeEAck(selected, ack));
  ASSERT_EQ(6u, ack.size());
  EXPECT_EQ(0x06, static_cast<unsigned char>(ack[0]));
  EXPECT_EQ('2', ack[1]);
  EXPECT_EQ('5', ack[2]);
  EXPECT_EQ('2', ack[3]);
  EXPECT_EQ('\r', ack[4]);
  EXPECT_EQ('\n', ack[5]);
}

TEST(Iec62056_21ModeE, RejectsUnsupportedMode)
{
  Iec62056_21Identification identification;
  ASSERT_EQ(
    TransportStatus::Ok,
    ParseIec62056_21Identification("/ABC5METER\\W1\r\n", identification));

  Iec62056_21BaudRate selected = Iec62056_21BaudRate::Baud300;
  EXPECT_EQ(
    TransportStatus::UnsupportedFeature,
    SelectIec62056_21ModeEBaudRate(
      identification,
      Iec62056_21BaudRate::Baud9600,
      selected));
}

TEST(Iec62056_21ModeE, DoesNotParseHdlcFrames)
{
  Iec62056_21Identification identification;

  EXPECT_EQ(
    TransportStatus::InvalidArgument,
    ParseIec62056_21Identification("~\xA0\x03\x00~", identification));
}

TEST(Iec62056_21ModeE, ReportsBaudRateCodesAndValues)
{
  EXPECT_EQ('0', Iec62056_21BaudRateCode(Iec62056_21BaudRate::Baud300));
  EXPECT_EQ('6', Iec62056_21BaudRateCode(Iec62056_21BaudRate::Baud19200));
  EXPECT_EQ(300u, Iec62056_21BaudRateValue(Iec62056_21BaudRate::Baud300));
  EXPECT_EQ(19200u, Iec62056_21BaudRateValue(Iec62056_21BaudRate::Baud19200));
}

} // namespace
