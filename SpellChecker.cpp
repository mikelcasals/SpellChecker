#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <chrono>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <list>
#include <condition_variable>
#include <atomic>
#include "TrieNode.h"

struct LineWithID {
    size_t ID;
    std::string line;
};

const size_t MAX_BUFFER_SIZE = 50;
TrieNode g_suggestionsDictionary;
std::unordered_set<std::string> g_dictionary;
char* g_mode;

std::atomic<bool> g_ProducerFinished = false;
std::atomic<bool> g_CoutFinished = false;

std::atomic<size_t> g_runningConsumers = 0;
std::atomic<size_t> g_nextCoutLine = 0;

std::list<LineWithID> g_lines;
std::list<LineWithID> g_analyzedLines;
std::list<LineWithID> g_coutLines;

std::mutex g_lines_mutex;
std::condition_variable g_lines_cv;
std::mutex g_analyzedLines_mutex;
std::condition_variable g_analyzedLines_cv;

class TicToc
{
    std::chrono::time_point<std::chrono::steady_clock> m_begin;

public:
    TicToc() : m_begin(std::chrono::high_resolution_clock::now()) {}
    ~TicToc()
    {
        const auto end = std::chrono::high_resolution_clock::now();
        const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_begin).count();
        std::clog << "Execution time: " << duration / 1000.0 << "ms\n";
    }
};

void loadDictionary(const char* dictionaryFilename) {

    std::ifstream dictionaryFile(dictionaryFilename);

    std::string word;
    while (getline(dictionaryFile, word)) {
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        g_dictionary.insert(word);
        if (strcmp(g_mode, "--suggestionsON") == 0) {
            g_suggestionsDictionary.insert(word);
        }

    }
    dictionaryFile.close();

}

bool checkDigitWord(std::string word) {

    if (word.empty()) {
        return true;
    }

    if (word.back() == 's') {
        word.pop_back();
    }

    for (char c : word) {
        if ((c < '0' || c >'9') && c != '.' && c != ',') {
            return false;
        }
    }

    return true;

}

bool checkValidPossesiveAtEnd(std::string word) {

    if (word.empty()) {
        return true;
    }

    word.pop_back();

    if (word.back() == 's' && g_dictionary.count(word)) {
        return true;
    }

    return false;

}

bool checkValidContraction(std::string word) {

    if (word.empty()) {
        return true;
    }

    std::string WordPart = word.substr(0, word.find('\''));
    std::string ContractionPart = word.substr(word.find('\'') + 1);


    if (ContractionPart == "t" || ContractionPart == "m") {
        return g_dictionary.count(WordPart + ContractionPart);
    }

    if (ContractionPart == "s") {
        if (WordPart.back() == 's') {
            return false;
        }
        else {
            return g_dictionary.count(WordPart);
        }
    }

    std::unordered_set<std::string> otherContractions{ "re", "ve", "ll", "d" };
    if (otherContractions.count(ContractionPart)) {
        return g_dictionary.count(WordPart);
    }

    return false;

}

bool isCorrectlySpelled(std::string word) {

    if (word.empty()) {
        return true;
    }

    std::transform(word.begin(), word.end(), word.begin(), ::tolower);

    if (g_dictionary.count(word)) {
        return true;
    }

    if (checkDigitWord(word)) {
        return true;
    }

    if (word.back() == '\'') {
        return checkValidPossesiveAtEnd(word);
    }

    if (checkValidContraction(word)) {
        return true;
    }

    return false;

}

std::string ProduceSuggestion(std::string word) {

    std::string suggestion;
    std::transform(word.begin(), word.end(), word.begin(), ::tolower);
    std::string WordPart = word.substr(0, word.find('\''));
    std::string ContractionPart = word.substr(word.find('\'') + 1);

    if (word.back() == '\'') {
        suggestion = g_suggestionsDictionary.searchSuggestion(WordPart) + '\'';
        return suggestion;
    }

    if (word == WordPart) {
        suggestion = g_suggestionsDictionary.searchSuggestion(word);
        return suggestion;
    }

    std::unordered_set<std::string> otherContractions{ "re", "ve", "ll", "d", "s" };
    if (otherContractions.count(ContractionPart)) {
        suggestion = g_suggestionsDictionary.searchSuggestion(WordPart) + "\'" + ContractionPart;
        return suggestion;
    }

    return suggestion;
}

std::string AnalyzeLine(std::string line) {

    std::unordered_set<char> specialCharacters{ ' ', '.', ',','\"', '?','!', ':', ';', '-', '(', ')', '[', ']' };
    std::string analyzedLine;
    std::string word;

    for (char c : line) {
        if (specialCharacters.count(c)) {
            if (isCorrectlySpelled(word)) {
                analyzedLine.append(word);
                analyzedLine.push_back(c);
            }
            else if (strcmp(g_mode, "--suggestionsON") == 0) {
                std::string suggestion = ProduceSuggestion(word);
                if (suggestion != "") {
                    analyzedLine.append("<a style=\"color:blue\"><span title = \"");
                    analyzedLine.append(suggestion);
                    analyzedLine.append("\">");
                    analyzedLine.append(word);
                    analyzedLine.append("</a>");
                    analyzedLine.push_back(c);
                }
                else {
                    analyzedLine.append("<a style=\"color:red\"><span title = \"");
                    analyzedLine.append("No suggestion found.");
                    analyzedLine.append("\">");
                    analyzedLine.append(word);
                    analyzedLine.append("</a>");
                    analyzedLine.push_back(c);
                }

            }
            else {
                analyzedLine.append("<a style=\"color:red\">");
                analyzedLine.append(word);
                analyzedLine.append("</a>");
                analyzedLine.push_back(c);

            }
            word.clear();
        }
        else {
            word.push_back(c);
        }
    }
    analyzedLine.append("<br>\n");

    return analyzedLine;

}

void producer(const char* textFilename) {

    std::ifstream textFile(textFilename);
    size_t ID = 0;
    std::string line;

    while (getline(textFile, line)) {
        std::unique_lock linesLock(g_lines_mutex);
        g_lines_cv.wait(linesLock, []() {return g_lines.size() < MAX_BUFFER_SIZE; });

        g_lines.push_back({ ID, line });
        ++ID;

        linesLock.unlock();
        g_lines_cv.notify_all();
    }
    g_ProducerFinished = true;

    textFile.close();

}

void consumer() {

    ++g_runningConsumers;
    while (true) {
        std::unique_lock linesLock(g_lines_mutex);
        g_lines_cv.wait(linesLock, []() {return g_lines.size() > 0 || g_ProducerFinished; });

        if (g_lines.empty() && g_ProducerFinished) {
            break;
        }

        LineWithID line = g_lines.front();
        g_lines.pop_front();

        linesLock.unlock();
        g_lines_cv.notify_all();

        line.line = AnalyzeLine(line.line);

        std::unique_lock analyzedLines_lock(g_analyzedLines_mutex);
        g_analyzedLines_cv.wait(analyzedLines_lock, []() {return g_analyzedLines.size() < MAX_BUFFER_SIZE; });

        g_analyzedLines.push_back(line);

        analyzedLines_lock.unlock();
        g_analyzedLines_cv.notify_all();

    }
    --g_runningConsumers;

}


void coutConsumer() {

    std::cout << "<html>\n";
    while (true) {
        std::unique_lock analyzedLines_lock(g_analyzedLines_mutex);
        g_analyzedLines_cv.wait(analyzedLines_lock, []() {return g_analyzedLines.size() > 0 || g_runningConsumers == 0 || g_ProducerFinished; });

        if (g_analyzedLines.empty() && g_coutLines.empty() && g_runningConsumers == 0 && g_ProducerFinished) {
            break;
        }

        while (g_analyzedLines.size() > 0) {
            g_coutLines.push_back(g_analyzedLines.front());
            g_analyzedLines.pop_front();
        }

        analyzedLines_lock.unlock();
        g_analyzedLines_cv.notify_all();

        while (g_coutLines.front().ID == g_nextCoutLine) {
            std::cout << g_coutLines.front().line;
            g_coutLines.pop_front();
            g_nextCoutLine++;

        }
        if (g_coutLines.size() > 1) {
            g_coutLines.sort([](const LineWithID a, const LineWithID b) { return a.ID < b.ID; });
        }

    }
    std::cout << "</html>\n";
    g_CoutFinished = true;
}

int main(size_t argc, char* argv[]) {

    TicToc tic_toe;

    if (argc < 4) {
        std::clog << "Error, not enough arguments\n";
        return 1;
    }

    char* dictionaryFilename = argv[1];
    char* textFilename = argv[2];
    g_mode = argv[3];

    std::ifstream dictionaryFile(dictionaryFilename);
    if (!dictionaryFile.is_open()) {
        std::clog << "Error, file " << dictionaryFilename << " not found.\n";
        return 1;
    }
    dictionaryFile.close();
    loadDictionary(dictionaryFilename);

    std::ifstream textFile(textFilename);
    if (!textFile.is_open()) {
        std::clog << "Error, file " << textFilename << " not found.\n";
        return 1;
    }
    textFile.close();

    if (strcmp(g_mode, "--suggestionsON") != 0 && strcmp(g_mode, "--suggestionsOFF") != 0) {
        std::clog << "Error, " << g_mode << " is not a valid mode.\n";
        std::clog << "Valid modes:\n ";
        std::clog << "--suggestionsON\n ";
        std::clog << "--suggestionsOFF\n";
        return 1;
    }

    std::thread producer_thread(producer, textFilename);
    std::vector<std::thread> consumer_threads;
    for (size_t i = 0; i < 4; ++i) {
        consumer_threads.emplace_back(consumer);
    }
    std::thread coutConsumer_thread(coutConsumer);

    while (g_runningConsumers > 0 || !g_CoutFinished) {
        g_lines_cv.notify_all();
        g_analyzedLines_cv.notify_all();
    }

    producer_thread.join();
    for (auto& thread : consumer_threads) {
        thread.join();
    }
    coutConsumer_thread.join();

    std::clog << "Finished!\n";
    return 0;

}