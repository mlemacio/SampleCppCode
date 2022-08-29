#include "marketPacketProcessor.h"

#include <fstream>
#include <assert.h>

namespace marketPacket
{
    void marketPacketProcessor_t::initialize()
    {
        assert(m_inputStream != nullptr);
        assert(m_outputStream != nullptr);

        // Make sure this only gets called once
        if (m_state != state_t::UNINITIALIZED)
        {
            assert(false);
            return;
        }

        m_tradeLocs.reserve(READ_BUFFER_SIZE / UPDATE_SIZE);
        m_state = state_t::CHECK_STREAM_VALIDITY;
    }

    const std::optional<failReason_t> &marketPacketProcessor_t::processNextPacket(const std::optional<size_t> &numPacketsToProcess)
    {
        resetPerRunVariables(numPacketsToProcess);

        runStateMachine();

        m_outputStream->flush();
        return m_failReason;
    }

    void marketPacketProcessor_t::runStateMachine()
    {
        while (!m_failReason.has_value())
        {
            switch (m_state)
            {

            case state_t::UNINITIALIZED:
            {
                uninitialized();
                m_state = state_t::CHECK_STREAM_VALIDITY;
                break;
            }

            case state_t::CHECK_STREAM_VALIDITY:
            {
                if (m_numPacketsToProcess.has_value() && m_numPacketsProcessed == m_numPacketsToProcess.value())
                {
                    // This is our stopping condition
                    return;
                }

                checkStreamValidity();
                m_state = state_t::READ_HEADER;
                break;
            }

            case state_t::READ_HEADER:
            {
                readHeader();
                m_state = state_t::READ_PART_BODY;
                break;
            }

            case state_t::READ_PART_BODY:
            {
                readPartBody();
                m_state = state_t::WRITE_UPDATES;
                break;
            }

            case state_t::WRITE_UPDATES:
            {
                writeUpdates();

                if (doneWithPacket())
                {
                    m_numPacketsProcessed++;
                    m_state = state_t::CHECK_STREAM_VALIDITY;
                    break;
                }

                // If we're not done with the packet yet, go and read some more
                m_state = state_t::READ_PART_BODY;
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
    }

    void marketPacketProcessor_t::uninitialized()
    {
        m_failReason.emplace(UNINITIALIZED);
        return;
    }

    void marketPacketProcessor_t::checkStreamValidity()
    {
        // Don't process, just return early
        if (!m_inputStream->is_open())
        {
            m_failReason.emplace(INPUT_STREAM_CLOSED);
            return;
        }

        // Do a quick peek to set flags if we're at the end of a file
        m_inputStream->peek();
        if (!m_inputStream->good())
        {
            if (m_inputStream->eof())
            {
                m_failReason.emplace(END_OF_FILE);
            }
            else
            {
                m_failReason.emplace(BAD_STREAM);
            }
            return;
        }
    }

    void marketPacketProcessor_t::readHeader()
    {
        // Assume it's a packet header
        if (!(m_inputStream->read(reinterpret_cast<char *>(&m_packetHeader), PACKET_HEADER_SIZE)))
        {
            m_failReason.emplace(PACKET_HEADER_READ_FAILED);
            return;
        }

        // Probably not a good thing
        if (m_packetHeader.packetLength < PACKET_HEADER_SIZE)
        {
            m_failReason.emplace(PACKET_HEADER_POORLY_FORMED);
            return;
        }

        // Reset our state info now that we know about the header
        resetPerPacketVariables();
    }

    void marketPacketProcessor_t::readPartBody()
    {
        // Figure out how much of the buffer we need to use
        size_t bytesLeft = m_bodySize - m_bodyBytesInterpreted;
        size_t validDataInBuffer = (bytesLeft < READ_BUFFER_SIZE) ? bytesLeft : READ_BUFFER_SIZE;

        // Read what needs to be read
        if (!(m_inputStream->read(reinterpret_cast<char *>(m_readBuffer.data()), validDataInBuffer)).good())
        {
            m_failReason.emplace(PACKET_READ_FAILED);
            return;
        }

        // Read the buffer until we run out of material
        // A few tricks here because we know READ_BUFFER_SIZE % UPDATE_SIZE = 0
        size_t bufferOffset = 0;
        while (bufferOffset < validDataInBuffer)
        {
            const std::byte *currBufferPos = m_readBuffer.data() + bufferOffset;

            auto [length, type] = isValidUpdatePtr(currBufferPos);
            if (type == updateType_e::INVALID)
            {
                m_failReason.emplace(UPDATE_POORLY_FORMED);
                return;
            }

            // Mark down we've 'read' an update of somesort
            bufferOffset += length;
            m_bodyBytesInterpreted += length;
            m_numUpdatesRead++;

            switch (type)
            {
            case updateType_e::TRADE:
            {
                // Just mark down where the trade update is for now
                m_tradeLocs.emplace_back(currBufferPos);
                break;
            }

            case updateType_e::QUOTE:
            {
                // Currently, we don't care about quotes. We could though
                break;
            }

            default:
            {
                // You really shouldn't be able to get here
                assert(false);
                m_failReason.emplace(UPDATE_POORLY_FORMED);
                return;
            }
            }
        }
    }

    void marketPacketProcessor_t::writeUpdates()
    {
        // Take all the ptrs we know about and write the information to the output stream
        for (const std::byte *tradePtr : m_tradeLocs)
        {
            appendTradePtrToStream(reinterpret_cast<const trade_t *>(tradePtr));
        }

        m_tradeLocs.clear();
    }

    bool marketPacketProcessor_t::doneWithPacket()
    {
        return m_numUpdatesRead == m_numUpdatesPacket;
    }

    void marketPacketProcessor_t::resetPerRunVariables(const std::optional<size_t> &numPacketsToProcess)
    {
        m_numPacketsToProcess = numPacketsToProcess;
        m_numPacketsProcessed = 0;
    }

    void marketPacketProcessor_t::resetPerPacketVariables()
    {
        m_numUpdatesPacket = m_packetHeader.numMarketUpdates;
        m_numUpdatesRead = 0;

        m_bodySize = m_packetHeader.packetLength - PACKET_HEADER_SIZE;
        m_bodyBytesInterpreted = 0;
    }

    std::tuple<uint16_t, updateType_e> marketPacketProcessor_t::isValidUpdatePtr(const std::byte *updatePtr)
    {
        // Kind of by definition, the first 3 bytes have to be the same format
        const uint16_t length = *(reinterpret_cast<const int16_t *>(updatePtr));
        const updateType_e type = *(reinterpret_cast<const updateType_e *>(updatePtr + TYPE_OFFSET));

        // Is both the length and type something we'd expect?
        if (length == UPDATE_SIZE && (type == updateType_e::TRADE || type == updateType_e::QUOTE))
        {
            return {length, type};
        }

        return {0, updateType_e::INVALID};
    }

    /**
     * std::format (C++20) would do a lot better here if it was available 
     */
    void marketPacketProcessor_t::appendTradePtrToStream(const trade_t *t)
    {
        // We're relying that the outputStream knows how to buffer it's own writes
        *m_outputStream << generateTradeString(t) << std::endl;
        
        // This is just a weird case
        if (!m_outputStream->good())
        {
            m_failReason.emplace(TRADE_WRITE_FAILED);
        }
    }
};