#pragma once

#include <cassert>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

namespace svg {

using namespace std::literals;

struct Rgb {
    Rgb() = default;
    Rgb(uint8_t _red, uint8_t _green, uint8_t _blue)
        : red(_red)
        , green(_green)
        , blue(_blue) 
    {}

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};
inline std::ostream& operator<<(std::ostream &out, Rgb val) {
    out << "rgb("sv
        << unsigned(val.red)
        << ',' << unsigned(val.green)
        << ',' << unsigned(val.blue)
        << ')';
    return out;
}

struct Rgba : Rgb {
    Rgba() = default;
    Rgba(uint8_t _red, uint8_t _green, uint8_t _blue, double _opacity)
        : Rgb(_red, _green, _blue)
        , opacity(_opacity) 
    {}
    
    double opacity = 1.0;
};
inline std::ostream& operator<<(std::ostream &out, Rgba val) {
    out << "rgba("sv << unsigned(val.red)
        << ',' << unsigned(val.green)
        << ',' << unsigned(val.blue)
        << ',' << val.opacity
        << ')';
    return out;
}

inline const std::string NoneColor = "none"s;

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
inline std::ostream& operator<<(std::ostream &out, Color color) {
    if (std::holds_alternative<std::monostate>(color)) {
        out << "none"sv;
    } else {
        visit([&out](auto value) { out << value; }, color);
    }
    return out;
}

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};
inline std::ostream& operator<<(std::ostream &out, StrokeLineCap val) {
    static const std::unordered_map<StrokeLineCap, std::string> view_info = {
        {StrokeLineCap::BUTT, "butt"s},
        {StrokeLineCap::ROUND, "round"s},
        {StrokeLineCap::SQUARE, "square"s}
    };
    assert(view_info.count(val));

    out << view_info.at(val);
    return out;
}

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};
inline std::ostream& operator<<(std::ostream &out, StrokeLineJoin val) {
    static const std::unordered_map<StrokeLineJoin, std::string>  view_info = {
        {StrokeLineJoin::ARCS, "arcs"s},
        {StrokeLineJoin::BEVEL, "bevel"s},
        {StrokeLineJoin::MITER, "miter"s},
        {StrokeLineJoin::MITER_CLIP, "miter-clip"s},
        {StrokeLineJoin::ROUND, "round"s}
    };
    assert(view_info.count(val));

    out << view_info.at(val);
    return out;
}

struct Point {
    Point() = default;
    Point(double _x, double _y)
        : x(_x)
        , y(_y) {
    }
    double x = 0;
    double y = 0;
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& _out)
        : out(_out) {
    }

    RenderContext(std::ostream& _out, int _indent_step, int _indent = 0)
        : out(_out)
        , indent_step(_indent_step)
        , indent(_indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class ObjectContainer {
protected:
        // Интерфейс не предполагает полиморфное удаление
        // Поэтому деструктор объявлен защищённым невиртуальным
        ~ObjectContainer() = default;

public:
    /*
     Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
    */
    template<typename ObjectType>
    void Add(ObjectType obj) {
        AddPtr(std::make_unique<ObjectType>(std::move(obj)));
    }

    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
};

class Drawable {
public:
    // Объекты Drawable могут удаляться полиморфно. Поэтому деструктор объявляем публичным виртуальным
    virtual ~Drawable() = default;

    virtual void Draw(ObjectContainer& obj_container) const = 0;
};

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }
    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_line_cap_ = line_cap;
        return AsOwner();
    }
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_line_join_ = line_join;
        return AsOwner();
    }    

protected:
    ~PathProps() = default;

    // Метод RenderAttrs выводит в поток общие для всех атрибуты
    void RenderAttrs(std::ostream& out) const {
        if (fill_color_) {
            out << " fill=\""sv << *fill_color_ << "\""sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv << *stroke_color_ << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        }
        if (stroke_line_cap_) {
            out << " stroke-linecap=\""sv << *stroke_line_cap_ << "\""sv;
        }
        if (stroke_line_join_) {
            out << " stroke-linejoin=\""sv << *stroke_line_join_ << "\""sv;
        }
    }

private:
    Owner& AsOwner() {
        // static_cast безопасно преобразует *this к Owner&,
        // если класс Owner — наследник PathProps
        return static_cast<Owner&>(*this);
    }

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_line_cap_;
    std::optional<StrokeLineJoin> stroke_line_join_;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:
    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;
    
    std::deque<Point> points_;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
public:
    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

private:
    void RenderObject(const RenderContext& context) const override;

private:
    struct Font {
        void Print(std::ostream& out) const {
            out << "font-size=\""sv << size_<< "\" "sv;
            if (family_) {
                out << "font-family=\""sv << *family_ << "\" "sv;
            }
            if (weight_) {
                out << "font-weight=\""sv << *weight_ << "\" "sv;
            }
        }

        uint32_t size_ = 1;
        std::optional<std::string> family_;
        std::optional<std::string> weight_;
    };

private:
    std::string data_;
    Font font_;
    Point position_;
    Point offset_;
};

class Document : public ObjectContainer {
public:
    // Добавляет в svg-документ объект-наследник svg::Object
    void AddPtr(std::unique_ptr<Object>&& obj) override;

    // Выводит в ostream svg-представление документа
    void Render(std::ostream& out) const;

private:
    std::deque<std::unique_ptr<Object>> objects_;
};

}  // namespace svg