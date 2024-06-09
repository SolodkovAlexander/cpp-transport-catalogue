#include "map_renderer.h"

#include <array>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std::literals;

namespace renderer {

MapRenderer::MapRenderer(MapRenderer::RenderSettings render_settings)
    : render_settings_(std::move(render_settings))
{}

void MapRenderer::SetRenderSettings(MapRenderer::RenderSettings render_settings) {
    std::swap(render_settings_, render_settings);
}

svg::Document MapRenderer::RenderMap(std::vector<BusPtr>&& buses, 
                                     std::unordered_set<BusPtr>&& roundtrip_buses) const {
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

    // Сортируем маршруты в лексиграфическом порядке
    std::sort(buses.begin(), buses.end(), [](BusPtr lhs, BusPtr rhs){ return lhs->id < rhs->id; });

    // Кэш для хранения SVG точки остановки
    std::map<StopPtr, svg::Point, StopCmp> stop_to_point;

    // Элементы названий маршрутов для рендеринга 
    std::vector<svg::Text> bus_names_items;
    bus_names_items.reserve(buses.size() * 4);
    
    // Шаблон элемента названия маршрута
    svg::Text bus_name_template;
    bus_name_template.SetOffset(render_settings_.bus_label_offset);
    bus_name_template.SetFontSize(render_settings_.bus_label_font_size);
    bus_name_template.SetFontFamily("Verdana"s);
    bus_name_template.SetFontWeight("bold"s);

    //Рендерим маршруты
    auto bus_color(render_settings_.color_palette.cbegin());
    for (auto bus : buses) {
        if (bus->stops.empty()) {
            continue;
        }

        // Валидируем цвет маршрута
        if (bus_color == render_settings_.color_palette.cend()) {
            bus_color = render_settings_.color_palette.cbegin();
        }
        
        // Формируем список точек ломаной маршрута
        std::vector<svg::Point> bus_points;
        bus_points.reserve(bus->stops.size());
        for (auto stop : bus->stops) {
            if (!stop_to_point.count(stop)) {
                stop_to_point[stop] = sphere_projector(stop->coordinates);
            }
            bus_points.push_back(stop_to_point.at(stop));
        }

        // Рендерим ломаную маршрута
        svg::Polyline bus_polyline;
        bus_polyline.SetFillColor("none"s);
        bus_polyline.SetStrokeWidth(render_settings_.line_width);
        bus_polyline.SetStrokeColor(*bus_color);
        bus_polyline.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        bus_polyline.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        for (const auto& bus_point : bus_points) {
            bus_polyline.AddPoint(bus_point);
        }
        render.Add(bus_polyline);

        // Рендерим название маршрута
        // Устанавливаем координаты первой остановки и название маршрута
        bus_name_template.SetPosition(*bus_points.begin());
        bus_name_template.SetData(bus->id);

        // Рендерим подложку названия маршрута
        svg::Text bus_name_background(bus_name_template);
        bus_name_background.SetFillColor(render_settings_.underlayer_color);
        bus_name_background.SetStrokeColor(render_settings_.underlayer_color);
        bus_name_background.SetStrokeWidth(render_settings_.underlayer_width);
        bus_name_background.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        bus_name_background.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        bus_names_items.push_back(bus_name_background);

        // Рендерим текст названия маршрута
        svg::Text bus_name_text(bus_name_template);
        bus_name_text.SetFillColor(*bus_color);
        bus_names_items.push_back(bus_name_text);

        // Если маршрут не кольцевой: добавляем название конечной точки маршрута
        if (!roundtrip_buses.count(bus)) {
            svg::Text bus_name_background_end_stop(bus_name_background);
            bus_name_background_end_stop.SetPosition(bus_points.at(bus_points.size() / 2));
            bus_names_items.push_back(bus_name_background_end_stop);
            
            svg::Text bus_name_text_end_stop(bus_name_text);
            bus_name_text_end_stop.SetPosition(bus_points.at(bus_points.size() / 2));
            bus_names_items.push_back(bus_name_text_end_stop);
        }

        // Меняем цвет для след. маршрута
        bus_color = std::next(bus_color);
    }
    for (auto& bus_name_item : bus_names_items) {
        render.Add(std::move(bus_name_item));
    }
    
    // Элементы названий маршрутов для рендеринга 
    std::vector<svg::Text> stop_names_items;
    stop_names_items.reserve(stop_to_point.size() * 2);

    // Шаблон элемента названия остановки
    svg::Text stop_name_template;
    stop_name_template.SetOffset(render_settings_.stop_label_offset);
    stop_name_template.SetFontSize(render_settings_.stop_label_font_size);
    stop_name_template.SetFontFamily("Verdana"s);

    // Рендерим остановки
    for (const auto& [stop, stop_point] : stop_to_point) {
        // Рендерим точку остановки
        svg::Circle stop_circle;
        stop_circle.SetCenter(stop_point);
        stop_circle.SetRadius(render_settings_.stop_radius);
        stop_circle.SetFillColor("white"s);
        render.Add(stop_circle);

        // Рендерим название остановки
        // Устанавливаем координаты и название остановки
        stop_name_template.SetPosition(stop_point);
        stop_name_template.SetData(stop->id);

        // Рендерим подложку названия остановки
        svg::Text stop_name_background(stop_name_template);
        stop_name_background.SetFillColor(render_settings_.underlayer_color);
        stop_name_background.SetStrokeColor(render_settings_.underlayer_color);
        stop_name_background.SetStrokeWidth(render_settings_.underlayer_width);
        stop_name_background.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        stop_name_background.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        stop_names_items.push_back(stop_name_background);

        // Рендерим текст названия остановки
        svg::Text stop_name_text(stop_name_template);
        stop_name_text.SetFillColor("black"s);
        stop_names_items.push_back(stop_name_text);
    }
    for (auto& stop_name_item : stop_names_items) {
        render.Add(std::move(stop_name_item));
    }

    return render;
}

}  // namespace renderer

