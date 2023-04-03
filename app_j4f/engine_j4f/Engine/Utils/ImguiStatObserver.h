#pragma once

#include "Statistic.h"
#include <string>
#include <cstdint>

namespace engine {

    class ImguiStatObserver : public IStatisticObserver {
    public:
        enum class Location : uint8_t {
            top_left        = 0,
            top_right       = 1,
            bottom_left     = 2,
            bottom_right    = 3,
            center          = 4,
            custom          = 5,
            locked          = 6
        };

        ImguiStatObserver(const Location location = Location::bottom_left);
        ~ImguiStatObserver() override;
        void statisticUpdate(const Statistic* s) override;
        void draw();
    private:
        Location _location = Location::bottom_left;

        std::string _timeString = "_";
        std::string _cpuName;
        std::string _gpuName;
        std::string _statString;
        std::string _versions;
    };

}
