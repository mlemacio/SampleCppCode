#include "marketPacketHelpers.h"

namespace marketPacket
{
    size_t rand()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<size_t> dist(0, std::numeric_limits<size_t>::max());

        return dist(gen);
    }

    std::string generateRandomSymbol()
    {
        static constexpr const std::string_view alphanum = "0123456789"
                                                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                           "abcdefghijklmnopqrstuvwxyz";

        std::string tmp_s;
        tmp_s.reserve(SYMBOL_LENGTH);

        for (int i = 0; i < SYMBOL_LENGTH; i++)
        {
            tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
        }

        return tmp_s;
    }

    std::string generateTradeString(const trade_t *t)
    {
        assert(t != nullptr);
        std::string tradeStr;

        // We *could* make this a compile time computation... but c'mon
        // sizeof("Trade: ") + SYMBOL_LENGTH +
        // sizeof(" Size: ") + numDigits(decltype(t->tradeSize)::max())) + 
        // sizeof(" Price: ") + numDigits(decltype(t->tradePrice)::max()))
        // Ends up being 47. Just call it 64.
        tradeStr.reserve(64);

        tradeStr.append("Trade: ");
        tradeStr.append(t->symbol, SYMBOL_LENGTH); // This one is finicky since the symbol isn't guaranteed to be null-terminated
        tradeStr.append(" Size: ");
        tradeStr.append(std::to_string(t->tradeSize));
        tradeStr.append(" Price: ");
        tradeStr.append(std::to_string(t->tradePrice));
        tradeStr.append("\n");

        return tradeStr;
    }
}