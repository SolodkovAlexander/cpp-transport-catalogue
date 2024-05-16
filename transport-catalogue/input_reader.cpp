#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <regex>
#include <unordered_map>
#include <vector>

using namespace std::literals;

/**
 * Парсит строку вида "10.123,  -30.1837" и возвращает пару координат (широта, долгота)
 */
geo::Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == std::string_view::npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    const double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    const double lng = std::stod(std::string(str.substr(not_space2)));

    return {lat, lng};
}

/**
 * Парсит строку вида "3900m to Marushkino, 21m to Eliseevka" и возвращает ассоц. массив: название остановки -> дистанция до этой остановки
 */
std::unordered_map<std::string, int> ParseDistanceToStops(std::string str) {
    static const auto distance_to_stop_reg = std::regex(R"((\d+)m\sto\s([^,]+))");
    std::unordered_map<std::string, int> distance_to_stops;
    for (std::smatch match; std::regex_search(str, match, distance_to_stop_reg); str = match.suffix()) {
        distance_to_stops.emplace(match[2], std::stoi(match[1]));
    }
    return distance_to_stops;
}

/**
 * Удаляет пробелы в начале и конце строки
 */
std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == std::string_view::npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

/**
 * Разбивает строку string на n строк, с помощью указанного символа-разделителя delim
 */
std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == std::string_view::npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

/**
 * Парсит маршрут.
 * Для кольцевого маршрута (A>B>C>A) возвращает массив названий остановок [A,B,C,A]
 * Для некольцевого маршрута (A-B-C-D) возвращает массив названий остановок [A,B,C,D,C,B,A]
 */
std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != std::string_view::npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

namespace transport {
CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == std::string_view::npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    static const std::unordered_map<std::string, CommandType> command_type_to_str = {
        {"Stop"s, CommandType::kStop},
        {"Bus"s, CommandType::kBus}
    };
    const auto command_type_str = std::string(line.substr(0, space_pos));

    return {(command_type_to_str.count(command_type_str) 
             ? command_type_to_str.at(command_type_str) 
             : CommandType::kUnknown),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        if (command_description.command == CommandType::kStop) {
            commands_.push_front(std::move(command_description));
        } else {
            commands_.push_back(std::move(command_description));
        }
    }
}

void InputReader::ApplyCommands(transport::TransportCatalogue& catalogue) const {
    //Команды отсортированы [Stop commands ..., Other commands ...]
    //Создаем остановки, затем определяем расстояния между ними
    for (const auto& command_description : commands_) {
        if (command_description.command != CommandType::kStop) {
            break;
        }
        catalogue.AddStop(command_description.id, ParseCoordinates(command_description.description));
    }
    for (const auto& command_description : commands_) {
        if (command_description.command != CommandType::kStop) {
            break;
        }
        for (const auto& [stop_id, distance] : ParseDistanceToStops(command_description.description)) {
            catalogue.SetStopDistance(catalogue.GetStop(command_description.id),
                                      catalogue.GetStop(stop_id),
                                      distance);
        }
    }

    for (const auto& command_description : commands_) {
       if (command_description.command == CommandType::kBus) { 
            //Создаем маршрут
            auto route = ParseRoute(command_description.description);
            std::vector<StopPtr> route_stops{route.size(), nullptr};
            std::transform(route.begin(), route.end(), 
                           route_stops.begin(), [&](std::string_view stop_id) -> StopPtr { return catalogue.GetStop(stop_id); });            
            catalogue.AddBus(command_description.id, std::move(route_stops));
        }
    }
}
} // namespace transport
