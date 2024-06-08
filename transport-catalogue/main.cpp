#include "input_reader.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

#include <iostream>
#include <string>

using namespace std;

int main() {
    // Транспортный каталог (ТК)
    transport::TransportCatalogue catalogue;

    // Обработка запросов на создание данных ТК
    int base_request_count{0};
    cin >> base_request_count >> ws;
    {
        transport::InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            string line;
            getline(cin, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }

    // Обработка запросов к ТК
    renderer::MapRenderer map_renderer;
    RequestHandler request_handler(catalogue, map_renderer);
    int stat_request_count{0};
    cin >> stat_request_count >> ws;
    for (int i = 0; i < stat_request_count; ++i) {
        string line;
        getline(cin, line);
        request_handler.ParseAndPrintStat(line, cout);
    }

    return 0;
}