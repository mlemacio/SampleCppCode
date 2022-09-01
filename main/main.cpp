#include <iostream>
#include <string_view>

#include "marketPacketGenerator/marketPacketGenerator.h"
#include "marketPacketProcessor/marketPacketProcessor.h"

// Ideally, all these go into a config file
constexpr std::string_view GENERATE_PATH = "./input.dat";
constexpr std::string_view INPUT_PATH = "./input.dat";
constexpr std::string_view OUTPUT_PATH = "/Users/maciejmleczko/projects/SampleCppCode/output.dat";

constexpr const size_t NUM_PACKETS = 2;
constexpr const size_t MAX_UPDATES_PACKET = 10;

int main()
{
    // Generate packets
    {
        marketPacket::marketPacketGenerator_t mpg(std::make_shared<std::ofstream>(GENERATE_PATH));
        mpg.initialize();

        const auto &generatorFailReason = mpg.generatePackets(NUM_PACKETS, MAX_UPDATES_PACKET);
        if (generatorFailReason.has_value())
        {
            std::cout << "Reason why we stopped generating early: " << generatorFailReason.value() << std::endl;
        }
    }

    // Process all the packets our input stream gives us
    {
        marketPacket::marketPacketProcessor_t mpp(std::make_shared<std::ifstream>(INPUT_PATH), std::make_shared<std::ofstream>(OUTPUT_PATH));
        mpp.initialize();

        const auto& processorFailReason = mpp.processNextPacket(NUM_PACKETS);
        if (processorFailReason.has_value())
        {
            std::cout << "Reason we're stopped processing early: " << processorFailReason.value() << std::endl;
        }
    }
    return EXIT_SUCCESS;
}