#include "json.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace std;

int main() {
    // Считываем JSON из stdin
    json::Document json_doc(json::Load(std::cin));
    
    // Обрабатываем запросы на создание данных транспортного каталога (ТК)
    transport::TransportCatalogue db;
    transport::FillTransportCatalogue(db, json_doc);
    
    // Обрабатываем настройки для визуализации ТК
    renderer::MapRenderer map_renderer;
    renderer::FillMapRenderer(map_renderer, json_doc);
    
    // Обработчик запросов
    RequestHandler request_handler(db, map_renderer);

    // Рендерим карту
    request_handler.RenderMap().Render(std::cout);

    // Обработка запросов к ТК
    /*json::Document requests_result(transport::ExecuteStatRequests(request_handler, json_doc));
    //Печатаем результаты запросов к ТК
    json::Print(requests_result, std::cout);*/

    return 0;
}