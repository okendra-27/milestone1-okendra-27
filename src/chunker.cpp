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

        // Prefer a paragraph boundary near the end of the chunk.
        if (tokens.size() - begin > policy_.max_tokens) {

            const std::size_t earliest =
                begin + policy_.max_tokens - policy_.paragraph_window;

            for (std::size_t candidate = end;
                 candidate >= earliest;
                 --candidate) {

                if (candidate < tokens.size() &&
                    tokens[candidate - 1].paragraph !=
                    tokens[candidate].paragraph) {

                    end = candidate;
                    break;
                }

                if (candidate == earliest) {
                    break;
                }
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

        // No more tokens.
        if (end == tokens.size()) {
            break;
        }

        // Start the next chunk with the required overlap.
        begin = end - policy_.overlap;
    }

    return chunks;
}

}  // namespace aiws