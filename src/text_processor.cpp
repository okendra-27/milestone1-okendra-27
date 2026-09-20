#include "aiws/text_processor.hpp"

#include <cctype>
#include <string>
#include <vector>

namespace aiws {

static bool is_ascii_letter(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool is_ascii_digit(char c) {
    return (c >= '0' && c <= '9');
}

static std::vector<std::size_t> detect_paragraph_breaks(const std::string& text) {
    std::vector<std::size_t> breaks;

    auto is_newline = [&](std::size_t i) {
        if (i >= text.size()) return false;
        if (text[i] == '\n') return true;
        if (text[i] == '\r' && i + 1 < text.size() && text[i+1] == '\n') return true;
        return false;
    };

    for (std::size_t i = 0; i < text.size(); ++i) {
        if (!is_newline(i)) continue;

        std::size_t j = i;
        if (text[i] == '\r') j++;  // skip CR
        j++;                       // skip LF

        while (j < text.size() && (text[j] == ' ' || text[j] == '\t')) j++;

        if (is_newline(j)) {
            std::size_t next = j;
            if (text[j] == '\r') next++;
            next++;
            breaks.push_back(next);
        }
    }

    return breaks;
}


std::vector<TokenInfo> TextProcessor::tokenize(const std::string& text) {
    std::vector<TokenInfo> tokens;
    tokens.reserve(text.size() / 4);

    const auto paragraph_breaks = detect_paragraph_breaks(text);

    std::size_t paragraph = 0;
    std::size_t pos = 0;

    auto is_separator = [&](char c) {
        return !is_ascii_letter(c) && !is_ascii_digit(c);
    };

    while (pos < text.size()) {
        for (std::size_t b : paragraph_breaks) {
            if (pos == b) {
                ++paragraph;
                break;
            }
        }

        while (pos < text.size() && is_separator(text[pos])) {
            ++pos;
        }
        if (pos >= text.size()) break;

        std::size_t begin = pos;
        while (pos < text.size() && !is_separator(text[pos])) {
            ++pos;
        }
        std::size_t end = pos;

        std::string token;
        token.reserve(end - begin);
        for (std::size_t i = begin; i < end; ++i) {
            char c = text[i];
            if (is_ascii_letter(c)) {
                token.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            } else if (is_ascii_digit(c)) {
                token.push_back(c);
            }
        }

        if (!token.empty()) {
            TokenInfo info;
            info.token = std::move(token);
            info.begin = begin;
            info.end = end;
            info.paragraph = paragraph;
            tokens.push_back(std::move(info));
        }
    }

    return tokens;
}

std::vector<std::string> TextProcessor::terms(const std::string& text) {
    const std::string norm = normalize(text);
    std::vector<std::string> result;

    std::size_t i = 0;
    while (i < norm.size()) {
        while (i < norm.size() && norm[i] == ' ') {
            ++i;
        }
        if (i >= norm.size()) break;

        std::size_t start = i;
        while (i < norm.size() && norm[i] != ' ') {
            ++i;
        }
        result.emplace_back(norm.substr(start, i - start));
    }

    return result;
}


std::string TextProcessor::normalize(const std::string& input) {
    std::string out;
    out.reserve(input.size());

    bool in_token = false;

    for (char c : input) {
        if (is_ascii_letter(c)) {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            in_token = true;
        } else if (is_ascii_digit(c)) {
            out.push_back(c);
            in_token = true;
        } else {
            if (in_token) {
                out.push_back(' ');
                in_token = false;
            }
        }
    }

    if (!out.empty() && out.back() == ' ') {
        out.pop_back();
    }

    return out;
}



std::string TextProcessor::join(const std::vector<TokenInfo>& tokens,
                                std::size_t begin,
                                std::size_t end) {
    std::string out;

    for (std::size_t i = begin; i < end; ++i) {
        if (!tokens[i].token.empty()) {
            if (!out.empty()) out.push_back(' ');
            out += tokens[i].token;
        }
    }

    return out;
}

std::string TextProcessor::join(const std::vector<std::string>& tokens,
                                std::size_t begin,
                                std::size_t end) {
    std::string out;

    for (std::size_t i = begin; i < end; ++i) {
        if (!tokens[i].empty()) {
            if (!out.empty()) out.push_back(' ');
            out += tokens[i];
        }
    }

    return out;
}

}  // namespace aiws
