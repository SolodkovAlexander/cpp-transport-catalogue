#include "svg.h"

#include <array>

namespace svg {

using namespace std::literals;

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;

    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\" "sv;
    RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point) {
    points_.emplace_back(std::move(point));
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;

    out << "<polyline points=\""sv;
    for (auto it = points_.cbegin(); it != points_.cend(); ++it) {
        if (it != points_.cbegin()) {
            out << " "sv;
        }
        out << (*it).x << ","sv << (*it).y;
    }
    out << "\" "sv;
    
    RenderAttrs(out);
    out << "/>"sv;
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos) {
    position_ = std::move(pos);
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = std::move(offset);
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    font_.size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_.family_ = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_.weight_ = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;

    out << "<text x=\""sv << position_.x << "\" y=\""sv << position_.y << "\" "sv;
    out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
    font_.Print(out);
    
    static const std::array<std::pair<char, std::string>, 5> char_escaping_from_to = {{
        {'&', "&amp;"s},
        {'"', "&quot;"s},
        {'\'', "&apos;"s},
        {'<', "&lt;"s},
        {'>', "&gt;"s}
    }};
    std::string text = data_;
    for (const auto& char_escaping_info : char_escaping_from_to) {
        std::string::size_type start_pos = 0;
        while ((start_pos = text.find(char_escaping_info.first, start_pos)) != std::string::npos) {
            text.replace(start_pos, 1, char_escaping_info.second);
            start_pos += char_escaping_info.second.size();
        }
    }

    RenderAttrs(out);
    out << ">"sv << text << "</text>"sv;
}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.emplace_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    RenderContext render_context{out, 0, 2};
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv
        << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;
    for (const auto& obj : objects_) {
        obj->Render(render_context);
    }
    out << "</svg>"sv;
}

}  // namespace svg