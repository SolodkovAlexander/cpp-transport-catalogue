#pragma once

#include "json.h"

using namespace std::literals;

namespace json {

class Builder {

// Обозначаем, с какими классами (до их определения) будем работать
private:
    class DictContext;
    class DictKeyValueContext;
    class ArrayContext;

private:
    // Контекст JSON по умолчанию (реализованы все переходы делегированием Builder-у)
    class MainContext {
    public:
        MainContext(Builder& builder) 
            : builder_(builder) 
        {}

    public:
        // Методы для работы только с словарем
        DictContext StartDict() { return builder_.StartDict(); }
        MainContext EndDict() { return builder_.EndDict(); }
        DictKeyValueContext Key(std::string dict_key) { return builder_.Key(std::move(dict_key)); }

        // Методы по работе только с массивом
        ArrayContext StartArray() { return builder_.StartArray(); }
        MainContext EndArray() { return builder_.EndArray(); }

        // Метод по работе с константами/значением по ключу словаря/элементом массива
        MainContext Value(Node::Value value) { return builder_.Value(std::move(value)); }
        
        // Метода получения заврешенного JSON объекта
        Node Build() { return builder_.Build(); }

    protected:
        Builder& builder_;
    };

    // Контекст JSON словаря, т.е. после вызова метода StartDict
    class DictContext : public MainContext {
    public:
        DictContext(MainContext base) : MainContext(base) {}
        DictContext StartDict() = delete;
        DictContext StartArray() = delete;
        DictContext EndArray() = delete;
        MainContext Value(Node::Value value) = delete;
        Node Build() = delete;
    };
    // Контекст JSON значения по ключу словаря, т.е. после вызова метода Key в словаре (после вызова метода StartDict)
    class DictKeyValueContext : public MainContext {
    public:
        DictKeyValueContext(MainContext base) : MainContext(base) {}
        DictContext EndDict() = delete;
        DictKeyValueContext Key(std::string dict_key) = delete;        
        DictContext EndArray() = delete;
        DictContext Value(Node::Value value) {
            return MainContext::Value(std::move(value));
        };
        Node Build() = delete;
    };
    // Контекст JSON массива, т.е. после вызова метода StartArray
    class ArrayContext : public MainContext {
    public:
        ArrayContext(MainContext base) : MainContext(base) {}
        DictContext EndDict() = delete;
        MainContext Key(std::string dict_key) = delete;
        ArrayContext Value(Node::Value value) {
            return MainContext::Value(std::move(value));
        };
        Node Build() = delete;
    };

public:
    Builder();

public:
    DictContext StartDict();
    MainContext EndDict();
    DictKeyValueContext Key(std::string dict_key);
     
    ArrayContext StartArray();
    MainContext EndArray();

    MainContext Value(Node::Value value); 

    Node Build();

private:
    Node* GetLastNode();

private:
    Node root_;
    std::vector<Node*> nodes_stack_;
};

}  // namespace json

/**
 * Tests compilation errors:
 *  
    // 1
    json::Builder{}.StartDict().Key(""s).Value(""s);
    json::Builder{}.StartDict().Key(""s).StartArray();
    json::Builder{}.StartDict().Key(""s).StartDict();
    json::Builder{}.StartDict().Key(""s).EndDict();
    json::Builder{}.StartDict().Key(""s).EndArray();
    json::Builder{}.StartDict().Key(""s).EndDict();

    // 2
    json::Builder{}.StartDict().Key(""s).Value(""s).Key("s");
    json::Builder{}.StartDict().Key(""s).Value(""s).EndDict();
    json::Builder{}.StartDict().Key(""s).Value(""s).Value(""s);
    json::Builder{}.StartDict().Key(""s).Value(""s).StartArray();
    json::Builder{}.StartDict().Key(""s).Value(""s).StartDict();
    json::Builder{}.StartDict().Key(""s).Value(""s).EndArray();

    // 3
    json::Builder{}.StartDict().Key("s");
    json::Builder{}.StartDict().EndDict();
    json::Builder{}.StartDict().Value(""s);
    json::Builder{}.StartDict().StartArray();
    json::Builder{}.StartDict().StartDict();
    json::Builder{}.StartDict().EndArray();

    // 4
    json::Builder{}.StartArray().Key("s");
    json::Builder{}.StartArray().EndDict();
    json::Builder{}.StartArray().Value(""s);
    json::Builder{}.StartArray().StartArray();
    json::Builder{}.StartArray().StartDict();
    json::Builder{}.StartArray().EndArray();

    // 5.1
    json::Builder{}.StartArray().Value(""s).Key("s");
    json::Builder{}.StartArray().Value(""s).EndDict();
    json::Builder{}.StartArray().Value(""s).Value(""s);
    json::Builder{}.StartArray().Value(""s).StartArray();
    json::Builder{}.StartArray().Value(""s).StartDict();
    json::Builder{}.StartArray().Value(""s).EndArray();
    
    // 5.2
    json::Builder{}.StartArray().Value(""s).Value(""s).Key("s");
    json::Builder{}.StartArray().Value(""s).Value(""s).EndDict();
    json::Builder{}.StartArray().Value(""s).Value(""s).Value(""s);
    json::Builder{}.StartArray().Value(""s).Value(""s).StartArray();
    json::Builder{}.StartArray().Value(""s).Value(""s).StartDict();
    json::Builder{}.StartArray().Value(""s).Value(""s).EndArray(); 
 * 
 */
