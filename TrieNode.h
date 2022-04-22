#pragma once
#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <algorithm>

class TrieNode {

private:
    std::map<char, TrieNode*> children;
    std::string word;
public:
    TrieNode() {}

    void insert(std::string word) {
        TrieNode* node = this;
        for (size_t i = 0; i < word.length(); ++i) {
            if (node->children.find(word[i]) == node->children.end()) {
                node->children[word[i]] = new TrieNode();
            }
            node = node->children[word[i]];
        }

        node->word = word;
    }

    bool contains(std::string word) {
        TrieNode* node = this;
        for (size_t i = 0; i < word.length(); ++i) {
            if (node->children.find(word[i]) == node->children.end()) {
                return false;
            }
            else {
                node = node->children[word[i]];
            }
        }
        return true;
    }

    std::string searchRecursive(TrieNode* node, char ch, std::vector<size_t> lastRow, const std::string word, size_t& maxDistance) {
        std::vector<size_t> currentRow(lastRow.size());
        std::string suggestion;
        currentRow[0] = lastRow[0] + 1;

        size_t insertCost, deleteCost, replaceCost;
        for (size_t i = 1; i < currentRow.size(); ++i) {
            insertCost = currentRow[i - 1] + 1;
            deleteCost = lastRow[i] + 1;
            if (word[i - 1] == ch) {
                replaceCost = lastRow[i - 1];
            }
            else {
                replaceCost = lastRow[i - 1] + 1;
            }

            currentRow[i] = std::min(std::min(insertCost, deleteCost), replaceCost);
        }

        if (currentRow.back() < maxDistance && node->word != "") {
            maxDistance = currentRow.back();
            suggestion = node->word;
        }

        std::string tmp;
        if (*min_element(currentRow.begin(), currentRow.end()) <= maxDistance) {
            for (std::map<char, TrieNode*>::iterator it = node->children.begin(); it != node->children.end(); ++it) {
                tmp = searchRecursive(it->second, it->first, currentRow, word, maxDistance);
                if (tmp != "") {
                    suggestion = tmp;
                }
            }
        }
        return suggestion;
    }

    std::string searchSuggestion(std::string word) {
        std::string suggestion;
        size_t maxDistance = 3;

        std::vector<size_t> currentRow(word.length() + 1);

        for (size_t i = 0; i < word.length() + 1; ++i) {
            currentRow[i] = i;
        }
        std::string tmp;
        for (int i = 0; i < word.length(); ++i) {
            if (this->children.find(word[i]) != this->children.end()) {
                tmp = searchRecursive(this->children[word[i]], word[i], currentRow, word, maxDistance);
                if (tmp != "") {
                    suggestion = tmp;
                }
            }
        }
        return suggestion;

    }
};
