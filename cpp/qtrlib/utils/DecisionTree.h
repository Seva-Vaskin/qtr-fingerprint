#pragma once

#include "FingerprintTableView.h"

#include <memory>
#include <stack>
#include <utility>

namespace qtr {

template<class Pred, class Info>
class DecisionNode {
public:
    using Node = DecisionNode<Pred, Info>;

    struct Children {
        Node *_next;  // always go
        Node *_false; // go only if _pred(query) is false
    };

    Children setPred(const Pred &pred) {
        _pred = pred;
        _next = std::make_unique<Node>();
        _false = std::make_unique<Node>();
        return {_next.get(), _false.get()};
    }

    void setInfo(Info &&info) {
        _info = std::move(info);
    }

    void setInfo(const Info &info) {
        _info = info;
    }

    bool isLeaf() const {
        return !_next;
    }

    Info &getInfo() {
        return _info;
    }

    const Info &getInfo() const {
        return _info;
    }

    template<class Query>
    Children next(const Query &query) const {
        return {_next.get(), (_pred(query) ? nullptr : _false.get()) };
    }

private:
    Pred _pred;
    Info _info;

    std::unique_ptr<Node> _next;  // always go
    std::unique_ptr<Node> _false; // go only if _pred(query) is false
};

template<class Pred, class Info>
class DecisionTree {
public:
    using Node = DecisionNode<Pred, Info>;

    Node &getRoot() { return _root; }

    template<class Query>
    std::vector<const Info *> search(const Query &query) const {

        std::vector<const Info *> result;
        
        std::stack<const Node *> stack;
        stack.push(&_root);

        while (!stack.empty()) {
            const Node *node = stack.top();
            stack.pop();

            if (node->isLeaf()) {
                result.push_back(&(node->getInfo()));
            }
            else {
                typename Node::Children children = node->next(query);
                if (children._next) stack.push(children._next);
                if (children._false) stack.push(children._false);
            }
        }

        return result;
    }

private:  
    Node _root;
};

} // namespace qtr