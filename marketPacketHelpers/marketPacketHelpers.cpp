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
        static const char alphanum[] = "0123456789"
                                       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "abcdefghijklmnopqrstuvwxyz";

        std::string tmp_s;
        tmp_s.reserve(SYMBOL_LENGTH);

        for (int i = 0; i < SYMBOL_LENGTH; i++)
        {
            // Make sure we don't accidentally use the regular rand()
            tmp_s += alphanum[marketPacket::rand() % (sizeof(alphanum) - 1)];
        }

        return tmp_s;
    }
}