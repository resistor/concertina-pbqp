# Concertina PBQP Solver

## What is the Anglo concertina?

The [Anglo concertina](https://en.wikipedia.org/wiki/Anglo_concertina) 
is a fun, portable free-reed instrument with wide repertoire. The
most common layout is tuned in C/G, and has 30 bisonoric buttons,
meaning that each button plays a different note depending on whether 
you are the bellows are being pushed or pulled. Since each button is
its own voice, a concertina can play full harmonies, i.e. it can
accompany itself like an accordion, though the individual notes of chords
must be played.

## Why is playing new pieces hard?

One of the challenges of playing new pieces on the concertina is that
the button layout is quote constraining:
 * Many notes are only on one direction of one button.
 * A few notes are available in multiple places, sometimes in the same
   direction, and sometimes in the opposite direction.
 * Each button has a natural "home" finger for pressing it. Simultaneously 
   pressing multiple buttons on the same home finger is challenging.
 * Quick changes of buttons with the same finger may be ergonomically 
   difficult if the buttons are physically far apart.

## PBQP for fingering generation

This project maps the problem of generating a fingering for a particular
tune (for a particular concertina layout) onto the [PBQP](http://www.complang.tuwien.ac.at/scholz/pbqp.html) NP-hard problem.
 * Nodes in the PBQP graph represent notes in the tune
 * Edges represent either simultaneous or sequential note constraints
 * Weights encode the physical properties of the concertina layout, both what's physically possible as well as the relative ergonomic cost of particular choices.

 Mapping concertina fingering into an NP-hard problem might not seem like a huge win off the bat, but heuristic solvers for PBQP exist that work well in practice. This project reuses the PBQP solver from [LLVM](https://llvm.org/doxygen/namespacellvm_1_1PBQP.html), which yields good fingerings in practice.

 ## Status

 This project is currently a prototype for me to experiment with the 
 concept, and to generate fingerings for my own use. The tune to be
 generated currently has to be encoded directly in to the C++ source code.

 ## Future Enhancements

  * Support tune input from [ABC](https://abcnotation.com) or other formats
  * Support tune output to... something
  * Represent tunes as intervals rather than notes, enabling the solver to 
    solve for the most playable key on a given concertina layout.
  * More convenient representation of typical chord vamps. They're
    currently a pain to encode by hand.
  * Model the choice of when to play partial/inverted chords to improve
    fingering convenience.