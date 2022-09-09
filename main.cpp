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
    for (auto direction : DIRECTIONS) {
        for (unsigned i = 1; i <= 15; ++i) {
            for (unsigned j = 1; j <= 15; ++j) {
                Costs[LEFT | direction | i][LEFT | direction | j] = 0;
                Costs[LEFT | direction | i][RIGHT | direction | j] = 0;
                Costs[RIGHT | direction | i][LEFT | direction | j] = 0;
                Costs[RIGHT | direction | i][RIGHT | direction | j] = 0;
            }
        }
    }
    return graph.addEdge(n1id, n2id, std::move(Costs));
}

auto addSequentialNoteEdge(PBQPRAGraph& graph, PBQPRAGraph::NodeId n1id, PBQPRAGraph::NodeId n2id) {
    llvm::PBQP::Matrix Costs((unsigned)ConcertinaReed::MaxReed, (unsigned)ConcertinaReed::MaxReed, 0);

    for (unsigned i = 1; i <= 15; ++i) {
        for (unsigned j = 1; j <= 15; ++j) {
            // Apply a cost to changing hands.
            Costs[LEFT | PUSH | i][RIGHT | PUSH | j] += 1;
            Costs[RIGHT | PUSH | i][LEFT | PUSH | j] += 1;

            Costs[LEFT | PULL | i][RIGHT | PULL | j] += 1;
            Costs[RIGHT | PULL | i][LEFT | PULL | j] += 1;

            Costs[LEFT | PULL | i][RIGHT | PUSH | j] += 1;
            Costs[RIGHT | PUSH | i][LEFT | PULL | j] += 1;

            Costs[LEFT | PUSH | i][RIGHT | PULL | j] += 1;
            Costs[RIGHT | PULL | i][LEFT | PUSH | j] += 1;
        }
    }

    // Apply a cost to changing bellows directions and buttons simultaneously.
    for (unsigned i = 1; i <= 15; ++i) {
        for (unsigned j = 1; j <= 15; ++j) {
            if (i != j) {
                Costs[LEFT | PUSH | i][LEFT | PULL | j] += 1;
                Costs[LEFT | PULL | i][LEFT | PUSH | j] += 1;

                Costs[RIGHT | PUSH | i][RIGHT | PULL | j] += 1;
                Costs[RIGHT | PULL | i][RIGHT | PUSH | j] += 1;
            }

            Costs[LEFT | PUSH | i][RIGHT | PULL | j] += 1;
            Costs[RIGHT | PULL | i][LEFT | PUSH | j] += 1;

            Costs[RIGHT | PUSH | i][LEFT | PULL | j] += 1;
            Costs[LEFT | PULL | i][RIGHT | PUSH | j] += 1;
        }
    }

    for (auto hand : HANDS) {
        for (auto direction : DIRECTIONS) {
            for (unsigned i = 1; i < 6; ++i) {
                // Apply a cost to sequential notes being assigned to reeds in the same column of
                // the 3x5 layout.
                Costs[hand | direction | i][hand | direction | i+5] += 3;
                Costs[hand | direction | i+5][hand | direction | i] += 3;
                Costs[hand | direction | i][hand | direction | i+10] += 3;
                Costs[hand | direction | i+10][hand | direction | i] += 3;
                Costs[hand | direction | i+5][hand | direction | i+10] += 3;
                Costs[hand | direction | i+10][hand | direction | i+5] += 3;
            }
        }
    }

    constexpr std::array<unsigned, 3> col4_reeds = { 4, 9, 14 };
    constexpr std::array<unsigned, 3> col5_reeds = { 5, 10, 15 };
    for (auto hand : HANDS) {
        for (auto direction : DIRECTIONS) {
            for (auto col4 : col4_reeds) {
                for (auto col5 : col5_reeds) {
                    // Apply a cost to sequential notes that use the pinky finger.
                    // This is not subsumed by the intra-column cost because the pinky
                    // covers two columns.
                    Costs[hand | direction | col4][hand | direction | col5] += 3;
                    Costs[hand | direction | col5][hand | direction | col4] += 3;
                }
            }
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

    std::vector<PBQPRAGraph::NodeId> seq_nodes = {
        addNote(g, ConcertinaNote::G4),
        addNote(g, ConcertinaNote::C5),
        addNote(g, ConcertinaNote::D5),
        addNote(g, ConcertinaNote::C5),
        addNote(g, ConcertinaNote::B4),
        addNote(g, ConcertinaNote::E4),
        addNote(g, ConcertinaNote::B4),
        addNote(g, ConcertinaNote::C5),
        addNote(g, ConcertinaNote::B4),
        addNote(g, ConcertinaNote::A4),
        addNote(g, ConcertinaNote::G4),
        addNote(g, ConcertinaNote::G4),
        addNote(g, ConcertinaNote::E4),
        addNote(g, ConcertinaNote::G4),
        addNote(g, ConcertinaNote::A4),
        addNote(g, ConcertinaNote::A4),
    };

    // Add edges to the PBQP graph that represent the fact that all of the notes
    // must be playable simultaneously.
    for (int i = 0; i < nodes.size(); ++i) {
        for (int j = i + 1; j < nodes.size(); ++j) {
            addSimultaneousNoteEdge(g, nodes[i], nodes[j]);
        }
    }

    // Add edges to the PBQP graph that represent the fact that these notes
    // must be played sequentially.
    for (int i = 0; i < seq_nodes.size()-1; ++i) {
        addSequentialNoteEdge(g, seq_nodes[i], seq_nodes[i+1]);
    }

    Solution solution = solve(g);

    printf("Simultaneous notes:\n");
    for (auto node : nodes) {
        ConcertinaReed n1reed = (ConcertinaReed)solution.getSelection(node);
        printf("  Reed assigned: %s\n", GetReedName(n1reed));
    }

    printf("Sequential notes:\n");
    for (auto node : seq_nodes) {
        ConcertinaReed n1reed = (ConcertinaReed)solution.getSelection(node);
        printf("  Reed assigned: %s\n", GetReedName(n1reed));
    }

    return 0;
}