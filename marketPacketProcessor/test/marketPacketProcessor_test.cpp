#include <gtest/gtest.h>

#include "marketPacketGenerator/marketPacketGenerator.h"
#include "marketPacketHelpers/marketPacketHelpers.h"
#include "marketPacketProcessor/marketPacketProcessor.h"

namespace test
{
  // Ideally, all these go into a config file
  constexpr std::string_view INPUT_PATH = "./input_test.dat";
  constexpr std::string_view OUTPUT_PATH = "./output_test.dat";

  /**
   * @brief Create a Default Processor
   */
  marketPacket::marketPacketProcessor_t createDefaultProcessor()
  {
    return marketPacket::marketPacketProcessor_t(std::make_shared<std::ifstream>(INPUT_PATH), std::make_shared<std::ofstream>(OUTPUT_PATH));
  }

  TEST(marketPacketProcessorTest, noInit)
  {
    marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();

    std::optional<std::string> failReason = mpp.processNextPacket();
    ASSERT_TRUE(failReason.has_value());
    EXPECT_EQ(failReason.value(), "Processor is uninitialized");
  }

  TEST(marketPacketProcessorTest, doubleInit)
  {
    marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();

    mpp.initialize();
    EXPECT_DEATH(mpp.initialize(), "");
  }

  TEST(marketPacketProcessorTest, noInputStream)
  {
    marketPacket::marketPacketProcessor_t mpp(nullptr, std::make_shared<std::ofstream>(OUTPUT_PATH));
    EXPECT_DEATH(mpp.initialize(), "");
  }

  TEST(marketPacketProcessorTest, noOutputStream)
  {
    marketPacket::marketPacketProcessor_t mpp(std::make_shared<std::ifstream>(INPUT_PATH), nullptr);
    EXPECT_DEATH(mpp.initialize(), "");
  }

  /**
   * This test takes longer than the others sometimes
   * I suspect because it's the first test to do real I/O
   */
  TEST(marketPacketProcessorTest, nonSensePacketHeader)
  {
    marketPacket::packetHeader_t ph{0, 0};
    ASSERT_TRUE(std::ofstream(INPUT_PATH).write(reinterpret_cast<char *>(&ph), sizeof(ph)));

    marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
    mpp.initialize();

    EXPECT_EQ(mpp.processNextPacket().value(), "Poorly formed packet header");
  }

  TEST(marketPacketProcessorTest, shortPacketHeader)
  {
    marketPacket::packetHeader_t ph{0, 0};
    ASSERT_TRUE(std::ofstream(INPUT_PATH).write(reinterpret_cast<char *>(&ph), sizeof(ph) - 1));

    marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
    mpp.initialize();

    EXPECT_EQ(mpp.processNextPacket().value(), "Packet header read failed");
  }

  TEST(marketPacketProcessorTest, badUpdateLength)
  {
    marketPacket::packetHeader_t ph{sizeof(marketPacket::packetHeader_t) + sizeof(marketPacket::trade_t), 1};
    marketPacket::trade_t trade{
        .length = 12,                              // Anything but marketPacket::UPDATE_SIZE (32) shouldn't be accepted
        .type = marketPacket::updateType_e::TRADE, // Make sure we don't reject because of the type
    };

    {
      std::ofstream genStream(INPUT_PATH);
      ASSERT_TRUE(genStream.write(reinterpret_cast<char *>(&ph), sizeof(ph)));
      ASSERT_TRUE(genStream.write(reinterpret_cast<char *>(&trade), sizeof(trade)));
    }

    marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
    mpp.initialize();

    ASSERT_EQ(mpp.processNextPacket().value(), "Poorly formed update");
  }

  TEST(marketPacketProcessorTest, badUpdateType)
  {
    marketPacket::packetHeader_t ph{sizeof(marketPacket::packetHeader_t) + sizeof(marketPacket::trade_t), 1};
    marketPacket::trade_t trade{
        .length = marketPacket::UPDATE_SIZE,         // Make sure we don't reject becuase of the length
        .type = marketPacket::updateType_e::INVALID, // Bad type
    };

    {
      std::ofstream genStream(INPUT_PATH);
      ASSERT_TRUE(genStream.write(reinterpret_cast<char *>(&ph), sizeof(ph)));
      ASSERT_TRUE(genStream.write(reinterpret_cast<char *>(&trade), sizeof(trade)));
    }

    marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
    mpp.initialize();

    ASSERT_EQ(mpp.processNextPacket().value(), "Poorly formed update");
  }

  TEST(marketPacketProcessorTest, processOnePacketOneTrade)
  {
    std::string randomSymbol = marketPacket::generateRandomSymbol();
    uint16_t randomTradeSize = static_cast<uint16_t>(marketPacket::rand());
    uint64_t randomTradePrice = static_cast<uint64_t>(marketPacket::rand());

    // This is the string we expect to get back with these values
    std::string expectedStr("Trade: " + randomSymbol + " Size: " + std::to_string(randomTradeSize) + " Price: " + std::to_string(randomTradePrice) + '\n');

    marketPacket::packetHeader_t ph{sizeof(marketPacket::packetHeader_t) + sizeof(marketPacket::trade_t), 1};
    marketPacket::trade_t trade{
        .length = sizeof(marketPacket::trade_t),
        .type = marketPacket::updateType_e::TRADE,
        .tradeSize = randomTradeSize,
        .tradePrice = randomTradePrice,
    };
    std::memcpy(trade.symbol, randomSymbol.c_str(), marketPacket::SYMBOL_LENGTH);

    {
      std::ofstream genStream(INPUT_PATH);
      ASSERT_TRUE(genStream.write(reinterpret_cast<char *>(&ph), sizeof(ph)));
      ASSERT_TRUE(genStream.write(reinterpret_cast<char *>(&trade), sizeof(trade)));
    }

    {
      marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
      mpp.initialize();

      ASSERT_FALSE(mpp.processNextPacket(1).has_value());
      ASSERT_TRUE(mpp.processNextPacket(1).has_value());
    }

    {
      std::shared_ptr<std::ifstream> readStream = std::make_shared<std::ifstream>(OUTPUT_PATH);
      char tradeLine[expectedStr.length() + 1];
      tradeLine[expectedStr.length()] = '\0';

      // Read in the trade
      ASSERT_TRUE(readStream->read(tradeLine, expectedStr.length()));
      EXPECT_EQ(expectedStr, std::string(tradeLine));
    }
  }

  TEST(marketPacketProcessorTest, processOnePacketOneQuote)
  {
    marketPacket::packetHeader_t ph{sizeof(marketPacket::packetHeader_t) + sizeof(marketPacket::quote_t), 1};
    marketPacket::quote_t quote{
        .length = sizeof(marketPacket::quote_t),
        .type = marketPacket::updateType_e::QUOTE,
    };

    {
      std::shared_ptr<std::ofstream> genStream = std::make_shared<std::ofstream>(INPUT_PATH);
      ASSERT_TRUE(genStream->write(reinterpret_cast<char *>(&ph), sizeof(ph)));
      ASSERT_TRUE(genStream->write(reinterpret_cast<char *>(&quote), sizeof(quote)));
    }

    {
      marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
      mpp.initialize();

      ASSERT_FALSE(mpp.processNextPacket(1).has_value());
      ASSERT_EQ(mpp.processNextPacket(1).value(), "End of file");
    }

    {
      char tradeLine[1];
      std::shared_ptr<std::ifstream> readStream = std::make_shared<std::ifstream>(OUTPUT_PATH);

      // The file should be empty because we don't process quotes
      EXPECT_FALSE(readStream->read(tradeLine, 1));
    }
  }

  TEST(marketPacketProcessorTest, processCorrectNumberPackets)
  {
    std::string randomSymbol = marketPacket::generateRandomSymbol();
    uint16_t randomTradeSize = static_cast<uint16_t>(marketPacket::rand());
    uint64_t randomTradePrice = static_cast<uint64_t>(marketPacket::rand());

    // This is the string we expect to get back with these values
    std::string expectedStr("Trade: " + randomSymbol + " Size: " + std::to_string(randomTradeSize) + " Price: " + std::to_string(randomTradePrice) + '\n');

    marketPacket::packetHeader_t ph{sizeof(marketPacket::packetHeader_t) + sizeof(marketPacket::trade_t), 1};
    marketPacket::trade_t trade{
        .length = sizeof(marketPacket::trade_t),
        .type = marketPacket::updateType_e::TRADE,
        .tradeSize = randomTradeSize,
        .tradePrice = randomTradePrice,
    };
    std::memcpy(trade.symbol, randomSymbol.c_str(), marketPacket::SYMBOL_LENGTH);

    constexpr const size_t NUM_PACKETS_TO_WRITE = 6;

    {
      std::ofstream genStream(INPUT_PATH);

      for (size_t i = 0; i < NUM_PACKETS_TO_WRITE; i++)
      {
        ASSERT_TRUE(genStream.write(reinterpret_cast<char *>(&ph), sizeof(ph)));
        ASSERT_TRUE(genStream.write(reinterpret_cast<char *>(&trade), sizeof(trade)));
      }
    }

    {
      marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
      mpp.initialize();

      ASSERT_FALSE(mpp.processNextPacket(1).has_value());
      ASSERT_FALSE(mpp.processNextPacket(2).has_value());
      ASSERT_FALSE(mpp.processNextPacket(1).has_value());
      ASSERT_FALSE(mpp.processNextPacket(2).has_value());

      ASSERT_EQ(mpp.processNextPacket().value(), "End of file");
    }

    {
      std::shared_ptr<std::ifstream> readStream = std::make_shared<std::ifstream>(OUTPUT_PATH);
      char tradeLine[expectedStr.length() + 1];
      tradeLine[expectedStr.length()] = '\0';

      // Read in the trade
      for (size_t i = 0; i < NUM_PACKETS_TO_WRITE; i++)
      {
        ASSERT_TRUE(readStream->read(tradeLine, expectedStr.length()));
        EXPECT_EQ(expectedStr, std::string(tradeLine));
      }
    }
  }

  /**
   * This is a weird case of two classes verifying the other.
   * Past basic tests, we assume basic functionality works at scale for the generator for this test.
   */
  TEST(marketPacketProcessorTest, genericStressTest_long)
  {
    constexpr const size_t NUM_PACKETS_TO_GENERATE = 10000;

    {
      marketPacket::marketPacketGenerator_t mpg(std::make_shared<std::ofstream>(INPUT_PATH));
      mpg.initialize();

      ASSERT_FALSE(mpg.generatePackets(NUM_PACKETS_TO_GENERATE, marketPacket::MAX_UPDATES_ALLOWED_IN_PACKET).has_value());
    }

    {
      marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
      mpp.initialize();

      ASSERT_EQ(mpp.processNextPacket().value(), "End of file");
    }
  }
}