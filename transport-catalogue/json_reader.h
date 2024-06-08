#pragma once

#include "json.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <iostream>

/*
 * Код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */

namespace transport {

// Заполняет данные в транспортном каталоге
void FillTransportCatalogue(TransportCatalogue& db, 
                            const json::Document& doc);

// Выполняет запросы статистики к транспортному каталогу
json::Document ExecuteStatRequests(const RequestHandler& request_handler, 
                                   const json::Document& doc);

}  // namespace transport

namespace renderer {

void FillMapRenderer(MapRenderer& map_renderer, 
                     const json::Document& doc);

}  // namespace renderer
