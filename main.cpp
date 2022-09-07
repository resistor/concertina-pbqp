#include "solver.h"
#include "concertina.h"

using llvm::PBQP::RegAlloc::PBQPRAGraph;
using llvm::PBQP::Solution;
using llvm::PBQP::RegAlloc::solve;

auto addNote(PBQPRAGraph& graph, ConcertinaNote note) {
    // The available note->reed mappings are sparse, so we default of INFINITY.
    PBQPRAGraph::RawVector Costs((unsigned int)ConcertinaReed::MaxReed, INFINITY);

    // Set all allowed note->reed mappings to have zero cost.
    auto reed_range = ReedMapping.equal_range(note);
    for (auto it = reed_range.first; it != reed_range.second; ++it) {
        Costs[(unsigned)it->second] = 0;
    }

    // Left-hand pinky reeds get a penalty.
    Costs[(unsigned)ConcertinaReed::L01Pull] += 1;
    Costs[(unsigned)ConcertinaReed::L01Push] += 1;
    Costs[(unsigned)ConcertinaReed::L02Pull] += 1;
    Costs[(unsigned)ConcertinaReed::L02Push] += 1;
    Costs[(unsigned)ConcertinaReed::L06Pull] += 1;
    Costs[(unsigned)ConcertinaReed::L06Push] += 1;
    Costs[(unsigned)ConcertinaReed::L07Pull] += 1;
    Costs[(unsigned)ConcertinaReed::L07Push] += 1;
    Costs[(unsigned)ConcertinaReed::L11Pull] += 1;
    Costs[(unsigned)ConcertinaReed::L11Push] += 1;
    Costs[(unsigned)ConcertinaReed::L12Pull] += 1;
    Costs[(unsigned)ConcertinaReed::L12Push] += 1;

    // Right-hand pinky reeds get a penalty.
    Costs[(unsigned)ConcertinaReed::R04Pull] += 1;
    Costs[(unsigned)ConcertinaReed::R04Push] += 1;
    Costs[(unsigned)ConcertinaReed::R05Pull] += 1;
    Costs[(unsigned)ConcertinaReed::R05Push] += 1;
    Costs[(unsigned)ConcertinaReed::R09Pull] += 1;
    Costs[(unsigned)ConcertinaReed::R09Push] += 1;
    Costs[(unsigned)ConcertinaReed::R10Pull] += 1;
    Costs[(unsigned)ConcertinaReed::R10Push] += 1;
    Costs[(unsigned)ConcertinaReed::R14Pull] += 1;
    Costs[(unsigned)ConcertinaReed::R14Push] += 1;
    Costs[(unsigned)ConcertinaReed::R15Pull] += 1;
    Costs[(unsigned)ConcertinaReed::R15Push] += 1;

    return graph.addNode(std::move(Costs));
}

auto addSimultaneousNoteEdge(PBQPRAGraph& graph, PBQPRAGraph::NodeId n1id, PBQPRAGraph::NodeId n2id) {
    llvm::PBQP::Matrix Costs((unsigned)ConcertinaReed::MaxReed, (unsigned)ConcertinaReed::MaxReed, INFINITY);
    for (unsigned i = (unsigned)ConcertinaReed::L01Pull; i <= (unsigned)ConcertinaReed::R15Pull; ++i) {
        for (unsigned j = (unsigned)ConcertinaReed::L01Pull; j <= (unsigned)ConcertinaReed::R15Pull; ++j) {
            Costs[i][j] = 0;
        }
    }
    for (unsigned i = (unsigned)ConcertinaReed::L01Push; i <= (unsigned)ConcertinaReed::R15Push; ++i) {
        for (unsigned j = (unsigned)ConcertinaReed::L01Push; j <= (unsigned)ConcertinaReed::R15Push; ++j) {
            Costs[i][j] = 0;
        }
    }

    return graph.addEdge(n1id, n2id, std::move(Costs));
}

int main() {
    PBQPRAGraph g({});

    // Construct the nodes of the PBQP graph, representing the individual notes.
    std::vector<PBQPRAGraph::NodeId> nodes = {
        addNote(g, ConcertinaNote::G3),
        addNote(g, ConcertinaNote::B3),
        addNote(g, ConcertinaNote::D4),
        addNote(g, ConcertinaNote::F4),
    };

    // Add edges to the PBQP graph that represent the fact that all of the notes
    // must be playable simultaneously.
    for (int i = 0; i < nodes.size(); ++i) {
        for (int j = i + 1; j < nodes.size(); ++j) {
            addSimultaneousNoteEdge(g, nodes[i], nodes[j]);
        }
    }

    Solution solution = solve(g);

    for (auto node : nodes) {
        ConcertinaReed n1reed = (ConcertinaReed)solution.getSelection(node);
        printf("Reed assigned: %s\n", GetReedName(n1reed));
    }

    return 0;
}