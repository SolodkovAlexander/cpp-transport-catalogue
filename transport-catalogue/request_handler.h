#pragma once

#include "graph.h"
#include "map_renderer.h"
#include "router.h"
#include "svg.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <set>

/*
 * Код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 */

// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
// с другими подсистемами приложения.
// См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)

struct BusStat {
    double curvature = 0.0;
    int route_length = 0;
    int stop_count = 0;
    int unique_stop_count = 0;
};

struct StopStat {
    std::set<std::string> bus_names;
};

struct RouteStat {
    struct WaitingOnStopItem {
        std::string_view name;
    };
    struct BusItem {
        std::string_view name;
        int span_count = 0;
        double time = 0.0;
    };

    int bus_wait_time = 0.0;
    double total_time = 0.0;
    std::vector<std::variant<WaitingOnStopItem, BusItem>> items;
};
using RouteStatItem = std::variant<RouteStat::WaitingOnStopItem, RouteStat::BusItem>;

class RequestHandler {
public:
    // MapRenderer понадобится в следующей части итогового проекта
    RequestHandler(const transport::TransportCatalogue& db, const renderer::MapRenderer& renderer);

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<BusStat> GetBusStat(std::string_view bus_name) const;

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<StopStat> GetStopStat(std::string_view stop_name) const;

    // Возвращает информацию о прохождении маршрута (запрос Route)
    std::optional<RouteStat> GetRouteStat(std::string_view stop_name_from, std::string_view stop_name_to) const;

    // Рендерит транспортный каталог
    svg::Document RenderMap() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const transport::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
    TransportRouter db_router_;
};


