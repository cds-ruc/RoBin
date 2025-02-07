#include <assert.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cmath>
#include <stack>
#include "Tree.h"
#include "N.cpp"

namespace ART_unsynchronized {

    Tree::Tree(LoadKeyFunction loadKey) : root(new N256(nullptr, 0)), loadKey(loadKey) {
    }

    Tree::~Tree() {
        N::deleteChildren(root);
        N::deleteNode(root);
    }

    TID Tree::lookup(const Key &k) const {
        N *node = nullptr;
        N *nextNode = root;
        uint32_t level = 0;
        bool optimisticPrefixMatch = false;

        while (true) {
            node = nextNode;
            switch (checkPrefix(node, k, level)) { // increases level
                case CheckPrefixResult::NoMatch:
                    return 0;
                case CheckPrefixResult::OptimisticMatch:
                    optimisticPrefixMatch = true;
                    // fallthrough
                case CheckPrefixResult::Match:
                    if (k.getKeyLen() <= level) {
                        return 0;
                    }
                    nextNode = N::getChild(k[level], node);

                    if (nextNode == nullptr) {
                        return 0;
                    }
                    if (N::isLeaf(nextNode)) {
                        TID tid = N::getLeaf(nextNode);
                        if (level < k.getKeyLen() - 1 || optimisticPrefixMatch) {
                            return checkKey(tid, k);
                        }
                        return tid;
                    }
                    level++;
            }
        }
    }

    bool Tree::lookupRange(const Key &, const Key &, Key &, TID [],
                                std::size_t , std::size_t &) const {
        return false;
        /*for (uint32_t i = 0; i < std::min(start.getKeyLen(), end.getKeyLen()); ++i) {
            if (start[i] > end[i]) {
                resultsFound = 0;
                return false;
            } else if (start[i] < end[i]) {
                break;
            }
        }
        TID toContinue = 0;
        std::function<void(const N *)> copy = [&result, &resultSize, &resultsFound, &toContinue, &copy](const N *node) {
            if (N::isLeaf(node)) {
                if (resultsFound == resultSize) {
                    toContinue = N::getLeaf(node);
                    return;
                }
                result[resultsFound] = N::getLeaf(node);
                resultsFound++;
            } else {
                std::tuple<uint8_t, N *> children[256];
                uint32_t childrenCount = 0;
                N::getChildren(node, 0u, 255u, children, childrenCount);
                for (uint32_t i = 0; i < childrenCount; ++i) {
                    const N *n = std::get<1>(children[i]);
                    copy(n);
                    if (toContinue != 0) {
                        break;
                    }
                }
            }
        };
        std::function<void(const N *, uint32_t)> findStart = [&copy, &start, &findStart, &toContinue, &restart, this](
                const N *node, uint32_t level) {
            if (N::isLeaf(node)) {
                copy(node);
                return;
            }

            uint64_t v;
            PCCompareResults prefixResult;
            do {
                v = node->startReading();
                prefixResult = checkPrefixCompare(node, start, level, loadKey);
            } while (!node->stopReading(v));
            switch (prefixResult) {
                case PCCompareResults::Bigger:
                    copy(node);
                    break;
                case PCCompareResults::Equal: {
                    uint8_t startLevel = (start.getKeyLen() > level) ? start[level] : 0;
                    std::tuple<uint8_t, N *> children[256];
                    uint32_t childrenCount = 0;
                    N::getChildren(node, startLevel, 255, children, childrenCount);
                    for (uint32_t i = 0; i < childrenCount; ++i) {
                        const uint8_t k = std::get<0>(children[i]);
                        const N *n = std::get<1>(children[i]);
                        if (k == startLevel) {
                            findStart(n, level + 1);
                        } else if (k > startLevel) {
                            copy(n);
                        }
                        if (toContinue != 0 || restart) {
                            break;
                        }
                    }
                    break;
                }
                case PCCompareResults::SkippedLevel:
                    restart = true;
                    break;
                case PCCompareResults::Smaller:
                    break;
            }
        };
        std::function<void(const N *, uint32_t)> findEnd = [&copy, &end, &toContinue, &restart, &findEnd, this](
                const N *node, uint32_t level) {
            if (N::isLeaf(node)) {
                return;
            }
            uint64_t v;
            PCCompareResults prefixResult;
            do {
                v = node->startReading();
                prefixResult = checkPrefixCompare(node, end, level, loadKey);
            } while (!node->stopReading(v));

            switch (prefixResult) {
                case PCCompareResults::Smaller:
                    copy(node);
                    break;
                case PCCompareResults::Equal: {
                    uint8_t endLevel = (end.getKeyLen() > level) ? end[level] : 255;
                    std::tuple<uint8_t, N *> children[256];
                    uint32_t childrenCount = 0;
                    N::getChildren(node, 0, endLevel, children, childrenCount);
                    for (uint32_t i = 0; i < childrenCount; ++i) {
                        const uint8_t k = std::get<0>(children[i]);
                        const N *n = std::get<1>(children[i]);
                        if (k == endLevel) {
                            findEnd(n, level + 1);
                        } else if (k < endLevel) {
                            copy(n);
                        }
                        if (toContinue != 0 || restart) {
                            break;
                        }
                    }
                    break;
                }
                case PCCompareResults::Bigger:
                    break;
                case PCCompareResults::SkippedLevel:
                    restart = true;
                    break;
            }
        };

        restart:
        restart = false;
        resultsFound = 0;

        uint32_t level = 0;
        N *node = nullptr;
        N *nextNode = root;

        while (true) {
            node = nextNode;
            uint64_t v;
            PCEqualsResults prefixResult;
            do {
                v = node->startReading();
                prefixResult = checkPrefixEquals(node, level, start, end, loadKey);
            } while (!node->stopReading(v));
            switch (prefixResult) {
                case PCEqualsResults::SkippedLevel:
                    goto restart;
                case PCEqualsResults::NoMatch: {
                    return false;
                }
                case PCEqualsResults::Contained: {
                    copy(node);
                    break;
                }
                case PCEqualsResults::StartMatch: {
                    uint8_t startLevel = (start.getKeyLen() > level) ? start[level] : 0;
                    std::tuple<uint8_t, N *> children[256];
                    uint32_t childrenCount = 0;
                    N::getChildren(node, startLevel, 255, children, childrenCount);
                    for (uint32_t i = 0; i < childrenCount; ++i) {
                        const uint8_t k = std::get<0>(children[i]);
                        const N *n = std::get<1>(children[i]);
                        if (k == startLevel) {
                            findStart(n, level + 1);
                        } else if (k > startLevel) {
                            copy(n);
                        }
                        if (restart) {
                            goto restart;
                        }
                        if (toContinue) {
                            break;
                        }
                    }
                    break;
                }
                case PCEqualsResults::BothMatch: {
                    uint8_t startLevel = (start.getKeyLen() > level) ? start[level] : 0;
                    uint8_t endLevel = (end.getKeyLen() > level) ? end[level] : 255;
                    if (startLevel != endLevel) {
                        std::tuple<uint8_t, N *> children[256];
                        uint32_t childrenCount = 0;
                        N::getChildren(node, startLevel, endLevel, children, childrenCount);
                        for (uint32_t i = 0; i < childrenCount; ++i) {
                            const uint8_t k = std::get<0>(children[i]);
                            const N *n = std::get<1>(children[i]);
                            if (k == startLevel) {
                                findStart(n, level + 1);
                            } else if (k > startLevel && k < endLevel) {
                                copy(n);
                            } else if (k == endLevel) {
                                findEnd(n, level + 1);
                            }
                            if (restart) {
                                goto restart;
                            }
                            if (toContinue) {
                                break;
                            }
                        }
                    } else {
                        nextNode = N::getChild(startLevel, node);
                        if (!node->stopReading(v)) {
                            goto restart;
                        }
                        level++;
                        continue;
                    }
                    break;
                }
            }
            break;
        }
        if (toContinue != 0) {
            loadKey(toContinue, continueKey);
            return true;
        } else {
            return false;
        }*/
    }


    TID Tree::checkKey(const TID tid, const Key &k) const {
        Key kt;
        this->loadKey(tid, kt);
        if (k == kt) {
            return tid;
        }
        return 0;
    }

    void Tree::insert(const Key &k, TID tid) {
        N *node = nullptr;
        N *nextNode = root;
        N *parentNode = nullptr;
        uint8_t parentKey, nodeKey = 0;
        uint32_t level = 0;

        while (true) {
            parentNode = node;
            parentKey = nodeKey;
            node = nextNode;

            uint32_t nextLevel = level;

            uint8_t nonMatchingKey;
            Prefix remainingPrefix;
            switch (checkPrefixPessimistic(node, k, nextLevel, nonMatchingKey, remainingPrefix,
                                                           this->loadKey)) { // increases level
                case CheckPrefixPessimisticResult::NoMatch: {
                    assert(nextLevel < k.getKeyLen()); //prevent duplicate key
                    // 1) Create new node which will be parent of node, Set common prefix, level to this node
                    auto newNode = new N4(node->getPrefix(), nextLevel - level);

                    // 2)  add node and (tid, *k) as children
                    newNode->insert(k[nextLevel], N::setLeaf(tid));
                    newNode->insert(nonMatchingKey, node);

                    // 3) update parentNode to point to the new node
                    N::change(parentNode, parentKey, newNode);

                    // 4) update prefix of node
                    node->setPrefix(remainingPrefix,
                                    node->getPrefixLength() - ((nextLevel - level) + 1));

                    return;
                }
                case CheckPrefixPessimisticResult::Match:
                    break;
            }
            assert(nextLevel < k.getKeyLen()); //prevent duplicate key
            level = nextLevel;
            nodeKey = k[level];
            nextNode = N::getChild(nodeKey, node);

            if (nextNode == nullptr) {
                N::insertA(node, parentNode, parentKey, nodeKey, N::setLeaf(tid));
                return;
            }
            if (N::isLeaf(nextNode)) {
                Key key;
                loadKey(N::getLeaf(nextNode), key);

                level++;
                assert(level < key.getKeyLen()); //prevent inserting when prefix of key exists already
                uint32_t prefixLength = 0;
                while (key[level + prefixLength] == k[level + prefixLength]) {
                    prefixLength++;
                }

                auto n4 = new N4(&k[level], prefixLength);
                n4->insert(k[level + prefixLength], N::setLeaf(tid));
                n4->insert(key[level + prefixLength], nextNode);
                N::change(node, k[level - 1], n4);
                return;
            }

            level++;
        }
    }

    bool Tree::update(const Key &k, TID tid) {
        N *node = nullptr;
        N *nextNode = root;
        uint32_t level = 0;
        bool optimisticPrefixMatch = false;

        while (true) {
            node = nextNode;
            switch (checkPrefix(node, k, level)) { // increases level
                case CheckPrefixResult::NoMatch:
                    return false;
                case CheckPrefixResult::OptimisticMatch:
                    optimisticPrefixMatch = true;
                    // fallthrough
                case CheckPrefixResult::Match:
                    if (k.getKeyLen() <= level) {
                        return false;
                    }
                    nextNode = N::getChild(k[level], node);

                    if (nextNode == nullptr) {
                        return false;
                    }
                    if (N::isLeaf(nextNode)) {
                        TID old_tid = N::getLeaf(nextNode);
                        if (level < k.getKeyLen() - 1 || optimisticPrefixMatch) {
                            if (checkKey(old_tid, k) == old_tid) {
                                N::change(node, k[level], N::setLeaf(tid));
                                return true;
                            }
                        }
                        N::change(node, k[level], N::setLeaf(tid));
                        return true;
                    }
                    level++;
            }
        }
    }

    void Tree::remove(const Key &k) {
        N *node = nullptr;
        N *nextNode = root;
        N *parentNode = nullptr;
        uint8_t parentKey, nodeKey = 0;
        uint32_t level = 0;
        //bool optimisticPrefixMatch = false;

        while (true) {
            parentNode = node;
            parentKey = nodeKey;
            node = nextNode;

            switch (checkPrefix(node, k, level)) { // increases level
                case CheckPrefixResult::NoMatch:
                    return;
                case CheckPrefixResult::OptimisticMatch:
                    // fallthrough
                case CheckPrefixResult::Match: {
                    nodeKey = k[level];
                    nextNode = N::getChild(nodeKey, node);

                    if (nextNode == nullptr) {
                        return;
                    }
                    if (N::isLeaf(nextNode)) {
                        assert(parentNode == nullptr || node->getCount() != 1);
                        if (node->getCount() == 2 && node != root) {
                            // 1. check remaining entries
                            N *secondNodeN;
                            uint8_t secondNodeK;
                            std::tie(secondNodeN, secondNodeK) = N::getSecondChild(node, nodeKey);
                            if (N::isLeaf(secondNodeN)) {

                                //N::remove(node, k[level]); not necessary
                                N::change(parentNode, parentKey, secondNodeN);

                                // delete node;
                            } else {
                                //N::remove(node, k[level]); not necessary
                                N::change(parentNode, parentKey, secondNodeN);
                                secondNodeN->addPrefixBefore(node, secondNodeK);

                                // delete node;
                            }
                        } else {
                            N::removeA(node, k[level], parentNode, parentKey);
                        }
                        return;
                    }
                    level++;
                }
            }
        }
    }


    inline typename Tree::CheckPrefixResult Tree::checkPrefix(N *n, const Key &k, uint32_t &level) {
        if (k.getKeyLen() <= level + n->getPrefixLength()) {
            return CheckPrefixResult::NoMatch;
        }
        if (n->hasPrefix()) {
            for (uint32_t i = 0; i < std::min(n->getPrefixLength(), maxStoredPrefixLength); ++i) {
                if (n->getPrefix()[i] != k[level]) {
                    return CheckPrefixResult::NoMatch;
                }
                ++level;
            }
            if (n->getPrefixLength() > maxStoredPrefixLength) {
                level += n->getPrefixLength() - maxStoredPrefixLength;
                return CheckPrefixResult::OptimisticMatch;
            }
        }
        return CheckPrefixResult::Match;
    }

    typename Tree::CheckPrefixPessimisticResult Tree::checkPrefixPessimistic(N *n, const Key &k, uint32_t &level,
                                                                        uint8_t &nonMatchingKey,
                                                                        Prefix &nonMatchingPrefix,
                                                                        LoadKeyFunction loadKey) {
        if (n->hasPrefix()) {
            uint32_t prevLevel = level;
            Key kt;
            for (uint32_t i = 0; i < n->getPrefixLength(); ++i) {
                if (i == maxStoredPrefixLength) {
                    loadKey(N::getAnyChildTid(n), kt);
                }
                uint8_t curKey = i >= maxStoredPrefixLength ? kt[level] : n->getPrefix()[i];
                if (curKey != k[level]) {
                    nonMatchingKey = curKey;
                    if (n->getPrefixLength() > maxStoredPrefixLength) {
                        if (i < maxStoredPrefixLength) {
                            loadKey(N::getAnyChildTid(n), kt);
                        }
                        for (uint32_t j = 0; j < std::min((n->getPrefixLength() - (level - prevLevel) - 1),
                                                          maxStoredPrefixLength); ++j) {
                            nonMatchingPrefix[j] = kt[level + j + 1];
                        }
                    } else {
                        for (uint32_t j = 0; j < n->getPrefixLength() - i - 1; ++j) {
                            nonMatchingPrefix[j] = n->getPrefix()[i + j + 1];
                        }
                    }
                    return CheckPrefixPessimisticResult::NoMatch;
                }
                ++level;
            }
        }
        return CheckPrefixPessimisticResult::Match;
    }

    typename Tree::PCCompareResults Tree::checkPrefixCompare(N *n, const Key &k, uint32_t &level,
                                                        LoadKeyFunction loadKey) {
        if (n->hasPrefix()) {
            Key kt;
            for (uint32_t i = 0; i < n->getPrefixLength(); ++i) {
                if (i == maxStoredPrefixLength) {
                    loadKey(N::getAnyChildTid(n), kt);
                }
                uint8_t kLevel = (k.getKeyLen() > level) ? k[level] : 0;

                uint8_t curKey = i >= maxStoredPrefixLength ? kt[level] : n->getPrefix()[i];
                if (curKey < kLevel) {
                    return PCCompareResults::Smaller;
                } else if (curKey > kLevel) {
                    return PCCompareResults::Bigger;
                }
                ++level;
            }
        }
        return PCCompareResults::Equal;
    }

    typename Tree::PCEqualsResults Tree::checkPrefixEquals(N *n, uint32_t &level, const Key &start, const Key &end,
                                                      LoadKeyFunction loadKey) {
        if (n->hasPrefix()) {
            bool endMatches = true;
            Key kt;
            for (uint32_t i = 0; i < n->getPrefixLength(); ++i) {
                if (i == maxStoredPrefixLength) {
                    loadKey(N::getAnyChildTid(n), kt);
                }
                uint8_t startLevel = (start.getKeyLen() > level) ? start[level] : 0;
                uint8_t endLevel = (end.getKeyLen() > level) ? end[level] : 0;

                uint8_t curKey = i >= maxStoredPrefixLength ? kt[level] : n->getPrefix()[i];
                if (curKey > startLevel && curKey < endLevel) {
                    return PCEqualsResults::Contained;
                } else if (curKey < startLevel || curKey > endLevel) {
                    return PCEqualsResults::NoMatch;
                } else if (curKey != endLevel) {
                    endMatches = false;
                }
                ++level;
            }
            if (!endMatches) {
                return PCEqualsResults::StartMatch;
            }
        }
        return PCEqualsResults::BothMatch;
    }

    long Tree::size() {
        auto size = N::size(root);
        return size + sizeof(root);
    }

    void Tree::print_depth_type_stats(std::string s) {
        std::ofstream out_key_depth("art_" + s + "_key_depth_stats.log");
        if (!out_key_depth.is_open()) {
            std::cerr << "Failed to open file." << std::endl;
            return ;
        }
        out_key_depth << "key,depth" << std::endl;

        std::vector<size_t> depth_distribution;
        std::vector<size_t> type_distribution;
        type_distribution.resize(4, 0);

        size_t sum_depth = 0, sum_keys = 0;
        size_t max_depth = 1;
        std::queue<N*> q;
        std::queue<size_t> d;
        q.push(root);
        d.push(1);

        while (!q.empty()) {
            N* cur_node = q.front();
            size_t cur_depth = d.front();
            q.pop();
            d.pop();

            if (N::isLeaf(cur_node)) {
                cur_depth--;
                sum_keys++;
                sum_depth += cur_depth;
                max_depth = std::max(max_depth, cur_depth);
                if (depth_distribution.size() <= cur_depth) {
                    depth_distribution.resize(cur_depth + 1, 0);
                }
                depth_distribution[cur_depth]++;
                auto ptr = reinterpret_cast<std::pair<uint64_t, uint64_t> *>(N::getLeaf(cur_node));
                if (ptr) {
                    out_key_depth << ptr->first << "," << cur_depth << std::endl;
                }
            } else {
                switch (cur_node->getType()) {
                    case NTypes::N4: {
                        type_distribution[static_cast<int>(NTypes::N4)]++;
                        auto n = static_cast<N4 *>(cur_node);
                        for (uint8_t i = 0; i < 4; ++i) {
                            if (n->get_child(i) != nullptr) {
                                q.push(n->get_child(i));
                                d.push(cur_depth + 1);
                            }
                        }
                        break;
                    }
                    case NTypes::N16: {
                        type_distribution[static_cast<int>(NTypes::N16)]++;
                        auto n = static_cast<N16 *>(cur_node);
                        for (uint8_t i = 0; i < 16; ++i) {
                            if (n->get_child(i) != nullptr) {
                                q.push(n->get_child(i));
                                d.push(cur_depth + 1);
                            }
                        }
                        break;
                    }
                    case NTypes::N48: {
                        type_distribution[static_cast<int>(NTypes::N48)]++;
                        auto n = static_cast<N48 *>(cur_node);
                        for (uint8_t i = 0; i < 48; ++i) {
                            if (n->get_child(i) != nullptr) {
                                q.push(n->get_child(i));
                                d.push(cur_depth + 1);
                            }
                        }
                        break;
                    }
                    case NTypes::N256: {
                        type_distribution[static_cast<int>(NTypes::N256)]++;
                        auto n = static_cast<N256 *>(cur_node);
                        for (uint16_t i = 0; i < 256; ++i) {
                            if (n->get_child(i) != nullptr) {
                                q.push(n->get_child(i));
                                d.push(cur_depth + 1);
                            }
                        }
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
        }

        out_key_depth.close();

        double avg_depth = double(sum_depth) / double(sum_keys);
        double variance = 0;
        for (size_t i = 1; i < depth_distribution.size(); i ++) {
            variance += (i - avg_depth) * (i - avg_depth) * depth_distribution[i];
        }
        variance /= sum_keys;

        std::ofstream out_depth_dist("art_" + s + "_depth_distribution.log");
        std::ofstream out_depth_stats("art_" + s + "_depth_stats.log");
        if (!out_depth_dist.is_open() || !out_depth_stats.is_open()) {
            std::cerr << "Failed to open file." << std::endl;
            return;
        }
        out_depth_dist << "depth,count" << std::endl;
        for (size_t i = 1; i < depth_distribution.size(); i ++) {
            out_depth_dist << i << "," << depth_distribution[i] << std::endl;
        }
        out_depth_stats << "sum_keys: " << sum_keys << std::endl;
        out_depth_stats << "max_depth: " << max_depth << std::endl;
        out_depth_stats << "avg_depth: " << avg_depth << std::endl;
        out_depth_stats << "variance: " << variance << std::endl;
        out_depth_stats << "standard: " << sqrt(variance) << std::endl;
        out_depth_dist.close();
        out_depth_stats.close();

        // type stats
        std::ofstream out_type_dist("art_" + s + "_type_distribution.log");
        if (!out_type_dist.is_open()) {
            std::cerr << "Failed to open file." << std::endl;
            return;
        }
        out_type_dist << "type,count" << std::endl;
        out_type_dist << "N4," << type_distribution[0] << std::endl;
        out_type_dist << "N16," << type_distribution[1] << std::endl;
        out_type_dist << "N48," << type_distribution[2] << std::endl;
        out_type_dist << "N256," << type_distribution[3] << std::endl;
        out_type_dist.close();
    }

    void Tree::verify_structure(std::string s) {
        std::ofstream out_file("art_" + s + "_structure.log");
        std::queue<N*> q;
        std::queue<size_t> d;
        q.push(root);
        d.push(1);

        while (!q.empty()) {
            N* cur_node = q.front();
            size_t cur_depth = d.front();
            q.pop();
            d.pop();

            if (N::isLeaf(cur_node)) {
                cur_depth--;
                TID tid = N::getLeaf(cur_node);
                auto ptr = reinterpret_cast<std::pair<uint64_t, uint64_t> *>(tid);
                if (ptr) {
                    out_file << "Leaf,key=" << ptr->first << ",depth=" << cur_depth << std::endl;
                }
            } else {
                switch (cur_node->getType()) {
                    case NTypes::N4: {
                        out_file << "N4,children=";
                        auto n = static_cast<N4 *>(cur_node);
                        for (uint8_t i = 0; i < 4; ++i) {
                            if (n->get_child(i) != nullptr) {
                                q.push(n->get_child(i));
                                d.push(cur_depth + 1);
                                out_file << i << ",";
                            }
                        }
                        out_file << ",count=" << n->getCount() << std::endl;
                        break;
                    }
                    case NTypes::N16: {
                        out_file << "N16,children=";
                        auto n = static_cast<N16 *>(cur_node);
                        for (uint8_t i = 0; i < 16; ++i) {
                            if (n->get_child(i) != nullptr) {
                                q.push(n->get_child(i));
                                d.push(cur_depth + 1);
                                out_file << i << ",";
                            }
                        }
                        out_file << ",count=" << n->getCount() << std::endl;
                        break;
                    }
                    case NTypes::N48: {
                        out_file << "N48,children=";
                        auto n = static_cast<N48 *>(cur_node);
                        for (uint8_t i = 0; i < 48; ++i) {
                            if (n->get_child(i) != nullptr) {
                                q.push(n->get_child(i));
                                d.push(cur_depth + 1);
                                out_file << i << ",";
                            }
                        }
                        out_file << ",count=" << n->getCount() << std::endl;
                        break;
                    }
                    case NTypes::N256: {
                        out_file << "N256,children=";
                        auto n = static_cast<N256 *>(cur_node);
                        for (uint16_t i = 0; i < 256; ++i) {
                            if (n->get_child(i) != nullptr) {
                                q.push(n->get_child(i));
                                d.push(cur_depth + 1);
                                out_file << i << ",";
                            }
                        }
                        out_file << ",count=" << n->getCount() << std::endl;
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
        }

        out_file.close();
    }
}