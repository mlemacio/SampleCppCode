#include <gtest/gtest.h>
#include <memory>

#include "marketPacketHelpers/marketPacketHelpers.h"

namespace test
{
    TEST(marketPacketHelpersTest, tradeStringFormat)
    {
        // We DON'T want a new line here, all this function should do is just a barebones translation
        std::string expectedString("Trade: ABCDE Size: 12 Price: 5235");

        // We really only care about these things when printing to user
        marketPacket::trade_t trade {
            .tradeSize = 12,
            .tradePrice = 5235
        };
        memcpy(trade.symbol, "ABCDE", marketPacket::SYMBOL_LENGTH);

        EXPECT_EQ(expectedString, marketPacket::generateTradeString(&trade));
    }
}