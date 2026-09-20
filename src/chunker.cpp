#include "aiws/chunker.hpp"
#include "aiws/text_processor.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace aiws {

Chunker::Chunker(ChunkingPolicy policy)
    : policy_(policy)
{
    if (policy_.max_tokens == 0 ||
        policy_.overlap >= policy_.max_tokens ||
        policy_.paragraph_window > policy_.max_tokens) {
        throw std::invalid_argument("invalid chunking policy");
    }
}

std::vector<Chunk> Chunker::chunk(
    const Document& document,
    std::size_t document_order) const
{
    const auto tokens = TextProcessor::tokenize(document.text());

    std::vector<Chunk> chunks;
    std::size_t begin = 0;

    while (begin < tokens.size()) {

        std::size_t end =
            std::min(begin + policy_.max_tokens, tokens.size());

        if (tokens.size() - begin > policy_.max_tokens) {

            const std::size_t earliest =
                begin + policy_.max_tokens - policy_.paragraph_window;

            std::size_t best = 0;

            for (std::size_t candidate = earliest;
                 candidate <= begin + policy_.max_tokens &&
                 candidate < tokens.size();
                 ++candidate) {

                if (candidate > 0 &&
                    tokens[candidate - 1].paragraph !=
                    tokens[candidate].paragraph) {

                    best = candidate;
                }
            }

            if (best != 0) {
                end = best;
            }
        }

        Chunk item;

        item.id =
            document.id() + "#" + std::to_string(chunks.size());

        item.document_id = document.id();
        item.document_order = document_order;
        item.sequence = chunks.size();

        item.text =
            TextProcessor::join(tokens, begin, end);

        item.token_count = end - begin;

        item.source_begin =
            tokens[begin].begin;

        item.source_end =
            tokens[end - 1].end;

        chunks.push_back(std::move(item));

        if (end == tokens.size()) {
            break;
        }

        begin = end - policy_.overlap;
    }

    return chunks;
}

}  // namespace aiws
