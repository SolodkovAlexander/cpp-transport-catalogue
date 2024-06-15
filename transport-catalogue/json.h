#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

using namespace std::literals;

namespace json {

class Node;
// Сохраните объявления Dict и Array без изменения
using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

// Эта ошибка должна выбрасываться при ошибках парсинга JSON
class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

using NodeParentClass = std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>;
class Node : private NodeParentClass {
public:
    using variant::variant;
    using Value = variant;

    Node(Value value) : variant(std::move(value)) {}

    void Print(std::ostream& output) const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsDict() const;

    inline bool IsInt() const { return std::holds_alternative<int>(*this); }
    inline bool IsPureDouble() const { return std::holds_alternative<double>(*this); }
    inline bool IsDouble() const { return IsInt() || IsPureDouble(); }
    inline bool IsBool() const { return std::holds_alternative<bool>(*this); }
    inline bool IsString() const { return std::holds_alternative<std::string>(*this); }
    inline bool IsNull() const { return std::holds_alternative<std::nullptr_t>(*this); }
    inline bool IsArray() const { return std::holds_alternative<Array>(*this); }
    inline bool IsDict() const { return std::holds_alternative<Dict>(*this); }
    
    bool operator==(const Node& rhs) const { return GetValue() == rhs.GetValue(); }
    const Value& GetValue() const { return *this; }
    Value& GetValue() { return *this; }

private:
    struct DataPrinter {
        std::ostream& out;

        inline void operator()(std::nullptr_t) const { out << "null"sv; }
        inline void operator()(int value) const { out << value; }
        inline void operator()(double value) const { out << value; }
        inline void operator()(std::string value) const {
            out << "\"";
            for (auto ch : value) {
                if (ch == '\n' || ch == '\r' || ch == '"' || ch == '\t' || ch == '\\') {
                    out << '\\';
                    switch (ch) {
                    case '\n': out << 'n'; break;
                    case '\r': out << 'r'; break;
                    case '"': out << '"'; break;
                    case '\t': out << 't'; break;
                    case '\\': out << '\\'; break;
                    default: break;
                    }
                } else {
                    out << ch;
                }
            }
            out << "\"";
        }
        inline void operator()(bool value) const { out << std::boolalpha << value << std::noboolalpha; }
        inline void operator()(Array value) const {
            out << '[';
            for (auto it = value.cbegin(); it != value.cend(); ++it) {
                if (it != value.cbegin()) out << ',';
                (*it).Print(out);
            }
            out << ']';
        }
        inline void operator()(Dict value) const {
            out << '{';
            for (auto it = value.cbegin(); it != value.cend(); ++it) {
                if (it != value.cbegin()) out << ',';
                out << '"' << (*it).first << "\":";
                (*it).second.Print(out);
            }
            out << '}';
        }
    };

    template <typename ValueType>
    static void FillNodeData(Node& node, ValueType value) {
        node = std::move(value);
    }
};

inline bool operator!=(const Node& lhs, const Node& rhs) {
    return !(lhs == rhs);
}


class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json
