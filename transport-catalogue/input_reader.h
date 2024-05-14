#pragma once

#include "geo.h"
#include "transport_catalogue.h"

#include <string>
#include <string_view>
#include <deque>

namespace transport {
enum class CommandType {
    kUnknown = 0,
    kStop,
    kBus
};

struct CommandDescription {
    // Определяет, известна ли команда
    explicit operator bool() const {
        return !(command == CommandType::kUnknown);
    }

    bool operator!() const {
        return !operator bool();
    }

    CommandType command;      // Тип команды
    std::string id;           // id маршрута или остановки
    std::string description;  // Параметры команды
};

class InputReader {
public:
    /**
     * Парсит строку в структуру CommandDescription и сохраняет результат в commands_
     */
    void ParseLine(std::string_view line);

    /**
     * Наполняет данными транспортный справочник, используя команды из commands_
     */
    void ApplyCommands(transport::TransportCatalogue& catalogue) const;

private:
    std::deque<CommandDescription> commands_;
};
} // namespace transport
