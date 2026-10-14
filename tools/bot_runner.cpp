#include "glory/server/load_test.hpp"

#include <charconv>
#include <exception>
#include <iostream>
#include <string_view>

namespace {

template<class Integer>
bool parseInteger(std::string_view text, Integer& value) {
    if (text.empty()) return false;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

} // namespace

int main(int argc, char** argv) {
    glory::server::LoadTestConfig config;
    for (int index = 1; index < argc; ++index) {
        const std::string_view option(argv[index]);
        if (index + 1 >= argc) {
            std::cerr << "Missing value for " << option << '\n';
            return 2;
        }
        const std::string_view value(argv[++index]);
        if (option == "--bots") {
            if (!parseInteger(value, config.botCount)) {
                std::cerr << "Invalid --bots value\n";
                return 2;
            }
        } else if (option == "--ticks") {
            if (!parseInteger(value, config.tickCount)) {
                std::cerr << "Invalid --ticks value\n";
                return 2;
            }
        } else {
            std::cerr << "Unknown option: " << option << "\nUsage: glory_bot_runner [--bots 8] [--ticks 120]\n";
            return 2;
        }
    }

    try {
        const auto metrics = glory::server::LoadTestCoordinator(config).run();
        glory::server::printLoadTestSummary(config, metrics, std::cout);
        return metrics.ticksCompleted == config.tickCount &&
            metrics.connectedBots == config.botCount && metrics.connectedBotsRate() == 1.0 &&
            metrics.avgTickDurationMs < 16.6 && metrics.remainingServerPlayers == 0 ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "Load test failed: " << error.what() << '\n';
        return 1;
    }
}
