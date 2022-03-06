#pragma once

#include <chrono>
#include <iostream>

class ScopeStopwatch : public vsg::Visitor
{
public:
    ScopeStopwatch(std::string message) : message(message) {
        timestampStart = getCurrentTimestamp();
    };

    ~ScopeStopwatch() {
        outputPassedTime();
    }

    void outputPassedTime() {
        uint64_t timestampEnd = getCurrentTimestamp();
        uint64_t totalTime = timestampEnd - timestampStart;
        std::cout << message << ": " << std::to_string(totalTime) << " ms" << std::endl;
    }

    std::string message;
    uint64_t timestampStart;

private:
    uint64_t getCurrentTimestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
};