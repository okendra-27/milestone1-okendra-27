# M1 Design

## 1. System structure

TextProcessor normalizes document and query text and finds paragraph boundaries.

DocumentChunker turns one `Document` into ordered `Chunk` objects with source information, normalized text, and token counts.

CorpusIndex owns the current searchable corpus. It stores the chunks and term frequencies for each chunk. 

Retriever uses the index to score and rank matching chunks. 

ContextBuilder takes ranked results and creates context items within a token budget. 

ProcessingCore coordinates these components.

The main flow is:

Document -> DocumentChunker -> CorpusIndex -> Retriever -> ContextBuilder

## 2. Design decisions

`Workspace` owns its documents. 

`CorpusIndex` owns its generated chunks and frequency maps. A chunk stores its document ID, title, sequence number, normalized text, token count, and original source span.

The index uses a vector to keep chunks in source order. It uses maps from terms to chunk IDs and frequencies so searches do not need to rescan every document every single time which can increase time/space complexity essentially.

## 3. Correctness and consistency

A rebuild creates new chunks, document data, and frequency data in local variables first. The old corpus is only replaced after all documents are processed successfully. This keeps the previous valid corpus available if duplicate document IDs cause an exception.

Each rebuild replaces all old index data, so removed documents and old terms do not stay. Duplicate document IDs are rejected because chunk IDs must remain unique.

## 4. Testing strategy

The tests use `assert()`.

Text-processing tests check lowercase conversion, separators, digits, empty input, and paragraph boundaries. 

Chunking tests check the 120-token maximum, 20-token overlap, paragraph-preferred boundaries, IDs, and source positions.

Index tests check document frequency, term frequency, invalid multi-word terms, missing chunk IDs, replacement rebuilds, and duplicate-ID rebuild failures. 

Retrieval tests check empty queries, unknown terms.

Context tests check zero budgets, partial chunk truncation, and token limits.

## 5. Alternatives considered
One option was to place all M1 behavior in `ProcessingCore`. This was not selected because chunking, indexing, ranking, and context construction would become difficult to test independently.

Another option was to rescan all documents during every search. This was not selected because an index is simpler for repeated searches and directly supports document-frequency and term-frequency calculations.

I selected separate small components with simple vectors and maps. This keeps the code readable, allows direct tests for each responsibility, and avoids advanced designs that are unnecessary for M1.

