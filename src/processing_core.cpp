#include "aiws/processing_core.hpp"

#include "aiws/chunker.hpp"
#include "aiws/context_builder.hpp"
#include "aiws/corpus_index.hpp"
#include "aiws/retrieval_engine.hpp"
#include "aiws/text_processor.hpp"

#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace aiws {

struct ProcessingCore::Impl {
    std::vector<Chunk> chunks;
    CorpusIndex index;
    RetrievalEngine retrieval;
    ContextBuilder context_builder;
};

ProcessingCore::ProcessingCore()
    : impl_(std::make_unique<Impl>())
{
}

ProcessingCore::~ProcessingCore() = default;

ProcessingCore::ProcessingCore(
    ProcessingCore&&) noexcept = default;

ProcessingCore& ProcessingCore::operator=(
    ProcessingCore&&) noexcept = default;

std::string ProcessingCore::normalize(
    const std::string& text)
{
    return TextProcessor::normalize(text);
}

void ProcessingCore::rebuild(
    const Workspace& workspace)
{
    const auto& documents = workspace.documents();

    // Check for duplicate document IDs first.
    std::unordered_set<std::string> document_ids;

    for (const auto& document : documents) {
        if (!document_ids.insert(document.id()).second) {
            throw std::invalid_argument(
                "workspace contains duplicate document IDs");
        }
    }

    Chunker chunker({
        kMaxChunkTokens,
        kChunkOverlap,
        kParagraphPreferenceWindow
    });

    std::vector<Chunk> new_chunks;

    for (std::size_t order = 0;
         order < documents.size();
         ++order) {

        auto document_chunks =
            chunker.chunk(documents[order], order);

        new_chunks.insert(
            new_chunks.end(),
            document_chunks.begin(),
            document_chunks.end()
        );
    }

    CorpusIndex new_index(new_chunks);

    // Commit only after everything succeeds.
    impl_->chunks.swap(new_chunks);
    impl_->index = std::move(new_index);
}

const std::vector<Chunk>&
ProcessingCore::chunks() const noexcept
{
    return impl_->chunks;
}

std::size_t ProcessingCore::chunk_count() const noexcept
{
    return impl_->chunks.size();
}

std::size_t ProcessingCore::document_frequency(
    const std::string& term) const
{
    const auto terms = TextProcessor::terms(term);

    if (terms.empty()) {
        return 0;
    }

    if (terms.size() != 1) {
        throw std::invalid_argument(
            "term must normalize to one token");
    }

    return impl_->index.document_frequency(
        terms.front());
}

std::size_t ProcessingCore::term_frequency(
    const std::string& term,
    const std::string& chunk_id) const
{
    const auto terms = TextProcessor::terms(term);

    if (terms.empty()) {
        return 0;
    }

    if (terms.size() != 1) {
        throw std::invalid_argument(
            "term must normalize to one token");
    }

    return impl_->index.term_frequency(
        terms.front(),
        chunk_id);
}

std::vector<SearchResult>
ProcessingCore::search(
    const std::string& query,
    int k) const
{
    return impl_->retrieval.search(
        query,
        k,
        impl_->chunks,
        impl_->index);
}

std::vector<ContextItem>
ProcessingCore::build_context(
    const std::string& query,
    int k,
    std::size_t token_budget) const
{
    return impl_->context_builder.build(
        search(query, k),
        token_budget);
}

}  // namespace aiws