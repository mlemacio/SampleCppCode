#pragma once

#include <random>

namespace marketPacket
{
    enum class updateType_e : uint8_t
    {
        INVALID = 0,

        QUOTE = 'Q',
        TRADE = 'T'
    };

    struct packetHeader_t
    {
        uint16_t packetLength;
        uint16_t numMarketUpdates;
    } __attribute__((packed));

    constexpr const size_t SYMBOL_LENGTH = 5;

    struct quote_t
    {
        uint16_t length;
        updateType_e type;
        char symbol[SYMBOL_LENGTH];
        uint16_t priceLevel;
        uint64_t priceLevelSize;
        uint64_t timeOfDay;
        char dynamicData[6];
    } __attribute__((packed));

    struct trade_t
    {
        uint16_t length;
        updateType_e type;
        char symbol[SYMBOL_LENGTH];
        uint16_t tradeSize;
        uint64_t tradePrice;
        char dynamicData[14];
    } __attribute__((packed));

    struct update_t
    {
        char data[32];
    } __attribute__((packed));

    constexpr const size_t TYPE_OFFSET = 2;

    constexpr const size_t READ_BUFFER_SIZE = 1024;
    constexpr const size_t WRITE_BUFFER_SIZE = 1024;

    constexpr const size_t UPDATE_SIZE = sizeof(update_t);
    constexpr const size_t PACKET_HEADER_SIZE = sizeof(packetHeader_t);
    constexpr const size_t UPDATES_IN_WRITE_BUF = WRITE_BUFFER_SIZE / sizeof(trade_t);
    constexpr const size_t MAX_UPDATES_ALLOWED_IN_PACKET = (std::numeric_limits<decltype(marketPacket::packetHeader_t::packetLength)>::max() / UPDATE_SIZE) - 1;

    static_assert(READ_BUFFER_SIZE % UPDATE_SIZE == 0, "This just isn't going to be good if it's not aligned");
    static_assert(WRITE_BUFFER_SIZE % UPDATE_SIZE == 0, "This just isn't going to be good if it's not aligned");

    static_assert(sizeof(quote_t) == UPDATE_SIZE, "All updates need to be the same size");
    static_assert(sizeof(trade_t) == UPDATE_SIZE, "All updates need to be the same size");

    /**
     * @brief rand() is awful as a random number generator. Create our own
     *
     * @return size_t
     */
    size_t rand();

    /**
     * @brief Creates a random symbol string for updates
     *
     * @return Random SYMBOL_LENGTH length stream
     */
    std::string generateRandomSymbol();
}