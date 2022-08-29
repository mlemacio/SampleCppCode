#pragma once

#include <assert.h>
#include <random>

#include "marketPacketStrings.h"


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
        std::byte dynamicData[6];
    } __attribute__((packed));

    struct trade_t
    {
        uint16_t length;
        updateType_e type;
        char symbol[SYMBOL_LENGTH];
        uint16_t tradeSize;
        uint64_t tradePrice;
        std::byte dynamicData[14];
    } __attribute__((packed));

    struct update_t
    {
        std::byte data[32];
    } __attribute__((packed));

    constexpr const size_t TYPE_OFFSET = 2;

    constexpr const size_t READ_BUFFER_SIZE = 16384;
    constexpr const size_t WRITE_BUFFER_SIZE = 16384;

    constexpr const size_t UPDATE_SIZE = sizeof(update_t);
    constexpr const size_t PACKET_HEADER_SIZE = sizeof(packetHeader_t);
    constexpr const size_t UPDATES_IN_WRITE_BUF = WRITE_BUFFER_SIZE / sizeof(trade_t);
    constexpr const size_t MAX_UPDATES_ALLOWED_IN_PACKET = (std::numeric_limits<decltype(marketPacket::packetHeader_t::packetLength)>::max() / UPDATE_SIZE) - 1;

    // Aligned buffers generally make life a lot easier
    static_assert(READ_BUFFER_SIZE % UPDATE_SIZE == 0);
    static_assert(WRITE_BUFFER_SIZE % UPDATE_SIZE == 0);

    // Forcing one size lets us make a lot of assumptions that make things way smoother
    static_assert(sizeof(quote_t) == UPDATE_SIZE);
    static_assert(sizeof(trade_t) == UPDATE_SIZE);

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

    /**
     * @brief Transforms raw trade data in human readable format
     * 
     * @param t trade ptr
     * @return std::string How we want the trade should look to a human
     */
    std::string generateTradeString(const trade_t *t);
}