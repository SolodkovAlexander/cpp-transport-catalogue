#pragma once

#include "map_renderer.h"
#include "svg.h"
#include "transport_catalogue.h"

/*
 * Код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 */

// Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
// с другими подсистемами приложения.
// См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)

class RequestHandler {
public:
    // MapRenderer понадобится в следующей части итогового проекта
    RequestHandler(const transport::TransportCatalogue& db, const renderer::MapRenderer& renderer);

    // Возвращает информацию о маршруте (запрос Bus)
    //std::optional<transport::BusStat> GetBusStat(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через
    //const std::unordered_set<transport::BusPtr>* GetBusesByStop(const std::string_view& stop_name) const;

    // Этот метод будет нужен в следующей части итогового проекта
    //svg::Document RenderMap() const;

    // Считывает запрос и печаетает ответ на запрос (статистику)
    void ParseAndPrintStat(std::string_view request,
                           std::ostream& output);

public:
    // Описание запроса (для разбора запроса в виде строки)
    struct RequestDescription {
        std::string_view request_type;
        std::string_view object_id;
    };

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const transport::TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
};
