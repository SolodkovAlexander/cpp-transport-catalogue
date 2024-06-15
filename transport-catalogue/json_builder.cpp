#include "json_builder.h"

namespace json {

Builder::Builder() {
    nodes_stack_.emplace_back(&root_);
}

Builder::DictContext Builder::StartDict() {
    // Если строка последн. элемент (значит перед нами были StartDict, Key) или если у нас массив
    if (GetLastNode()->IsString() || GetLastNode()->IsArray()) {
        auto dict_node = new Node(Dict{});
        nodes_stack_.push_back(dict_node);
    } else if (nodes_stack_.size() == 1) { 
        // Если размер стэка всего один: устанавливаем значение (дальше Key, EndDict)
        Node::Value& last_node_value = GetLastNode()->GetValue();
        last_node_value = std::move(Dict{});
    } else {
        throw std::logic_error("Builder.StartDict: Failed: try to start dict to the wrong place");
    }
    
    return Builder::DictContext{*this};
}

Builder::MainContext Builder::EndDict() {
    if (GetLastNode()->IsDict()) {
        if (nodes_stack_.size() == 1) {
            nodes_stack_.pop_back();
        } else {
            // Получаем наш законченный словарь
            auto dict_node = GetLastNode();
            nodes_stack_.pop_back();

            // Завершаем в словаре ключ-значение или добавляем в массив элемент
            Value(dict_node->GetValue());
            delete dict_node;
        }
    } else {
        throw std::logic_error("Builder.EndDict: Failed: last node is not dict");
    }

    return Builder::MainContext{*this};
}

Builder::DictKeyValueContext Builder::Key(std::string dict_key) {
    // Если последн. элемент словарь: добавляем ключ словаря как последн. элемент (дальше Value, StartArray, StartDict)
    if (GetLastNode()->IsDict()) {
        auto key_node = new Node{std::move(dict_key)};
        nodes_stack_.emplace_back(key_node);
    } else {
        throw std::logic_error("Builder.Key: Failed: last node is not dict");
    }
    
    return Builder::DictKeyValueContext{*this};
}

Builder::ArrayContext Builder::StartArray() {
    // Если строка последн. элемент (значит перед нами были StartDict, Key) или если у нас массив
    if (GetLastNode()->IsString() || GetLastNode()->IsArray()) {
        auto dict_node = new Node(Array{});
        nodes_stack_.push_back(dict_node);
    } else if (nodes_stack_.size() == 1) { 
        // Если размер стэка всего один: устанавливаем значение (дальше Value, StartDict, EndArray)
        GetLastNode()->GetValue().emplace<Array>(Array{});
    } else {
        throw std::logic_error("Builder.StartArray: Failed: try to start dict to the wrong place");
    }
    
    return Builder::ArrayContext{*this};
}

Builder::MainContext Builder::EndArray() {
    if (GetLastNode()->IsArray()) {
        if (nodes_stack_.size() == 1) {
            nodes_stack_.pop_back();
        } else {
            // Получаем наш массив
            auto array_node = GetLastNode();
            nodes_stack_.pop_back();

            // Завершаем в словаре ключ-значение или добавляем в след. массив элемент
            Value(array_node->GetValue());
            delete array_node;
        }
    } else {
        throw std::logic_error("Builder.EndDict: Failed: last node is not dict");
    }
    return Builder::MainContext{*this};
}

Builder::MainContext Builder::Value(Node::Value value) {
    // Если строка последн. элемент: значит перед нами были StartDict, Key
    if (GetLastNode()->IsString()) {
        // Забираем ключ
        auto key = std::move((std::get<std::string>(GetLastNode()->GetValue())));
        delete GetLastNode();
        nodes_stack_.pop_back();

        // Устанавливаем в словаре значение (дальше Key, EndDict)
        (std::get<Dict>(GetLastNode()->GetValue()))[key] = Node{std::move(value)};
    } else if (GetLastNode()->IsArray()) {
        // Если массив последн. элемент
        Node value_node;
        value_node.GetValue() = std::move(value);
        (std::get<Array>(GetLastNode()->GetValue())).push_back(value_node);
    } else if (nodes_stack_.size() == 1) {
        // Если размер стэка всего один: устанавливаем значение (конец: JSON закончен)
        GetLastNode()->GetValue() = std::move(value);
        nodes_stack_.pop_back();
    } else {
        throw std::logic_error("Builder.Value: Failed: try to set value to the wrong place");
    }
    
    return Builder::MainContext{*this};
}

Node Builder::Build() {
    if (!nodes_stack_.empty()) {
        throw std::logic_error("Builder.Build: Failed: JSON not finished");
    }
    return std::move(root_);
}

Node* Builder::GetLastNode() {
    if (nodes_stack_.empty()) {
        throw std::logic_error("Builder.GetLastNode: Failed: builder already has finished JSON");
    }
    return nodes_stack_.back();
}

}  // namespace json