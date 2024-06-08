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
    
    // Обрабатываем запросы на создание данных транспортного каталога
    transport::TransportCatalogue db;
    transport::FillTransportCatalogue(db, json_doc);

    // Обработка запросов к ТК
    json::Document requests_result(transport::ExecuteStatRequests(db, json_doc));

    //Печатаем результаты запросов к ТК
    json::Print(requests_result, std::cout);

    return 0;
}