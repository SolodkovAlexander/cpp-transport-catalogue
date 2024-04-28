#pragma once

#include "geo.h"
#include "transport_catalogue.h"

#include <string>
#include <string_view>
#include <deque>

namespace transport::commands {
namespace detail {
struct CommandDescription {
    // Определяет, задана ли команда (поле command непустое)
    explicit operator bool() const {
        return !command.empty();
    }

    bool operator!() const {
        return !operator bool();
    }

    std::string command;      // Название команды
    std::string id;           // id маршрута или остановки
    std::string description;  // Параметры команды
};

CommandDescription ParseCommandDescription(std::string_view line);
geo::Coordinates ParseCoordinates(std::string_view str);
std::vector<std::string_view> ParseRoute(std::string_view route);
}

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
    std::deque<detail::CommandDescription> commands_;
};
}
