#pragma once

#include "json.h"

using namespace std::literals;

namespace json {

class Builder {
public:
    Builder();
    
public: 
    class DictItemContext {
    public:
        DictItemContext(Builder& builder) : builder_(builder) {}

    public:
        class DictItemKeyContext {
        public:
            DictItemKeyContext(Builder& builder) : builder_(builder) {}
            DictItemContext Value(Node::Value value) { 
                builder_.Value(std::move(value)); 
                return DictItemContext{builder_};
            }
            auto StartDict() { return builder_.StartDict(); }
            auto StartArray() { return builder_.StartArray(); }
        protected:
            Builder& builder_;
        };

        DictItemKeyContext Key(std::string dict_key) {
            builder_.Key(std::move(dict_key));
            return DictItemKeyContext{builder_};
        }
        Builder& EndDict() { return builder_.EndDict(); }
        
    private:
        Builder& builder_;
    };

    class ArrayItemContext {
    public:
        ArrayItemContext(Builder& builder) : builder_(builder) {}

    public:
        ArrayItemContext Value(Node::Value value) { 
            builder_.Value(std::move(value)); 
            return ArrayItemContext{builder_};
        }
        auto StartDict() { return builder_.StartDict(); }   
        auto StartArray() { return builder_.StartArray(); }
        Builder& EndArray() { return builder_.EndArray(); }
    private:
        Builder& builder_;
    };

public:
    Builder& Key(std::string dict_key);
    Builder& Value(Node::Value value);
    DictItemContext StartDict();    
    ArrayItemContext StartArray();
    Builder& EndDict();
    Builder& EndArray();
    Node Build();

private:
    Node* GetLastNode();

private:
    Node root_;
    std::vector<Node*> nodes_stack_;
};

}  // namespace json