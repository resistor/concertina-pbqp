#include "concertina.h"
#include "solver.h"

using llvm::PBQP::Solution;
using llvm::PBQP::RegAlloc::PBQPRAGraph;
using llvm::PBQP::RegAlloc::solve;

struct ConcertinaGraph {
  PBQPRAGraph graph;
  std::unordered_map<PBQPRAGraph::NodeId, std::vector<ConcertinaReed>>
      node_options;
};

ConcertinaReed lookupSolution(ConcertinaGraph &graph, PBQPRAGraph::NodeId nid,
                              unsigned val) {
  return graph.node_options[nid][val];
}

auto addNote(ConcertinaGraph &graph, ConcertinaNote note) {
  // Set all allowed note->reed mappings to have zero cost.
  auto reed_range = CGWheatstoneReedMapping.equal_range(note);
  std::vector<ConcertinaReed> node_options_vec;
  for (auto it = reed_range.first; it != reed_range.second; ++it) {
    node_options_vec.push_back(it->second);
  }

  PBQPRAGraph::RawVector Costs(node_options_vec.size(), 0);
  for (int i = 0; i < node_options_vec.size(); ++i) {
    ConcertinaReed reed = node_options_vec[i];

    // Pinky reeds get a cost penalty.
    unsigned col = GetColumn(reed);
    if (((unsigned)reed & HAND_MASK) == LEFT) {
      if (col == 0 || col == 1) {
        Costs[i] += 1;
      }
    } else if (((unsigned)reed & HAND_MASK) == RIGHT) {
      if (col == 3 || col == 4) {
        Costs[i] += 1;
      }
    }
  }

  auto nid = graph.graph.addNode(std::move(Costs));
  graph.node_options[nid] = std::move(node_options_vec);
  return nid;
}

void setupSimultaneousNoteCosts(llvm::PBQP::Matrix &Costs,
                                std::vector<ConcertinaReed> &n_options,
                                std::vector<ConcertinaReed> &m_options) {
  for (int n = 0; n < n_options.size(); ++n) {
    ConcertinaReed n_reed = n_options[n];
    for (int m = 0; m < m_options.size(); ++m) {
      ConcertinaReed m_reed = m_options[m];

      // Mismatched bellows directions are impossible, thus infinite cost.
      if (((unsigned)n_reed & DIRECTION_MASK) !=
          ((unsigned)m_reed & DIRECTION_MASK)) {
        Costs[n][m] = INFINITY;
      }

      // Apply a cost to multiple
      if (((unsigned)n_reed & HAND_MASK) == ((unsigned)m_reed & HAND_MASK) &&
          GetColumn(n_reed) == GetColumn(m_reed)) {
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
                              std::vector<ConcertinaReed> n_options,
                              std::vector<ConcertinaReed> m_options) {
  for (int n = 0; n < n_options.size(); ++n) {
    ConcertinaReed n_reed = n_options[n];
    for (int m = 0; m < m_options.size(); ++m) {
      ConcertinaReed m_reed = m_options[m];

      // Apply a cost to changing hands.
      if (((unsigned)n_reed & HAND_MASK) != ((unsigned)m_reed & HAND_MASK)) {
        Costs[n][m] += 1;
      }

      // Apply a cost to changing bellows directions and buttons simultaneously.
      if (((unsigned)n_reed & DIRECTION_MASK) !=
              ((unsigned)m_reed & DIRECTION_MASK) &&
          ((unsigned)n_reed & FINGER_MASK) !=
              ((unsigned)m_reed & FINGER_MASK)) {
        Costs[n][m] += 1;
      }

      // Intra-hand rules
      if (((unsigned)n_reed & HAND_MASK) == ((unsigned)m_reed & HAND_MASK)) {
        // Apply a cost to going directly from the upper to the lower row.
        if (GetRow(n_reed) == 2 && GetRow(m_reed) == 0 ||
            GetRow(n_reed) == 0 && GetRow(m_reed) == 2) {
          Costs[n][m] += 1;
        }

        // Apply a cost to sequential notes being assigned to reeds in the same
        // column of the 3x5 layout.
        if (GetColumn(n_reed) == GetColumn(m_reed)) {
          Costs[n][m] += 3;
        }

        // Apply a cost to sequential notes that use the pinky finger.
        // This is not subsumed by the intra-column cost because the pinky
        // covers two columns.
        if (((unsigned)n_reed & HAND_MASK) == LEFT) {
          if ((GetColumn(n_reed) == 0 && GetColumn(m_reed) == 1) ||
              (GetColumn(n_reed) == 1 && GetColumn(m_reed) == 0)) {
            Costs[n][m] += 3;
          }
        } else {
          if ((GetColumn(n_reed) == 3 && GetColumn(m_reed) == 4) ||
              (GetColumn(n_reed) == 4 && GetColumn(m_reed) == 3)) {
            Costs[n][m] += 3;
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
    ConcertinaReed n1reed =
        lookupSolution(g, node, solution.getSelection(node));
    printf("  Reed assigned: %s\n", GetReedName(n1reed));
  }

  printf("Sequential notes:\n");
  for (auto node : seq_nodes) {
    ConcertinaReed n1reed =
        lookupSolution(g, node, solution.getSelection(node));
    printf("  Reed assigned: %s\n", GetReedName(n1reed));
  }

  printf("Mixed notes:\n");
  printf("  Reed assigned: %s\n",
         GetReedName(lookupSolution(g, id2, solution.getSelection(id2))));
  printf("  Reed assigned: %s\n",
         GetReedName(lookupSolution(g, id9, solution.getSelection(id9))));
  printf("  Reed assigned: %s\n",
         GetReedName(lookupSolution(g, id4, solution.getSelection(id4))));

  return 0;
}