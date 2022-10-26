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
      int col = GetColumn(it->second);
      int finger_col = GetFingerColumn((ConcertinaReed)finger);
      if (std::abs(finger_col - col) < 2) {
        // Fingers are allowed to travel at most one column from their
        // home column.
        node_options_vec.push_back((unsigned)it->second | finger);
      }
    }
  }

  PBQPRAGraph::RawVector Costs(node_options_vec.size(), 0);
  for (int i = 0; i < node_options_vec.size(); ++i) {
    unsigned reed = node_options_vec[i];
    unsigned col = GetColumn((ConcertinaReed)reed);

    // Apply a cost the non-home reeds.
    if (col > 1) {
      Costs[i] += col;
    }

    // Apply a cost to playing buttons with fingers other than the "home"
    // finger.
    unsigned row = GetRow((ConcertinaReed)reed);
    unsigned finger_col = GetFingerColumn((ConcertinaReed)reed);
    if (row == 0 && col == 3) {
      // The L02a and R04a buttons are more easily reached by the ring
      // finger, despite being in the pinky column.
      if (finger_col == 3) {
        Costs[i] += 1;
      }
    } else {
      if (col != finger_col) {
        Costs[i] += 2;
      }
    }
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

      if ((n_reed & HAND_MASK) == (m_reed & HAND_MASK)) {
        // Apply a cost to multiple buttons in the same column.
        if (GetColumn((ConcertinaReed)n_reed) ==
            GetColumn((ConcertinaReed)m_reed)) {
          Costs[n][m] += 3;
        }

        // Apply a cost to playing upper and lower row simultaneously.
        auto n_row = GetRow((ConcertinaReed)n_reed);
        auto m_row = GetRow((ConcertinaReed)m_reed);
        if ((n_row == 0 && m_row == 2) || (n_row == 2 && m_row == 0)) {
          Costs[n][m] += 1;
        }
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

      // Apply a cost to anything *other* than simple bellows reversal
      // or a repeated note.
      if ((n_reed & ~DIRECTION_MASK) != (m_reed & ~DIRECTION_MASK)) {
        Costs[n][m] += 1;
      }

      // Apply a cost to changing hands.
      if ((n_reed & HAND_MASK) != (m_reed & HAND_MASK)) {
        Costs[n][m] += 1;
      }

      // Intra-hand rules
      if ((n_reed & HAND_MASK) == (m_reed & HAND_MASK)) {
        // Apply a cost to going directly from the upper to the lower row.
        auto n_row = GetRow((ConcertinaReed)n_reed);
        auto m_row = GetRow((ConcertinaReed)m_reed);
        if ((n_row == 0 && m_row == 2) || (n_row == 2 && m_row == 0)) {
          Costs[n][m] += 1;
        }

        if ((n_reed & BUTTON_MASK) != (m_reed & BUTTON_MASK)) {
          // Apply a cost to sequential notes being assigned to reeds in the
          // same column.
          if (GetColumn((ConcertinaReed)n_reed) ==
              GetColumn((ConcertinaReed)m_reed)) {
            Costs[n][m] += 4;
          }

          // Apply a cost to sequential notes being assigned to the same finger.
          if (GetFingerColumn((ConcertinaReed)n_reed) ==
              GetFingerColumn((ConcertinaReed)m_reed)) {
            Costs[n][m] += 2;
          }
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
  /*
    // Construct the nodes of the PBQP graph, representing the individual notes.
    std::vector<PBQPRAGraph::NodeId> nodes = {
        addNote(g, ConcertinaNote::G3),
        addNote(g, ConcertinaNote::B3),
        addNote(g, ConcertinaNote::D4),
        addNote(g, ConcertinaNote::F4),
    };
    */

  std::vector<PBQPRAGraph::NodeId> seq_nodes = {
      // loop 0
      addNote(g, ConcertinaNote::A4),  // 0

      // 1
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::C5),

      // 8
      addNote(g, ConcertinaNote::Bflat4),
      addNote(g, ConcertinaNote::G4),
      addNote(g, ConcertinaNote::Bflat4),

      // 11
      addNote(g, ConcertinaNote::A4),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::G5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::D5),

      // 18
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::G5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::C5),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::C5),

      // 26
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::C5),

      // 33
      addNote(g, ConcertinaNote::Bflat4),
      addNote(g, ConcertinaNote::G4),
      addNote(g, ConcertinaNote::Bflat4),

      // 36
      addNote(g, ConcertinaNote::A4),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::G5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::E5),

      // 43
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::D5),

      // loop 1 - 46
      addNote(g, ConcertinaNote::E5),

      // 47
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::G5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::E5),

      // 53
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::E5),

      // 59
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::C5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::A5),

      // 65
      addNote(g, ConcertinaNote::Bflat4),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::A4),
      addNote(g, ConcertinaNote::B4),
      addNote(g, ConcertinaNote::Csharp5),

      // 71
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::Csharp5),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::C5),

      // 79
      addNote(g, ConcertinaNote::Bflat4),
      addNote(g, ConcertinaNote::G4),
      addNote(g, ConcertinaNote::Bflat4),

      // 82
      addNote(g, ConcertinaNote::A4),
      addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::G5),
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::E5),

      // 88
      addNote(g, ConcertinaNote::F5),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::D5),
  };

  std::vector<PBQPRAGraph::NodeId> seq_nodes2 = {
      // loop 0
      addNote(g, ConcertinaNote::A3),  // 0

      // 1
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::E4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::E4),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::C4),

      // 8
      addNote(g, ConcertinaNote::Bflat3),
      addNote(g, ConcertinaNote::G3),
      addNote(g, ConcertinaNote::Bflat3),

      // 11
      addNote(g, ConcertinaNote::A3),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::A4),
      addNote(g, ConcertinaNote::G4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::D4),

      // 18
      addNote(g, ConcertinaNote::E4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::G4),
      addNote(g, ConcertinaNote::E4),
      addNote(g, ConcertinaNote::C4),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::E4),
      addNote(g, ConcertinaNote::C4),

      // 26
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::E4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::E4),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::C4),

      // 33
      addNote(g, ConcertinaNote::Bflat3),
      addNote(g, ConcertinaNote::G3),
      addNote(g, ConcertinaNote::Bflat3),

      // 36
      addNote(g, ConcertinaNote::A3),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::A4),
      addNote(g, ConcertinaNote::G4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::E4),

      // 43
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::D4),

      // loop 1 - 46
      addNote(g, ConcertinaNote::E4),

      // 47
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::E4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::G4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::E4),

      // 53
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::E4),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::E4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::E4),

      // 59
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::A4),
      addNote(g, ConcertinaNote::C4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::A4),

      // 65
      addNote(g, ConcertinaNote::Bflat3),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::A3),
      addNote(g, ConcertinaNote::B3),
      addNote(g, ConcertinaNote::Csharp4),

      // 71
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::Csharp4),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::E4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::E4),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::C4),

      // 79
      addNote(g, ConcertinaNote::Bflat3),
      addNote(g, ConcertinaNote::G3),
      addNote(g, ConcertinaNote::Bflat3),

      // 82
      addNote(g, ConcertinaNote::A3),
      addNote(g, ConcertinaNote::A4),
      addNote(g, ConcertinaNote::A4),
      addNote(g, ConcertinaNote::G4),
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::E4),

      // 88
      addNote(g, ConcertinaNote::F4),
      addNote(g, ConcertinaNote::D4),
      addNote(g, ConcertinaNote::D4),
  };

  /*
  // Add edges to the PBQP graph that represent the fact that all of the notes
  // must be playable simultaneously.
  for (int i = 0; i < nodes.size(); ++i) {
    for (int j = i + 1; j < nodes.size(); ++j) {
      addSimultaneousNoteEdge(g, nodes[i], nodes[j]);
    }
  }*/

  // Add edges to the PBQP graph that represent the fact that these notes
  // must be played sequentially.
  for (int i = 0; i < seq_nodes.size() - 1; ++i) {
    addSimultaneousNoteEdge(g, seq_nodes[i], seq_nodes2[i]);
    addSequentialNoteEdge(g, seq_nodes[i], seq_nodes[i + 1]);
    addSequentialNoteEdge(g, seq_nodes[i], seq_nodes2[i + 1]);
    addSequentialNoteEdge(g, seq_nodes2[i], seq_nodes2[i + 1]);
    addSequentialNoteEdge(g, seq_nodes2[i], seq_nodes[i + 1]);
  }
  addSimultaneousNoteEdge(g, seq_nodes[90], seq_nodes2[90]);

  addSequentialNoteEdge(g, seq_nodes[45], seq_nodes[0]);
  addSequentialNoteEdge(g, seq_nodes[90], seq_nodes[46]);
  addSequentialNoteEdge(g, seq_nodes2[45], seq_nodes2[0]);
  addSequentialNoteEdge(g, seq_nodes2[90], seq_nodes2[46]);
  addSequentialNoteEdge(g, seq_nodes[45], seq_nodes2[0]);
  addSequentialNoteEdge(g, seq_nodes[90], seq_nodes2[46]);
  addSequentialNoteEdge(g, seq_nodes2[45], seq_nodes[0]);
  addSequentialNoteEdge(g, seq_nodes2[90], seq_nodes[46]);
  // addSequentialNoteEdge(g, seq_nodes[49], seq_nodes[26]);
  // addSequentialNoteEdge(g, seq_nodes[75], seq_nodes[50]);

  /*
    PBQPRAGraph::NodeId id2 = addNote(g, ConcertinaNote::E5);
    PBQPRAGraph::NodeId id9 = addNote(g, ConcertinaNote::B4);
    PBQPRAGraph::NodeId id4 = addNote(g, ConcertinaNote::E4);
    addSequentialAndSimultaneousNoteEdge(g, id2, id9);
    addSequentialNoteEdge(g, id9, id4);
    addSimultaneousNoteEdge(g, id2, id4);
  */

  /*
    PBQPRAGraph::NodeId n1 = addNote(g, ConcertinaNote::C4);
    addSimultaneousNoteEdge(g, n1, seq_nodes[1]);
    PBQPRAGraph::NodeId n2 = addNote(g, ConcertinaNote::E4);
    PBQPRAGraph::NodeId n3 = addNote(g, ConcertinaNote::G4);
    addSimultaneousNoteEdge(g, n2, seq_nodes[2]);
    addSimultaneousNoteEdge(g, n3, seq_nodes[2]);
    addSimultaneousNoteEdge(g, n2, seq_nodes[3]);
    addSimultaneousNoteEdge(g, n3, seq_nodes[3]);
    addSimultaneousNoteEdge(g, n2, n3);
    addSequentialNoteEdge(g, n1, n2);
    addSequentialNoteEdge(g, n1, n3);*/

  Solution solution = solve(g.graph);
  /*
    printf("Simultaneous notes:\n");
    for (auto node : nodes) {
      unsigned n1reed = lookupSolution(g, node, solution.getSelection(node));
      printf("  Reed assigned: %s\n", GetReedAndFinger(n1reed).c_str());
    }*/

  printf("High Oct notes:\n");
  for (int i = 0; i < seq_nodes.size(); ++i) {
    auto node1 = seq_nodes[i];
    auto node2 = seq_nodes2[i];
    unsigned n1reed = lookupSolution(g, node1, solution.getSelection(node1));
    unsigned n2reed = lookupSolution(g, node2, solution.getSelection(node2));

    printf("  Reed assigned: ( %s , %s )\n", GetReedAndFinger(n1reed).c_str(),
           GetReedAndFinger(n2reed).c_str());
  }
  /*
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
  */

  /*
    printf("Cord notes:\n");
    printf("  Reed assigned: %s\n",
           GetReedAndFinger(lookupSolution(g, n1, solution.getSelection(n1)))
               .c_str());
    printf("  Reed assigned: %s\n",
           GetReedAndFinger(lookupSolution(g, n2, solution.getSelection(n2)))
               .c_str());
    printf("  Reed assigned: %s\n",
           GetReedAndFinger(lookupSolution(g, n3, solution.getSelection(n3)))
               .c_str());
               */
  return 0;
}