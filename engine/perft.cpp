#include "perft.h"
#include <cstdio>
#include <cstdlib>
#include "movegen.h"

uint64_t perft(Position& pos, int depth)
{
    if (depth == 0)
        return 1;

    MoveList moves;
    generate_legal_moves(pos, moves);

    // At the last level, every legal move is one leaf, so there's no need
    // to make each move just to count it. This "bulk counting" makes perft
    // several times faster without changing the result.
    if (depth == 1)
        return moves.size();

    uint64_t nodes = 0;
    for (Move m : moves)
    {
        pos.make_move(m);
        nodes += perft(pos, depth - 1);
        pos.undo_move();
    }
    return nodes;
}

uint64_t perft_divide(Position& pos, int depth)
{
    MoveList moves;
    generate_legal_moves(pos, moves);

    uint64_t total = 0;
    for (Move m : moves)
    {
        pos.make_move(m);
        uint64_t count = perft(pos, depth - 1);
        pos.undo_move();

        std::printf("%s: %llu\n", move_to_uci(m).c_str(), static_cast<unsigned long long>(count));
        total += count;
    }
    std::printf("\nMoves: %d\nNodes: %llu\n", moves.size(), static_cast<unsigned long long>(total));
    return total;
}

std::string move_to_uci(Move move)
{
    std::string s;
    s += static_cast<char>('a' + move.from % 8);
    s += static_cast<char>('1' + move.from / 8);
    s += static_cast<char>('a' + move.to % 8);
    s += static_cast<char>('1' + move.to / 8);
    if (move.is_promotion())
        s += "pnbrqk"[move.promotion];
    return s;
}

// Turns a UCI string like "e2e4" or "e7e8q" into the matching legal move.
// Returns Move(0, 0) if the string doesn't match any legal move, so
// illegal or garbled input from the player is rejected automatically.
Move uci_to_move(Position& pos, const std::string& uci)
{
    MoveList moves;
    generate_legal_moves(pos, moves);

    for (Move m : moves)
        if (move_to_uci(m) == uci)
            return m;

    return Move(0, 0);
}