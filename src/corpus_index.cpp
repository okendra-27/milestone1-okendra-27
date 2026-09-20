#include "aiws/corpus_index.hpp"

#include "aiws/text_processor.hpp"

#include <stdexcept>

namespace aiws {

CorpusIndex::CorpusIndex(const std::vector<Chunk>& chunks) {
    build(chunks);
}

void CorpusIndex::build(const std::vector<Chunk>& chunks) {
    std::unordered_map<std::string, std::vector<Posting>> new_postings;
    std::unordered_map<std::string, std::size_t> new_chunk_by_id;

    for (std::size_t chunk_index = 0;
         chunk_index < chunks.size();
         ++chunk_index) {

        const auto inserted =
            new_chunk_by_id.emplace(
                chunks[chunk_index].id,
                chunk_index
            );

        if (!inserted.second) {
            throw std::invalid_argument("duplicate chunk ID");
        }

        std::unordered_map<std::string, std::size_t> frequencies;

        for (const auto& term :
             TextProcessor::terms(chunks[chunk_index].text)) {

            ++frequencies[term];
        }

        for (const auto& entry : frequencies) {
            new_postings[entry.first].push_back(
                {chunk_index, entry.second}
            );
        }
    }

    postings_.swap(new_postings);
    chunk_by_id_.swap(new_chunk_by_id);
}

std::size_t CorpusIndex::document_frequency(
    const std::string& normalized_term) const noexcept {

    const auto found = postings_.find(normalized_term);

    if (found == postings_.end()) {
        return 0;
    }

    return found->second.size();
}

std::size_t CorpusIndex::term_frequency(
    const std::string& normalized_term,
    const std::string& chunk_id) const noexcept {

    const auto chunk = chunk_by_id_.find(chunk_id);
    const auto entries = postings_.find(normalized_term);

    if (chunk == chunk_by_id_.end() ||
        entries == postings_.end()) {
        return 0;
    }

    for (const auto& posting : entries->second) {
        if (posting.chunk_index == chunk->second) {
            return posting.frequency;
        }
    }

    return 0;
}

const std::vector<CorpusIndex::Posting>*
CorpusIndex::postings(
    const std::string& normalized_term) const noexcept {

    const auto found = postings_.find(normalized_term);

    if (found == postings_.end()) {
        return nullptr;
    }

    return &found->second;
}

const Chunk* CorpusIndex::find_chunk(
    const std::vector<Chunk>& chunks,
    const std::string& chunk_id) const noexcept {

    const auto found = chunk_by_id_.find(chunk_id);

    if (found == chunk_by_id_.end() ||
        found->second >= chunks.size()) {
        return nullptr;
    }

    return &chunks[found->second];
}

std::size_t CorpusIndex::chunk_index(
    const std::string& chunk_id) const {

    const auto found = chunk_by_id_.find(chunk_id);

    if (found == chunk_by_id_.end()) {
        throw std::out_of_range("unknown chunk ID");
    }

    return found->second;
}

}  // namespace aiws