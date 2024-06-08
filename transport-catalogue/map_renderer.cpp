#include "map_renderer.h"

#include <unordered_map>
#include <set>
#include <string>
#include <unordered_set>

using namespace std::literals;

namespace renderer {

MapRenderer::MapRenderer(MapRenderer::RenderSettings render_settings)
    : render_settings_(std::move(render_settings))
{}

void MapRenderer::SetRenderSettings(MapRenderer::RenderSettings render_settings) {
    std::swap(render_settings_, render_settings);
}

svg::Document MapRenderer::RenderBuses(std::vector<BusPtr>&& buses) const {
    svg::Document render;

    // Вычисляем данные для проецирования координат
    std::deque<geo::Coordinates> stops_coordinates;
    for (auto bus : buses) {
        for (auto stop : bus->stops) {
            stops_coordinates.push_back(stop->coordinates);
        }
    }
    SphereProjector sphere_projector(stops_coordinates.begin(),
                                     stops_coordinates.end(),
                                     render_settings_.width,
                                     render_settings_.height,
                                     render_settings_.padding);

    // Рендерим
    std::sort(buses.begin(), buses.end(), [](BusPtr lhs, BusPtr rhs){ return lhs->id < rhs->id; });
    auto bus_color(render_settings_.color_palette.cbegin());
    std::unordered_map<StopPtr, svg::Point> stop_to_point;
    for (auto bus : buses) {
        if (bus->stops.empty()) {
            continue;
        }

        // Валидируем цвет маршрута
        if (bus_color == render_settings_.color_palette.cend()) {
            bus_color = render_settings_.color_palette.cbegin();
        }
        
        svg::Polyline bus_polyline;
        bus_polyline.SetFillColor("none"s);
        bus_polyline.SetStrokeWidth(render_settings_.line_width);
        bus_polyline.SetStrokeColor(*bus_color);
        bus_polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        bus_polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        for (auto stop : bus->stops) {
            if (!stop_to_point.count(stop)) {
                stop_to_point[stop] = sphere_projector(stop->coordinates);
            }
            bus_polyline.AddPoint(stop_to_point.at(stop));
        }
        render.Add(bus_polyline);

        // Меняем цвет для след. маршрута
        bus_color = std::next(bus_color);
    }

    return render;
}

}  // namespace renderer

