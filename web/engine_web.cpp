// The bridge between the engine and the browser.
//
// JavaScript can't use C++ classes like Position or Move directly, so this
// file keeps one game in a global Position and exposes a few simple
// functions that only take and return strings, numbers and booleans.
// Moves travel as UCI text ("e2e4", "e7e8q"), the same format perft prints.

#include <string>
#include <stdexcept>

#include "position.h"
#include "movegen.h"
#include "search.h"
#include "perft.h"   // move_to_uci, uci_to_move

namespace {
    Position gPos;
}

// Starts a new game from the normal starting position.
void new_game()
{
    gPos = Position();
}

// Loads a position from FEN. Returns false (and keeps the old position)
// if the FEN is malformed.
bool set_position(const std::string& fen)
{
    try
    {
        gPos = Position(fen);
        return true;
    }
    catch (const std::invalid_argument&)
    {
        return false;
    }
}

// All legal moves for the side to move, space-separated: "e2e4 d2d4 g1f3 ..."
std::string legal_moves()
{
    MoveList moves;
    generate_legal_moves(gPos, moves);

    std::string result;
    for (Move m : moves)
    {
        if (!result.empty()) result += ' ';
        result += move_to_uci(m);
    }
    return result;
}

// Plays the player's move. Returns false if it isn't legal.
bool play_move(const std::string& uci)
{
    Move m = uci_to_move(gPos, uci);
    if (m.from == m.to)
        return false;
    gPos.make_move(m);
    return true;
}

// Lets the engine think for up to timeMs milliseconds, plays its move,
// and returns it in UCI form. Returns "" if the game is already over.
std::string engine_move(int timeMs)
{
    Move m = find_best_move(gPos, 64, timeMs);
    if (m.from == m.to)
        return "";
    gPos.make_move(m);
    return move_to_uci(m);
}

// Takes back the last move (used for undo buttons).
void undo_move()
{
    gPos.undo_move();
}

// The board as 64 characters, a1 first and h8 last:
// "PNBRQK" for white pieces, "pnbrqk" for black, '.' for empty.
std::string board()
{
    const char* letters = "PNBRQKpnbrqk";
    std::string result(64, '.');
    for (int sq = 0; sq < 64; sq++)
    {
        Piece p = gPos.piece_on(sq);
        if (p != NO_PIECE)
            result[sq] = letters[p];
    }
    return result;
}

// "w" or "b".
std::string side_to_move()
{
    return gPos.side_to_move() == WHITE ? "w" : "b";
}

// "playing", "checkmate" (the side to move has been mated), "stalemate",
// or a draw: "draw-repetition", "draw-fifty", "draw-material".
std::string game_status()
{
    MoveList moves;
    generate_legal_moves(gPos, moves);
    if (moves.size() == 0)
        return in_check(gPos) ? "checkmate" : "stalemate";

    if (gPos.is_threefold())            return "draw-repetition";
    if (gPos.half_move_clock() >= 100)  return "draw-fifty";
    if (gPos.insufficient_material())   return "draw-material";
    return "playing";
}

// True if the side to move is in check, for highlighting the king.
bool is_in_check()
{
    return in_check(gPos);
}

// ---------- Exposing the functions to JavaScript ----------
// Only compiled by Emscripten; a normal compiler skips this block, so the
// file can still be built and tested natively.
#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>

EMSCRIPTEN_BINDINGS(chess_engine)
{
    emscripten::function("newGame",     &new_game);
    emscripten::function("setPosition", &set_position);
    emscripten::function("legalMoves",  &legal_moves);
    emscripten::function("playMove",    &play_move);
    emscripten::function("engineMove",  &engine_move);
    emscripten::function("undoMove",    &undo_move);
    emscripten::function("board",       &board);
    emscripten::function("sideToMove",  &side_to_move);
    emscripten::function("gameStatus",  &game_status);
    emscripten::function("isInCheck",   &is_in_check);
}
#endif
