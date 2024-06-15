#include "json.h"

#include <stdexcept>
#include <unordered_set>

using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);

Node LoadArray(istream& input) {
    Array result;
    char c;
    for (; input >> c && c != ']';) {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    if (c != ']') {
        throw json::ParsingError("load array failed: invalid JSON");
    }

    return Node{std::move(result)};
}

Node LoadNumber(istream& input) {
    std::string int_part;
    bool is_negative{input.peek() == '-'};
    if (is_negative) {
        input.get();
    }

    bool exp_int_part{false};
    if (input.peek() == '0') {
        int_part += input.get();
    } else {
        while (isdigit(input.peek())) {
            int_part += input.get();
        }
        if (int_part.empty()) {
            throw json::ParsingError("load number failed: invalid JSON");
        }
        if (input.peek() == 'e' || input.peek() == 'E') {
            exp_int_part = true;
            int_part += input.get(); //e

            char exp_sign = '+';
            if (input.peek() == '-' || input.peek() == '+') {
                exp_sign = input.get();
            }
            int_part += exp_sign; //e+ | e-

            while (isdigit(input.peek())) {
                int_part += input.get();
            }
            if (int_part.back() == exp_sign) {
                throw json::ParsingError("load number failed: invalid JSON");
            }
        }
    }
    if (is_negative) {
        int_part = '-' + int_part;
    }

    //double
    if (exp_int_part) {
        return Node{std::stod(int_part)};
    }

    //int
    if (input.peek() != '.') {
        return Node{std::stoi(int_part)};
    }

    int_part += input.get(); //0.

    std::string fract_part;
    while (isdigit(input.peek())) {
        fract_part += input.get();
    }
    if (fract_part.empty()) {
        throw json::ParsingError("load number failed: invalid JSON");
    }

    //double
    if (input.peek() != 'e' && input.peek() != 'E') {
        return Node{std::stod(int_part + fract_part)};
    }

    fract_part += input.get(); //e

    char exp_sign = '+';
    if (input.peek() == '-' || input.peek() == '+') {
        exp_sign = input.get();
    }
    fract_part += exp_sign; //e+ | e-

    while (isdigit(input.peek())) {
        fract_part += input.get();
    }
    if (fract_part.back() == exp_sign) {
        throw json::ParsingError("load number failed: invalid JSON");
    }

    //double
    return Node{std::stod(int_part + fract_part)};
}

Node LoadString(istream& input) {
    string line;
    bool is_esc_last_char{false};
    while (input.peek() != istream::traits_type::eof() && (input.peek() != '"' || is_esc_last_char)) {
        if (is_esc_last_char
            && (input.peek() == 'n' || input.peek() == 'r' || input.peek() == '"' || input.peek() == 't' || input.peek() == '\\')) {
            switch (input.peek()) {
            case 'n': line.back() = '\n'; break;
            case 'r': line.back() = '\r'; break;
            case '"': line.back() = '"'; break;
            case 't': line.back() = '\t'; break;
            case '\\': line.back() = '\\'; break;
            default: break;
            }
            input.get();
            is_esc_last_char = false;
        } else {
            line += input.get();
            is_esc_last_char = (line.back() == '\\');
        }
    }
    if (input.peek() != '"' || input.peek() == istream::traits_type::eof()) {
        throw json::ParsingError("load string failed: invalid JSON");
    }
    input.get();

    return Node{move(line)};
}

Node LoadNull(istream& input) {
    std::string value_str(4, '\0');
    input.read(&value_str[0], 4);
    if (value_str != "null"s) {
        throw json::ParsingError("load null failed: invalid JSON");
    }
    return Node();
}

Node LoadBool(istream& input, bool is_true) {
    size_t value_str_size(is_true ? 4 : 5);
    std::string value_str(value_str_size, '\0');
    input.read(&value_str[0], value_str_size);
    if (value_str != "true"s && value_str != "false"s) {
        throw json::ParsingError("load bool failed: invalid JSON");
    }
    return Node(is_true);
}

Node LoadDict(istream& input) {
    static const std::unordered_set<char> useless_chars = { ' ', '\t','\r','\n' };

    Dict result;
    char c;
    for (; input >> c && c != '}';) {
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input).AsString();

        //Избавляемся от бесполезных символов
        while (useless_chars.count(c)) {
            input >> c;
        }
        if (!(input >> c) || c != ':') {
            throw json::ParsingError("load dict failed: invalid JSON");
        }

        result.insert({move(key), LoadNode(input)});
    }
    if (c != '}') {
        throw json::ParsingError("load dict failed: invalid JSON");
    }

    return Node(move(result));
}

Node LoadNode(istream& input) {
    char c;
    input >> c;

    //Избавляемся от бесполезных символов
    static const std::unordered_set<char> useless_chars = { ' ', '\t','\r','\n' };
    while (useless_chars.count(c)) {
        input >> c;
    }

    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input, (c == 't'));
    } else {
        input.putback(c);
        return LoadNumber(input);
    }
}

}  // namespace

void Node::Print(std::ostream &output) const {
    std::visit(DataPrinter{output}, static_cast<NodeParentClass>(*this));
}

int Node::AsInt() const {
    if (!IsInt()) {
        throw std::logic_error("other type of data"s);
    }
    return *std::get_if<int>(this);
}
bool Node::AsBool() const {
    if (!IsBool()) {
        throw std::logic_error("other type of data");
    }
    return *std::get_if<bool>(this);
}
double Node::AsDouble() const {
    if (!IsDouble()) {
        throw std::logic_error("other type of data");
    }
    if (!IsPureDouble()) {
        if (IsInt()) {
            return static_cast<double>(*std::get_if<int>(this));
        }
        throw std::logic_error("other type of double data");
    }
    return *std::get_if<double>(this);
}
const std::string& Node::AsString() const {
    if (!IsString()) {
        throw std::logic_error("other type of data");
    }
    return *std::get_if<std::string>(this);
}
const Array& Node::AsArray() const {
    if (!IsArray()) {
        throw std::logic_error("other type of data");
    }
    return *std::get_if<Array>(this);
}
const Dict& Node::AsDict() const {
    if (!IsDict()) {
        throw std::logic_error("other type of data");
    }
    return *std::get_if<Dict>(this);
}

Document::Document(Node root)
    : root_(move(root))
{}

const Node& Document::GetRoot() const {
    return root_;
}

Document Load(istream& input) {
    return Document{LoadNode(input)};
}

void Print(const Document& doc, std::ostream& output) {
    doc.GetRoot().Print(output);
}

}  // namespace json
