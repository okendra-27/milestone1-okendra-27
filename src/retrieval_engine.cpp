#include "aiws/retrieval_engine.hpp"

#include "aiws/text_processor.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace aiws {

double RetrievalEngine::canonical_score(double value) {
    return std::round(value * 1'000'000'000'000.0) / 1'000'000'000'000.0;
}

std::vector<SearchResult> RetrievalEngine::search(const std::string& query,
                                                  int k,
                                                  const std::vector<Chunk>& chunks,
                                                  const CorpusIndex& index) const {
    if (k < 0) throw std::invalid_argument("k must not be negative");
    if (k == 0 || chunks.empty()) return {};

    std::vector<std::string> query_terms;
    std::unordered_set<std::string> seen_terms;
    for (const auto& term : TextProcessor::terms(query)) {
        if (seen_terms.insert(term).second) query_terms.push_back(term);
    }
    if (query_terms.empty()) return {};

    std::unordered_map<std::size_t, std::size_t> matches;
    for (const auto& term : query_terms) {
        const auto* entries = index.postings(term);
        if (entries == nullptr) continue;
        for (const auto& entry : *entries) ++matches[entry.chunk_index];
    }

    std::vector<SearchResult> results;
    results.reserve(matches.size());
    for (const auto& candidate : matches) {
        const Chunk& chunk = chunks[candidate.first];
        double base = 0.0;
        for (const auto& term : query_terms) {
            const std::size_t frequency = index.term_frequency(term, chunk.id);
            if (frequency == 0) continue;
            const double tf = 1.0 + std::log(static_cast<double>(frequency));
            const double idf = std::log((static_cast<double>(chunks.size()) + 1.0) /
                                        (static_cast<double>(index.document_frequency(term)) + 1.0)) + 1.0;
            base += tf * idf;
        }
        const double coverage = 1.0 + 0.10 * static_cast<double>(candidate.second) /
                                            static_cast<double>(query_terms.size());
        results.push_back({chunk.id, chunk.document_id, chunk.sequence, chunk.text,
                           canonical_score(base * coverage), candidate.second});
    }

    std::sort(results.begin(), results.end(), [&chunks, &index](const SearchResult& left,
                                                                  const SearchResult& right) {
        if (left.score != right.score) return left.score > right.score;
        const Chunk* left_chunk = index.find_chunk(chunks, left.chunk_id);
        const Chunk* right_chunk = index.find_chunk(chunks, right.chunk_id);
        if (left_chunk->document_order != right_chunk->document_order) {
            return left_chunk->document_order < right_chunk->document_order;
        }
        return left.chunk_sequence < right.chunk_sequence;
    });
    if (results.size() > static_cast<std::size_t>(k)) results.resize(static_cast<std::size_t>(k));
    return results;
}

}  // namespace aiws
