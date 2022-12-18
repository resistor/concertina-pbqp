#include "concertina.h"
#include "solver.h"
#include "MidiFile.h"
#include <unordered_set>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>


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
      if (n_reed == m_reed) {
        Costs[n][m] = -INFINITY;
        return;
      }


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

void test_midi();

int main() {
  test_midi();

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
      addNote(g, ConcertinaNote::B5),

      addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::G5),

      addNote(g, ConcertinaNote::E5),

      addNote(g, ConcertinaNote::A5),

      addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::G5),
      addNote(g, ConcertinaNote::G5),

      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::G5),

      addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::C6),
      addNote(g, ConcertinaNote::C6),
      addNote(g, ConcertinaNote::A5),

      addNote(g, ConcertinaNote::Fsharp5),
      addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::C5),
      addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::B4),
      addNote(g, ConcertinaNote::A4),
      addNote(g, ConcertinaNote::G4),

      addNote(g, ConcertinaNote::G4),
  };

  std::vector<PBQPRAGraph::NodeId> seq_nodes2 = {
      addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::G5),

      addNote(g, ConcertinaNote::C5),

      addNote(g, ConcertinaNote::C5),

      addNote(g, ConcertinaNote::C5),
      addNote(g, ConcertinaNote::G5),
      addNote(g, ConcertinaNote::G5),

      addNote(g, ConcertinaNote::B4),
      addNote(g, ConcertinaNote::B4),

      addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::B4),
      addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::B4),
      addNote(g, ConcertinaNote::E5),
      addNote(g, ConcertinaNote::D5),
      addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::A5),
      addNote(g, ConcertinaNote::Fsharp5),

      addNote(g, ConcertinaNote::Fsharp5),
      addNote(g, ConcertinaNote::D5),

      addNote(g, ConcertinaNote::C5),
      addNote(g, ConcertinaNote::C5),
      addNote(g, ConcertinaNote::B4),

      addNote(g, ConcertinaNote::A4),
      addNote(g, ConcertinaNote::B4),

      addNote(g, ConcertinaNote::B4),
      addNote(g, ConcertinaNote::A4),
      addNote(g, ConcertinaNote::G4),

      addNote(g, ConcertinaNote::G3),
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
  addSimultaneousNoteEdge(g, seq_nodes[seq_nodes.size()-1], seq_nodes2[seq_nodes.size()-1]);

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

ConcertinaNote midi2note(uint8_t n) {
  switch (n) {
    case 84: return ConcertinaNote::C5;
    case 83: return ConcertinaNote::B5;
    case 81: return ConcertinaNote::A5;
    case 80: return ConcertinaNote::Gsharp5;
    case 79: return ConcertinaNote::G5;
    case 78: return ConcertinaNote::Fsharp5;
    case 77: return ConcertinaNote::F5;
    case 76: return ConcertinaNote::E5;
    case 75: return ConcertinaNote::Dsharp5;
    case 74: return ConcertinaNote::D5;
    case 73: return ConcertinaNote::Csharp5;
    case 72: return ConcertinaNote::C5;
    case 71: return ConcertinaNote::B4;
    case 70: return ConcertinaNote::Bflat4;
    case 69: return ConcertinaNote::A4;
    case 68: return ConcertinaNote::Gsharp4;
    case 67: return ConcertinaNote::G4;
    case 66: return ConcertinaNote::Fsharp4;
    case 65: return ConcertinaNote::F4;
    case 64: return ConcertinaNote::E4;
    case 63: return ConcertinaNote::Dsharp4;
    case 62: return ConcertinaNote::D4;
    case 61: return ConcertinaNote::Csharp4;
    case 60: return ConcertinaNote::C4;
    case 59: return ConcertinaNote::B3;
    case 58: return ConcertinaNote::Bflat3;
    case 57: return ConcertinaNote::A3;
    case 55: return ConcertinaNote::G3;
    case 54: return ConcertinaNote::Fsharp3;
    case 53: return ConcertinaNote::F3;
    case 52: return ConcertinaNote::E3;
    case 48: return ConcertinaNote::C3;
    case 43: return ConcertinaNote::G2;
    case 38: return ConcertinaNote::D2;
    case 36: return ConcertinaNote::C2;
    default: {
      fprintf(stderr, "Unknown note: %u\n", n);
      exit(-1);
    }
  }
}

void test_midi() {
  smf::MidiFile midifile;
  midifile.read("sample.mid");

  while (midifile.getTrackCount() > 1) {
    midifile.mergeTracks(0, 1);
  }
  int tracks = midifile.getTrackCount();

  midifile.sortTracks();
  midifile.doTimeAnalysis();
  midifile.linkNotePairs();

  ConcertinaGraph g{{{}}, {}};

  std::unordered_map<const smf::MidiEvent*, PBQPRAGraph::NodeId> event_map;
  std::unordered_set<PBQPRAGraph::NodeId> live_notes;
  std::unordered_set<PBQPRAGraph::NodeId> recently_ended;
  int last_tick = 0;
  bool last_event_was_note_on = false;

  for (int i = 0, e = midifile[0].getEventCount(); i != e; ++i) {
    const auto& event = midifile[0][i];
    if (event.isNoteOn()) {
      uint8_t note = event[1];

      auto node_id = addNote(g, midi2note(note));
      event_map[&event] = node_id;

      for (auto simul_id : live_notes) {
        addSimultaneousNoteEdge(g, simul_id, node_id);
      }

      live_notes.insert(node_id);

      if (last_event_was_note_on && event.tick - last_tick > 10) {
        recently_ended.clear();
      }

      for (auto seq_id : recently_ended) {
        addSequentialNoteEdge(g, seq_id, node_id);
      }

      last_event_was_note_on = true;
    } else if (event.isNoteOff()) {
      uint8_t note = event[1];
      
      auto node_id = event_map[event.getLinkedEvent()];
      live_notes.erase(node_id);

      if (event.tick - last_tick > 10) {
        recently_ended.clear();
      }

      recently_ended.insert(node_id);
      last_event_was_note_on = false;
    }

    last_tick = event.tick;
  }

  Solution solution = solve(g.graph);

  last_tick = 0;
  bool first = true;
  for (int i = 0, e = midifile[0].getEventCount(); i != e; ++i) {
    const auto& event = midifile[0][i];
    if (!event.isNoteOn()) continue;
    if (first || event.tick - last_tick > 10) {
      printf("\nTime %d:", event.tick);
    }
    auto node = event_map[&event];
    unsigned n1reed = lookupSolution(g, node, solution.getSelection(node));

    printf(" (%s)", GetReedAndFinger(n1reed).c_str()); 
    last_tick = event.tick;
    first = false;
  }
  printf("\n");
}