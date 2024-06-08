#pragma once

#include "json.h"
#include "transport_catalogue.h"

#include <iostream>

/*
 * Код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

namespace transport {

using namespace std::literals;
const std::string JSON_BASE_REQUESTS_KEY = "base_requests"s;
const std::string JSON_STAT_REQUESTS_KEY = "stat_requests"s;

// Заполняет данные в транспортном каталоге
void FillTransportCatalogue(TransportCatalogue& db, 
                            const json::Document& doc);

// Выполняет запросы статистики к транспортному каталогу
json::Document ExecuteStatRequests(const TransportCatalogue& db, 
                                   const json::Document& doc);

}  // namespace transport