#include <iostream>
#include <string_view>

#include "marketPacketGenerator/marketPacketGenerator.h"
#include "marketPacketProcessor/marketPacketProcessor.h"

// Ideally, all these go into a config file
constexpr std::string_view GENERATE_PATH = "./input.dat";
constexpr std::string_view INPUT_PATH = "./input.dat";
constexpr std::string_view OUTPUT_PATH = "./output.dat";

constexpr const size_t NUM_PACKETS_TO_GENERATE = 2;
constexpr const size_t MAX_UPDATES_TO_GENERATE = 10;

int main()
{
    // Generate packets
    {
        marketPacket::marketPacketGenerator_t mpg(std::make_shared<std::ofstream>(GENERATE_PATH));
        mpg.initialize();

        const std::optional<std::string> &generatorFailReason = mpg.generatePackets(NUM_PACKETS_TO_GENERATE, MAX_UPDATES_TO_GENERATE);
        if (generatorFailReason.has_value())
        {
            std::cout << "Reason why we stopped generating early: " << generatorFailReason.value() << std::endl;
        }
    }

    // Process all the packets our input stream gives us
    {
        marketPacket::marketPacketProcessor_t mpp(std::make_shared<std::ifstream>(INPUT_PATH), std::make_shared<std::ofstream>(OUTPUT_PATH));
        mpp.initialize();

        // Go until we get a reason to stop
        std::optional<std::string> processorFailReason;
        processorFailReason = mpp.processNextPacket();

        if (processorFailReason.has_value())
        {
            std::cout << "Reason we're stopped processing early: " << processorFailReason.value() << std::endl;
        }
    }
    return EXIT_SUCCESS;
}