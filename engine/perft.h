#ifndef CHESS_PERFT_H
#define CHESS_PERFT_H
#include <cstdint>
#include <string>
#include "position.h"

// Counts every legal move sequence of exactly `depth` moves from `pos`.
// Comparing these counts against known values is the standard way to
// verify that move generation, make_move, and undo_move are all correct.
uint64_t perft(Position& pos, int depth);

// Same as perft, but prints the count under each first move separately.
// When a count is wrong, compare this output against a trusted engine
// to find exactly which move's subtree is off, then repeat one level down.
uint64_t perft_divide(Position& pos, int depth);

// A move in UCI notation: "e2e4", or "e7e8q" for a promotion.
std::string move_to_uci(Move move);

// The legal move matching a UCI string, or Move(0, 0) if none matches.
Move uci_to_move(Position& pos, const std::string& uci);

#endif //CHESS_PERFT_H