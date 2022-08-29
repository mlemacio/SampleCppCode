#include <assert.h>
#include <cmath>
#include <cstring>
#include <fstream>
#include <memory>
#include <vector>

#include "marketPacketGenerator.h"

namespace marketPacket
{

    void marketPacketGenerator_t::initialize()
    {
        assert(m_oStream != nullptr);

        // Make sure this only gets called once
        if (m_state != state_t::UNINITIALIZED)
        {
            assert(false);
            m_failReason.emplace(UNINITIALIZED);
            return;
        }

        // Ideally, these are random every time but we don't *need* it to be
        // And that'd slow down performance by a lot making a random one everytime
        // There are solutions to that, just none of them are simple
        m_trade = trade_t{
            .length = sizeof(trade_t),
            .type = updateType_e::TRADE,
            .tradeSize = static_cast<uint16_t>(rand()),
            .tradePrice = static_cast<uint64_t>(rand()),
        };

        m_quote = quote_t{
            .length = sizeof(quote_t),
            .type = updateType_e::QUOTE,
            .priceLevel = static_cast<uint16_t>(rand()),
            .priceLevelSize = static_cast<uint64_t>(rand()),
            .timeOfDay = static_cast<uint64_t>(rand())};

        std::memcpy(m_trade.symbol, generateRandomSymbol().c_str(), SYMBOL_LENGTH);
        std::memcpy(m_quote.symbol, generateRandomSymbol().c_str(), SYMBOL_LENGTH);

        m_state = state_t::WRITE_HEADER;
    };

    const std::optional<failReason_t> &marketPacketGenerator_t::generatePackets(size_t numPackets, size_t numMaxUpdates)
    {
        resetPerRunVariables(numPackets, numMaxUpdates);

        runStateMachine();

        m_oStream->flush();
        return m_failReason;
    };

    void marketPacketGenerator_t::runStateMachine()
    {
        while (!m_failReason.has_value())
        {
            switch (m_state)
            {

            case state_t::UNINITIALIZED:
            {
                uninitialized();
                m_state = state_t::WRITE_HEADER;
                break;
            }

            case state_t::WRITE_HEADER:
            {
                writeHeader();
                m_state = state_t::GENERATE_UPDATES;
                break;
            }

            case state_t::GENERATE_UPDATES:
            {
                generateUpdates();

                // Have we written the right number of updates for this packet
                if (m_numUpdatesWritten == m_numUpdates)
                {
                    m_state = state_t::WRITE_HEADER;
                    m_numPacketsWritten++;

                    // Have we written the right number of packets
                    if (m_numPacketsWritten == m_numPackets)
                    {
                        m_state = state_t::WRITE_HEADER;
                        return;
                    }
                }

                break;
            }

            default:
            {
                assert(false);
                m_failReason.emplace(INVALID_STATE);
                return;
            }
            }
        }
    };

    void marketPacketGenerator_t::uninitialized()
    {
        m_failReason.emplace(UNINITIALIZED);
        return;
    };

    void marketPacketGenerator_t::writeHeader()
    {
        // Figure out how many updates we're going to do this packet
        // Gives us [1, n_numMaxUpdates]
        m_numUpdates = rand() % m_numMaxUpdates;
        m_numUpdates++;

        // This is kind of an annoying write you can't easily pack into the other writes
        m_ph.numMarketUpdates = m_numUpdates;
        m_ph.packetLength = sizeof(packetHeader_t) + m_numUpdates * sizeof(trade_t);

        if (!(m_oStream->write(reinterpret_cast<char *>(&m_ph), sizeof(m_ph))))
        {
            m_failReason.emplace(HEADER_WRITE_FAILED);
            return;
        }

        resetPerPacketVariables();
    };

    void marketPacketGenerator_t::generateUpdates()
    {
        size_t numUpdatesToGenerate = UPDATES_IN_WRITE_BUF;
        if (m_numUpdates - m_numUpdatesWritten <= UPDATES_IN_WRITE_BUF)
        {
            numUpdatesToGenerate = m_numUpdates - m_numUpdatesWritten;
        }

        for (size_t i = 0; i < numUpdatesToGenerate; i++)
        {
            // Pick randomly betweern a trade or quote and write it to buffer
            const void *srcPtr = (rand() % 2) ? reinterpret_cast<void *>(&m_trade) : reinterpret_cast<void *>(&m_quote);
            memcpy(&m_updates[i], srcPtr, UPDATE_SIZE);
        }

        if (!(m_oStream->write(reinterpret_cast<char *>(m_updates.data()), numUpdatesToGenerate * sizeof(trade_t))))
        {
            m_failReason.emplace(UPDATE_WRITE_FAILED);
            return;
        }

        m_numUpdatesWritten += numUpdatesToGenerate;
    };

    void marketPacketGenerator_t::resetPerRunVariables(size_t numPackets, size_t numMaxUpdates)
    {
        // Due to the way the struct is constructed, this number needs to stay in a certain range or we can't interpret it
        if (numMaxUpdates > MAX_UPDATES_ALLOWED_IN_PACKET)
        {
            m_failReason.emplace(TOO_MANY_UPDATES);
            return;
        }

        // Reset some state variable for this run
        m_numMaxUpdates = numMaxUpdates;
        m_numPackets = numPackets;
        m_numPacketsWritten = 0;
    }

    void marketPacketGenerator_t::resetPerPacketVariables()
    {
        m_numUpdatesWritten = 0;
    }
};