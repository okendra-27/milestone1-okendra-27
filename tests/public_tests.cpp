#include "aiws/processing_core.hpp"
#include <string>
#include <iostream>
#include <cassert>

namespace {
int failures = 0;
void check(bool c, const std::string& m) { if (!c) { ++failures; std::cerr << "FAIL: " << m << "\n"; } }
std::string mk(int n) { std::string s; for (int i=0;i<n;++i){ if(!s.empty()) s+=' '; s+="w"+std::to_string(i);} return s; }
}

/* ASSERT CASES */
static void assert_pre_main() {
    using namespace aiws;

    assert(ProcessingCore::normalize("A   B   C") == "a b c");
    assert(ProcessingCore::normalize("MIXED case INPUT") == "mixed case input");
    assert(ProcessingCore::normalize("...---") == "");
    assert(ProcessingCore::normalize("Hello\tWorld") == "hello world");
    assert(ProcessingCore::normalize("  x y  ") == "x y");

    Workspace ws;
    ws.add_document(Document{"d","D", mk(121)});
    ProcessingCore core;
    core.rebuild(ws);
    assert(core.chunk_count() == 2);
    assert(core.chunks()[0].token_count == 120);
    assert(core.chunks()[1].token_count == 21);

    ws = Workspace{};
    ws.add_document(Document{"d2","D2", mk(300)});
    core.rebuild(ws);
    assert(core.chunk_count() == 3);
    assert(core.chunks()[0].token_count == 120);
    assert(core.chunks()[1].token_count == 120);
    assert(core.chunks()[2].token_count == 60);

    Workspace ws2;
    ws2.add_document(Document{"a","A","alpha beta alpha"});
    ws2.add_document(Document{"b","B","beta gamma beta"});
    core.rebuild(ws2);
    assert(core.document_frequency("alpha") == 1);
    assert(core.document_frequency("beta") == 2);
    assert(core.term_frequency("alpha","a#0") == 2);
    assert(core.term_frequency("beta","b#0") == 2);

    bool threw = false;
    try { core.term_frequency("x","a#99"); } catch(const std::out_of_range&) { threw = true; }
    assert(threw);

    ws2.add_document(Document{"c","C","alpha"});
    core.rebuild(ws2);
    assert(core.document_frequency("alpha") == 2);

    Workspace ws3;
    ws3.add_document(Document{"d1","D1","alpha alpha beta"});
    ws3.add_document(Document{"d2","D2","alpha gamma"});
    ws3.add_document(Document{"d3","D3","beta beta beta"});
    core.rebuild(ws3);

    auto r = core.search("alpha beta", 10);
    assert(r.size() == 3);
    assert(r[0].document_id == "d1");

    auto r2 = core.search("beta", 2);
    assert(r2.size() == 2);
    assert(r2[0].document_id == "d3");

    bool threw2 = false;
    try { core.search("alpha", -1); } catch(const std::invalid_argument&) { threw2 = true; }
    assert(threw2);

    Workspace ws4;
    ws4.add_document(Document{"x","X","alpha beta gamma delta"});
    ws4.add_document(Document{"y","Y","alpha alpha alpha"});
    core.rebuild(ws4);

    auto ctx = core.build_context("alpha beta", 10, 2);
    assert(ctx.size() == 1);
    assert(ctx[0].token_count == 2);
    assert(ctx[0].truncated);

    auto ctx2 = core.build_context("alpha", 3, 3);
    assert(ctx2.size() == 1);
    assert(ctx2[0].token_count <= 3);

    Workspace ws5;
    ws5.add_document(Document{"a","A","alpha beta gamma"});
    ws5.add_document(Document{"b","B","beta gamma delta"});
    ws5.add_document(Document{"c","C","alpha delta epsilon"});
    core.rebuild(ws5);

    auto r_end = core.search("alpha gamma", 10);
    assert(r_end.size() == 3);
    assert(r_end[0].document_id == "a");

    auto ctx_end = core.build_context("alpha gamma", 15, 10);
    assert(!ctx_end.empty());
    assert(ctx_end[0].token_count <= 10);

    bool threw3 = false;
    try { core.search("", 5); } catch(const std::invalid_argument&) { threw3 = true; }
    assert(threw3);
}


int main() {
    assert_pre_main();

    using namespace aiws;

    check(ProcessingCore::normalize("A   B   C") == "a b c", "normalize collapse");
    check(ProcessingCore::normalize("MIXED case INPUT") == "mixed case input", "normalize case");
    check(ProcessingCore::normalize("...---") == "", "normalize punctuation only");
    check(ProcessingCore::normalize("Hello\tWorld") == "hello world", "normalize tabs");
    check(ProcessingCore::normalize("  x y  ") == "x y", "normalize trim");

    Workspace ws_chunk;
    ws_chunk.add_document(Document{"d1","D1", mk(121)});
    ProcessingCore core;
    core.rebuild(ws_chunk);
    check(core.chunk_count() == 2, "121 tokens -> 2 chunks");
    check(core.chunks()[0].token_count == 120, "chunk0 size");
    check(core.chunks()[1].token_count == 21, "chunk1 size");

    ws_chunk = Workspace{};
    ws_chunk.add_document(Document{"d2","D2", mk(300)});
    core.rebuild(ws_chunk);
    check(core.chunk_count() == 3, "300 tokens -> 3 chunks");
    check(core.chunks()[0].token_count == 120, "chunk0");
    check(core.chunks()[1].token_count == 120, "chunk1");
    check(core.chunks()[2].token_count == 60, "chunk2");

    Workspace ws_index;
    ws_index.add_document(Document{"a","A","alpha beta alpha"});
    ws_index.add_document(Document{"b","B","beta gamma beta"});
    core.rebuild(ws_index);
    check(core.document_frequency("alpha") == 1, "df alpha");
    check(core.document_frequency("beta") == 2, "df beta");
    check(core.term_frequency("alpha","a#0") == 2, "tf alpha");
    check(core.term_frequency("beta","b#0") == 2, "tf beta");

    bool threw_tf = false;
    try { core.term_frequency("x","a#99"); } catch(const std::out_of_range&) { threw_tf = true; }
    check(threw_tf, "invalid chunk id throws");

    ws_index.add_document(Document{"c","C","alpha"});
    core.rebuild(ws_index);
    check(core.document_frequency("alpha") == 2, "df after rebuild");

    Workspace ws_rank;
    ws_rank.add_document(Document{"d1","D1","alpha alpha beta"});
    ws_rank.add_document(Document{"d2","D2","alpha gamma"});
    ws_rank.add_document(Document{"d3","D3","beta beta beta"});
    core.rebuild(ws_rank);

    auto r = core.search("alpha beta", 10);
    check(r.size() == 3, "candidate union");
    check(r[0].document_id == "d1", "d1 highest score");
    check(r[1].document_id != r[0].document_id, "distinct ordering");

    auto r2 = core.search("beta", 2);
    check(r2.size() == 2, "top2 beta");
    check(r2[0].document_id == "d3", "d3 highest beta");

    bool threw_k = false;
    try { core.search("alpha", -1); } catch(const std::invalid_argument&) { threw_k = true; }
    check(threw_k, "negative k throws");

    Workspace ws_ctx;
    ws_ctx.add_document(Document{"x","X","alpha beta gamma delta"});
    ws_ctx.add_document(Document{"y","Y","alpha alpha alpha"});
    core.rebuild(ws_ctx);

    auto ctx = core.build_context("alpha beta", 10, 2);
    check(ctx.size() == 1, "one chunk");
    check(ctx[0].token_count == 2, "budget applied");
    check(ctx[0].truncated, "truncated");

    auto ctx2 = core.build_context("alpha", 3, 3);
    check(ctx2.size() == 1, "single chunk");
    check(ctx2[0].token_count <= 3, "budget respected");

    Workspace ws_end;
    ws_end.add_document(Document{"a","A","alpha beta gamma"});
    ws_end.add_document(Document{"b","B","beta gamma delta"});
    ws_end.add_document(Document{"c","C","alpha delta epsilon"});
    core.rebuild(ws_end);

    auto r_end = core.search("alpha gamma", 10);
    check(r_end.size() == 3, "all docs candidate");
    check(r_end[0].document_id == "a", "best match");

    auto ctx_end = core.build_context("alpha gamma", 15, 10);
    check(!ctx_end.empty(), "context produced");
    check(ctx_end[0].token_count <= 10, "budget respected");

    bool threw_empty = false;
    try { core.search("", 5); } catch(const std::invalid_argument&) { threw_empty = true; }
    check(threw_empty, "empty query throws");

    if (failures == 0) {
        std::cout << "All M1 tests passed\n";
        return 0;
    }
    std::cerr << failures << " test(s) failed\n";
    return 1;
}
