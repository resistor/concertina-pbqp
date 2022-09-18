#include "concertina.h"
#include "solver.h"

using llvm::PBQP::Solution;
using llvm::PBQP::RegAlloc::PBQPRAGraph;
using llvm::PBQP::RegAlloc::solve;

struct ConcertinaGraph {
  PBQPRAGraph graph;
  std::unordered_map<PBQPRAGraph::NodeId, std::vector<unsigned>> node_options;
};

unsigned lookupSolution(ConcertinaGraph &graph, PBQPRAGraph::NodeId nid,
                        unsigned val) {
  return graph.node_options[nid][val];
}

auto addNote(ConcertinaGraph &graph, ConcertinaNote note) {
  // Set all allowed note->reed mappings to have zero cost.
  auto reed_range = CGWheatstoneReedMapping.equal_range(note);
  std::vector<unsigned> node_options_vec;
  for (auto it = reed_range.first; it != reed_range.second; ++it) {
    for (auto finger : FINGERS) {
      node_options_vec.push_back((unsigned)it->second | finger);
    }
  }

  PBQPRAGraph::RawVector Costs(node_options_vec.size(), 0);
  for (int i = 0; i < node_options_vec.size(); ++i) {
    unsigned reed = node_options_vec[i];
    unsigned col = GetColumn((ConcertinaReed)reed);

    // Apply a cost the pinky reeds.
    if (col == 3 || col == 4) {
      Costs[i] += 1;
    }

    // Apply a cost to playing buttons with fingers other than the "home"
    // finger.
    unsigned finger_col = GetFingerColumn((ConcertinaReed)reed);
    Costs[i] += std::abs((int)col - (int)finger_col);
  }

  auto nid = graph.graph.addNode(std::move(Costs));
  graph.node_options[nid] = std::move(node_options_vec);
  return nid;
}

void setupSimultaneousNoteCosts(llvm::PBQP::Matrix &Costs,
                                std::vector<unsigned> &n_options,
                                std::vector<unsigned> &m_options) {
  for (int n = 0; n < n_options.size(); ++n) {
    unsigned n_reed = n_options[n];
    for (int m = 0; m < m_options.size(); ++m) {
      unsigned m_reed = m_options[m];

      // Mismatched bellows directions are impossible, thus infinite cost.
      if ((n_reed & DIRECTION_MASK) != (m_reed & DIRECTION_MASK)) {
        Costs[n][m] = INFINITY;
      }

      // Using the same finger more than once is impossible.
      if ((n_reed & FINGER_MASK) == (m_reed & FINGER_MASK)) {
        Costs[n][m] = INFINITY;
      }

      // Apply a cost to multiple buttons in the same column.
      if ((n_reed & HAND_MASK) == (m_reed & HAND_MASK) &&
          GetColumn((ConcertinaReed)n_reed) ==
              GetColumn((ConcertinaReed)m_reed)) {
        Costs[n][m] += 3;
      }
    }
  }
}

auto addSimultaneousNoteEdge(ConcertinaGraph &graph, PBQPRAGraph::NodeId n1id,
                             PBQPRAGraph::NodeId n2id) {
  auto &n_options = graph.node_options[n1id];
  auto &m_options = graph.node_options[n2id];

  llvm::PBQP::Matrix Costs(n_options.size(), m_options.size(), 0);
  setupSimultaneousNoteCosts(Costs, n_options, m_options);
  return graph.graph.addEdge(n1id, n2id, std::move(Costs));
}

void setupSequentialNoteCosts(llvm::PBQP::Matrix &Costs,
                              std::vector<unsigned> n_options,
                              std::vector<unsigned> m_options) {
  for (int n = 0; n < n_options.size(); ++n) {
    unsigned n_reed = n_options[n];
    for (int m = 0; m < m_options.size(); ++m) {
      unsigned m_reed = m_options[m];

      // Apply a cost to changing hands.
      if ((n_reed & HAND_MASK) != (m_reed & HAND_MASK)) {
        Costs[n][m] += 1;
      }

      // Apply a cost to changing bellows directions and buttons simultaneously.
      if ((n_reed & DIRECTION_MASK) != (m_reed & DIRECTION_MASK) &&
          (n_reed & BUTTON_MASK) != (m_reed & BUTTON_MASK)) {
        Costs[n][m] += 1;
      }

      // Intra-hand rules
      if ((n_reed & HAND_MASK) == (m_reed & HAND_MASK)) {
        // Apply a cost to going directly from the upper to the lower row.
        if (GetRow((ConcertinaReed)n_reed) == 2 &&
                GetRow((ConcertinaReed)m_reed) == 0 ||
            GetRow((ConcertinaReed)n_reed) == 0 &&
                GetRow((ConcertinaReed)m_reed) == 2) {
          Costs[n][m] += 1;
        }

        // Apply a cost to sequential notes being assigned to reeds in the same
        // column of the 3x5 layout.
        if (GetColumn((ConcertinaReed)n_reed) ==
            GetColumn((ConcertinaReed)m_reed)) {
          Costs[n][m] += 3;
        }

        // Apply a cost to sequential notes that use the pinky finger.
        // This is not subsumed by the intra-column cost because the pinky
        // covers two columns.
        if ((GetColumn((ConcertinaReed)n_reed) == 3 &&
             GetColumn((ConcertinaReed)m_reed) == 4) ||
            (GetColumn((ConcertinaReed)n_reed) == 4 &&
             GetColumn((ConcertinaReed)m_reed) == 3)) {
          Costs[n][m] += 3;
        }
      }
    }
  }
}

auto addSequentialNoteEdge(ConcertinaGraph &graph, PBQPRAGraph::NodeId n1id,
                           PBQPRAGraph::NodeId n2id) {
  auto &n_options = graph.node_options[n1id];
  auto &m_options = graph.node_options[n2id];

  llvm::PBQP::Matrix Costs(n_options.size(), m_options.size(), 0);
  setupSequentialNoteCosts(Costs, n_options, m_options);
  return graph.graph.addEdge(n1id, n2id, std::move(Costs));
}

auto addSequentialAndSimultaneousNoteEdge(ConcertinaGraph &graph,
                                          PBQPRAGraph::NodeId n1id,
                                          PBQPRAGraph::NodeId n2id) {
  auto &n_options = graph.node_options[n1id];
  auto &m_options = graph.node_options[n2id];

  llvm::PBQP::Matrix Costs(n_options.size(), m_options.size(), 0);
  setupSequentialNoteCosts(Costs, n_options, m_options);
  setupSimultaneousNoteCosts(Costs, n_options, m_options);
  return graph.graph.addEdge(n1id, n2id, std::move(Costs));
}

int main() {
  ConcertinaGraph g{{{}}, {}};

  // Construct the nodes of the PBQP graph, representing the individual notes.
  std::vector<PBQPRAGraph::NodeId> nodes = {
      addNote(g, ConcertinaNote::G3),
      addNote(g, ConcertinaNote::B3),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::F4),
  };

  std::vector<PBQPRAGraph::NodeId> seq_nodes = {
      addNote(g, ConcertinaNote::C5), addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::E5), addNote(g, ConcertinaNote::G5),
      addNote(g, ConcertinaNote::G5), addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::G5), addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::C5), addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::E5), addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::D5), addNote(g, ConcertinaNote::C5),
      addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::C5), addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::E5), addNote(g, ConcertinaNote::G5),
      addNote(g, ConcertinaNote::G5), addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::G5), addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::C5), addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::E5), addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::D5), addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::C5),

      addNote(g, ConcertinaNote::D5), addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::F5), addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::A5), addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::A5), addNote(g, ConcertinaNote::G5),
      addNote(g, ConcertinaNote::G5), addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::C5), addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::C5), addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::E5), addNote(g, ConcertinaNote::G5),
      addNote(g, ConcertinaNote::G5), addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::G5), addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::C5), addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::E5), addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::D5), addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::C5),
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
  for (int i = 0; i < seq_nodes.size() - 1; ++i) {
    addSequentialNoteEdge(g, seq_nodes[i], seq_nodes[i + 1]);
  }

  PBQPRAGraph::NodeId id2 = addNote(g, ConcertinaNote::E5);
  PBQPRAGraph::NodeId id9 = addNote(g, ConcertinaNote::B4);
  PBQPRAGraph::NodeId id4 = addNote(g, ConcertinaNote::E4);
  addSequentialAndSimultaneousNoteEdge(g, id2, id9);
  addSequentialNoteEdge(g, id9, id4);
  addSimultaneousNoteEdge(g, id2, id4);

  Solution solution = solve(g.graph);

  printf("Simultaneous notes:\n");
  for (auto node : nodes) {
    unsigned n1reed = lookupSolution(g, node, solution.getSelection(node));
    printf("  Reed assigned: %s\n", GetReedAndFinger(n1reed).c_str());
  }

  printf("Sequential notes:\n");
  for (auto node : seq_nodes) {
    unsigned n1reed = lookupSolution(g, node, solution.getSelection(node));
    printf("  Reed assigned: %s\n", GetReedAndFinger(n1reed).c_str());
  }

  printf("Mixed notes:\n");
  printf("  Reed assigned: %s\n",
         GetReedAndFinger(lookupSolution(g, id2, solution.getSelection(id2)))
             .c_str());
  printf("  Reed assigned: %s\n",
         GetReedAndFinger(lookupSolution(g, id9, solution.getSelection(id9)))
             .c_str());
  printf("  Reed assigned: %s\n",
         GetReedAndFinger(lookupSolution(g, id4, solution.getSelection(id4)))
             .c_str());

  return 0;
}