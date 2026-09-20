#include "aiws/context_builder.hpp"

#include "aiws/text_processor.hpp"

#include <unordered_set>

namespace aiws {

std::vector<ContextItem> ContextBuilder::build(
    const std::vector<SearchResult>& ranked,
    std::size_t token_budget) const
{
    std::vector<ContextItem> context;
    std::unordered_set<std::string> included;

    std::size_t remaining = token_budget;

    for (const auto& result : ranked) {

        // No more tokens available.
        if (remaining == 0) {
            break;
        }
        
        if (!included.insert(result.chunk_id).second) {
            continue;
        }

        // Count normalized source-content tokens.
        const auto terms = TextProcessor::terms(result.text);

        if (terms.empty()) {
            continue;
        }

        const std::size_t used =
            std::min(terms.size(), remaining);

        const bool truncated =
            used < terms.size();

        ContextItem item;
        item.chunk_id = result.chunk_id;
        item.document_id = result.document_id;
        item.chunk_sequence = result.chunk_sequence;
        item.text = TextProcessor::join(terms, 0, used);
        item.token_count = used;
        item.score = result.score;
        item.truncated = truncated;

        context.push_back(std::move(item));

        remaining -= used;

        if (truncated) {
            break;
        }
    }

    return context;
}

}  // namespace aiws