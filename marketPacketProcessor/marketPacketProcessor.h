#pragma once

#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include "marketPacketHelpers/marketPacketHelpers.h"

namespace marketPacket
{
    /**
     * Processes input stream one packet at a time and translates to output stream
     */
    class marketPacketProcessor_t
    {
    public:
        /**
         * @brief Construct a new marketPacketProcessor_t object
         *
         * @param iStream   Input stream, where we get our data from
         * @param oStream   Output stream, where to write the interpreted updates
         */
        marketPacketProcessor_t(std::ifstream&& iStream, std::ofstream&& oStream)
            : m_state(state_t::UNINITIALIZED),
              m_failReason(),
              m_numPacketsToProcess(),
              m_bodySize(),
              m_bodyBytesInterpreted(),
              m_numUpdatesPacket(),
              m_numUpdatesRead(),
              m_packetHeader(),
              m_readBuffer(),
              m_tradeLocs(),
              m_inputStream(std::move(iStream)),
              m_outputStream(std::move(oStream)){};

        /**
         * @brief Sets up the processor for use. Processor won't work unless this is called
         */
        void initialize();

        /**
         * @brief If available, processes the next packet in the input stream.
         *
         * @return If we didn't do any work, why
         */
        const std::optional<failReason_t> &processNextPacket(const std::optional<size_t> &numPacketsToProcess = std::nullopt);

    private:
        /**
         * @brief Possible states for a processor to be in
         */
        enum class state_t : uint8_t
        {
            ERROR = 0,

            UNINITIALIZED,
            CHECK_STREAM_VALIDITY,
            READ_HEADER,
            READ_PART_BODY,
            WRITE_UPDATES
        };

        /**
         * @brief State Machine Functions
         */
        void runStateMachine();
        void uninitialized();       // Tells user processor isn't initialized
        void checkStreamValidity(); // Makes sure input stream has data and can be read from
        void readHeader();          // Reads in a header to get metadata about body and how to read it
        void readPartBody();        // Buffered reads packet body
        void writeUpdates();        // Takes buffered reads and interprets them to output stream as readable updates

        /**
         * @brief Checks conditions to see if we can move on from the current packet
         *
         * @return True if we're read the number of updates we expect
         */
        bool doneWithPacket();

        /**
         * @brief Checks if ptr points to something we'd consider a valid update
         *
         * @param updatePtr Ptr into read buffer on what we assume is the start to an update
         * @return If valid, a tuple containing the length and type of the update
         */
        std::tuple<uint16_t, updateType_e> isValidUpdatePtr(const std::byte *updatePtr);

        /**
         * @brief Certain variables need to be reset per run and/or per packet
         */
        void resetPerRunVariables(const std::optional<size_t> &numPacketsToProcess);
        void resetPerPacketVariables();

        /**
         * @brief Outputs relevant information about trade to output stream
         *
         * @param t trade ptr
         */
        void appendTradePtrToStream(const trade_t *t);

        state_t m_state;                          // Current state of processor
        std::optional<failReason_t> m_failReason; // If processNextPacket() returns false, the reason

        std::size_t m_numPacketsProcessed;           // In this run, how many packets have we seen so far
        std::optional<size_t> m_numPacketsToProcess; // If set, how many packets to try to read. Otherwise, go until failure

        size_t m_bodySize;             // Size of the packet body
        size_t m_bodyBytesInterpreted; // Number of bytes in the body have been interpreted so far
        size_t m_numUpdatesPacket;     // Number of updates in this packet body
        size_t m_numUpdatesRead;       // Number of updates we've read so far

        packetHeader_t m_packetHeader;                        // Packet header we read into
        std::array<std::byte, READ_BUFFER_SIZE> m_readBuffer; // Where we read parts of the packet body into
        std::vector<const std::byte *> m_tradeLocs;           // Locations, by ptr, of trades we need to interpret

        std::ifstream m_inputStream;  // Input stream
        std::ofstream m_outputStream; // Output stream
    };
};