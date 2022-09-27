#pragma once

#include <array>
#include <iostream>
#include <fstream>
#include <optional>
#include <vector>

#include "marketPacketHelpers/marketPacketHelpers.h"

namespace marketPacket
{
    /**
     * Generates packets to an output stream
     */
    class marketPacketGenerator_t
    {
    public:
        /**
         * @brief Construct a new marketPacketGenerator object
         *
         * @param oStream Where market packets get written to
         */
        marketPacketGenerator_t(std::ofstream&& oStream)
            : m_state(state_t::UNINITIALIZED),
              m_failReason(),
              m_numPackets(),
              m_numPacketsWritten(),
              m_numMaxUpdates(),
              m_numUpdates(),
              m_numUpdatesWritten(),
              m_ph(),
              m_updates(),
              m_oStream(std::move(oStream)){};

        /**
         * @brief Sets up class to do work
         *
         * No work will be done until this is called
         */
        void initialize();

        /**
         * @brief Generate packets of a certain format.
         *
         * Note: This specifically doesn't have inherent "infinite" generation capabilities
         *       It could... but if someone wants to do that, just throw it in a while loop
         *       I do not trust people to not screw up infinite generation as a default
         */
        const std::optional<failReason_t> &generatePackets(size_t numPackets, size_t numMaxUpdates);

    private:
        /**
         * @brief Possible states for a generator to be in
         */
        enum class state_t : uint8_t
        {
            ERROR = 0,

            UNINITIALIZED,
            WRITE_HEADER,
            GENERATE_UPDATES
        };

        /**
         * @brief State Machine Functions
         */
        void runStateMachine();
        void uninitialized();   // Tells user generator hasn't been initialized yet
        void writeHeader();     // Generates some metadata about the packet and writes the header to the stream
        void generateUpdates(); // Buffered generates and writes updates to stream

        /**
         * @brief Certain variables need to be reset per run and/or per packet
         */
        void resetPerRunVariables(size_t numPackets, size_t numMaxUpdates);
        void resetPerPacketVariables();

        state_t m_state;                          // Current state of the generator
        std::optional<failReason_t> m_failReason; // If populated, why we stopped generating

        size_t m_numPackets;        // Number of packets we should generate in this run
        size_t m_numPacketsWritten; // Number of packets we have written to the stream so far in this

        uint16_t m_numMaxUpdates;     // Per packet, what is the max number of updates in said packet
        uint16_t m_numUpdates;        // How many updates we expect to generate in a packet
        uint16_t m_numUpdatesWritten; // How many updates we have written so far

        trade_t m_trade; // Trade to write
        quote_t m_quote; // Quote to write

        packetHeader_t m_ph;                                  // Header we write to the stream
        std::array<update_t, UPDATES_IN_WRITE_BUF> m_updates; // Where we store the updates before we write

        std::ofstream m_oStream; // Output stream
    };
};