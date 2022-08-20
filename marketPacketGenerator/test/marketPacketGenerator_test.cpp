#include <gtest/gtest.h>

#include "marketPacketGenerator/marketPacketGenerator.h"
#include "marketPacketProcessor/marketPacketProcessor.h"

namespace test
{
    // Ideally, this goes into a config file
    constexpr std::string_view GENERATE_PATH = "./generate_test.dat";
    constexpr std::string_view OUTPUT_PATH = "./output_test.dat";
    constexpr const size_t MANY_PACKETS = 1000;

    marketPacket::marketPacketGenerator_t createDefaultGenerator()
    {
        return marketPacket::marketPacketGenerator_t(std::make_shared<std::ofstream>(GENERATE_PATH));
    }

    marketPacket::marketPacketProcessor_t createDefaultProcessor()
    {
        return marketPacket::marketPacketProcessor_t(std::make_shared<std::ifstream>(GENERATE_PATH), std::make_shared<std::ofstream>(OUTPUT_PATH));
    }

    TEST(marketPacketGeneratorTest, noInit)
    {
        marketPacket::marketPacketGenerator_t mpg = createDefaultGenerator();
        EXPECT_EQ(mpg.generatePackets(1, 1).value(), "Generator is uninitialized");
    }

    TEST(marketPacketGeneratorTest, doubleInit)
    {
        marketPacket::marketPacketGenerator_t mpg = createDefaultGenerator();
        mpg.initialize();

        EXPECT_DEATH(mpg.initialize(), "");
    }

    TEST(marketPacketGeneratorTest, noOutputStream)
    {
        marketPacket::marketPacketGenerator_t mpg = marketPacket::marketPacketGenerator_t(nullptr);
        EXPECT_DEATH(mpg.initialize(), "");
    }

    TEST(marketPacketGeneratorTest, tooManyUpdates)
    {
        EXPECT_EQ(createDefaultGenerator().generatePackets(1, std::numeric_limits<size_t>::max()).value(), "Can't request that many updates in a packet");
    }

    TEST(marketPacketGeneratorTest, onePacketoneUpdate)
    {
        {
            marketPacket::marketPacketGenerator_t mpg = createDefaultGenerator();
            mpg.initialize();

            EXPECT_FALSE(mpg.generatePackets(1, 1).has_value());
        }

        // Make sure we wrote exactly the bytes we expected
        size_t expectedSize = sizeof(marketPacket::packetHeader_t) + sizeof(marketPacket::update_t);
        EXPECT_EQ(std::ifstream(GENERATE_PATH, std::ifstream::ate | std::ifstream::binary).tellg(), expectedSize);

        // This inherently checks validity of the the underlying data
        marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
        mpp.initialize();

        EXPECT_FALSE(mpp.processNextPacket(1).has_value());
        EXPECT_EQ(mpp.processNextPacket(1).value(), "End of file");
    }

    TEST(marketPacketGeneratorTest, onePacketManyUpdate)
    {
        {
            marketPacket::marketPacketGenerator_t mpg = createDefaultGenerator();
            mpg.initialize();

            EXPECT_FALSE(mpg.generatePackets(1, marketPacket::MAX_UPDATES_ALLOWED_IN_PACKET).has_value());
        }

        // Since we don't know exactly the number of packets, we can only check within a bound
        size_t maxSize = sizeof(marketPacket::packetHeader_t) + marketPacket::MAX_UPDATES_ALLOWED_IN_PACKET * sizeof(marketPacket::update_t);
        EXPECT_LE(std::ifstream(GENERATE_PATH, std::ifstream::ate | std::ifstream::binary).tellg(), maxSize);

        marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
        mpp.initialize();

        EXPECT_FALSE(mpp.processNextPacket(1).has_value());
        EXPECT_EQ(mpp.processNextPacket(1).value(), "End of file");
    }

    TEST(marketPacketGeneratorTest, manyPacketOneUpdate)
    {
        {
            marketPacket::marketPacketGenerator_t mpg = createDefaultGenerator();
            mpg.initialize();

            EXPECT_FALSE(mpg.generatePackets(MANY_PACKETS, 1).has_value());
        }

        size_t expectedSize = MANY_PACKETS * (sizeof(marketPacket::packetHeader_t) + sizeof(marketPacket::update_t));
        EXPECT_EQ(std::ifstream(GENERATE_PATH, std::ifstream::ate | std::ifstream::binary).tellg(), expectedSize);

        marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
        mpp.initialize();

        EXPECT_FALSE(mpp.processNextPacket(MANY_PACKETS).has_value());
        EXPECT_EQ(mpp.processNextPacket(1).value(), "End of file");
    }

    TEST(marketPacketGeneratorTest, manyPacketManyUpdate)
    {
        {
            marketPacket::marketPacketGenerator_t mpg = createDefaultGenerator();
            mpg.initialize();

            EXPECT_FALSE(mpg.generatePackets(MANY_PACKETS, marketPacket::MAX_UPDATES_ALLOWED_IN_PACKET).has_value());
        }

        // We can only check an upper bound
        size_t expectedSize = MANY_PACKETS * (sizeof(marketPacket::packetHeader_t) + marketPacket::MAX_UPDATES_ALLOWED_IN_PACKET * sizeof(marketPacket::update_t));
        EXPECT_LE(std::ifstream(GENERATE_PATH, std::ifstream::ate | std::ifstream::binary).tellg(), expectedSize);

        marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
        mpp.initialize();

        EXPECT_FALSE(mpp.processNextPacket(MANY_PACKETS).has_value());
        EXPECT_EQ(mpp.processNextPacket(1).value(), "End of file");
    }

    TEST(marketPacketGeneratorTest, multipleCalls)
    {
        constexpr const size_t NUM_CALLS = 5;

        {
            marketPacket::marketPacketGenerator_t mpg = createDefaultGenerator();
            mpg.initialize();

            for (size_t i = 0; i < NUM_CALLS; i++)
            {
                EXPECT_FALSE(mpg.generatePackets(1, 1).has_value());
            }
        }

        // We can only check an upper bound
        size_t expectedSize = NUM_CALLS * (sizeof(marketPacket::packetHeader_t) + sizeof(marketPacket::update_t));
        EXPECT_LE(std::ifstream(GENERATE_PATH, std::ifstream::ate | std::ifstream::binary).tellg(), expectedSize);

        marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
        mpp.initialize();

        EXPECT_FALSE(mpp.processNextPacket(NUM_CALLS).has_value());
        EXPECT_EQ(mpp.processNextPacket(1).value(), "End of file");
    }

    /**
     * This is a weird case of two classes verifying the other.
     * Past basic tests, we assume basic functionality works at scale for the processor for this test.
     */
    TEST(marketPacketGeneratorTest, genericStressTest_long)
    {
        // Should take about a minute
        constexpr const size_t NUM_PACKETS = 40000;

        {
            marketPacket::marketPacketGenerator_t mpg = createDefaultGenerator();
            mpg.initialize();

            EXPECT_FALSE(mpg.generatePackets(NUM_PACKETS, marketPacket::MAX_UPDATES_ALLOWED_IN_PACKET).has_value());
        }

        marketPacket::marketPacketProcessor_t mpp = createDefaultProcessor();
        mpp.initialize();

        EXPECT_FALSE(mpp.processNextPacket(NUM_PACKETS).has_value());
        EXPECT_EQ(mpp.processNextPacket(1).value(), "End of file");
    }
}