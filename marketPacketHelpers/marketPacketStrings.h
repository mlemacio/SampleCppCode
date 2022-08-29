#pragma once

#include <string>

namespace marketPacket
{
    // In theory, this could get a lot cooler / more in-depth, but that'd be over-engineering at this point
    using failReason_t = const std::string_view;

    static constexpr failReason_t UNINITIALIZED{"Uninitialized"};
    static constexpr failReason_t INVALID_STATE{"Invalid state in state machine. How?"};

    // Generator specific failures
    static constexpr failReason_t HEADER_WRITE_FAILED{"writeHeader() failed"};
    static constexpr failReason_t UPDATE_WRITE_FAILED{"Update write failed"};
    static constexpr failReason_t TOO_MANY_UPDATES{"Can't request that many updates in a packet"};

    // Processor specific failures
    static constexpr failReason_t INPUT_STREAM_CLOSED{"Input stream isn't open"};
    static constexpr failReason_t END_OF_FILE{"End of file"};
    static constexpr failReason_t BAD_STREAM{"Stream is bad"};

    static constexpr failReason_t PACKET_HEADER_READ_FAILED{"Packet header read failed"};
    static constexpr failReason_t PACKET_HEADER_POORLY_FORMED{"Stream is bad"};
    static constexpr failReason_t PACKET_READ_FAILED{"Packet read failed"};

    static constexpr failReason_t UPDATE_POORLY_FORMED{"Poorly formed update"};
    static constexpr failReason_t TRADE_WRITE_FAILED{"Failure in writing trade to stream"};
}