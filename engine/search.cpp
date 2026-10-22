#include "search.h"
#include "evaluate.h"
#include "movegen.h"
#include "book.h"
#include <algorithm>
#include <chrono>

namespace {

using Clock = std::chrono::steady_clock;
Clock::time_point gDeadline;
bool gStop = false;
uint64_t gNodes = 0;

// Checks the clock every 2048 nodes, since reading it at every node is slow.
// Once time is up, gStop stays set until the next search starts.
bool time_up()
{
    if ((++gNodes & 2047) == 0 && Clock::now() >= gDeadline)
        gStop = true;
    return gStop;
}

bool is_capture(const Position& pos, Move m)
{
    return pos.piece_on(m.to) != NO_PIECE || m.flag == EN_PASSANT;
}

// Piece values for ordering, indexed by PieceType. The king gets a real
// value here so a king capture sorts as an "expensive attacker".
constexpr int ORDER_VALUES[6] = {100, 320, 330, 500, 900, 1000};

// Higher scores are searched first.
int score_move(const Position& pos, Move m)
{
    int score = 0;

    if (is_capture(pos, m))
    {
        // MVV-LVA: the victim is weighted 10x so it dominates, and the
        // attacker only breaks ties (pawn takes queen before queen takes queen).
        PieceType victim = (m.flag == EN_PASSANT) ? PAWN : get_type(pos.piece_on(m.to));
        PieceType attacker = get_type(pos.piece_on(m.from));
        score = 10'000 + 10 * ORDER_VALUES[victim] - ORDER_VALUES[attacker];
    }

    if (m.is_promotion())
        score += 5'000 + ORDER_VALUES[m.promotion];

    return score;
}

// Sorts `moves` so the most promising are searched first.
void order_moves(const Position& pos, MoveList& moves)
{
    struct ScoredMove { Move move; int score; };
    std::array<ScoredMove, 256> scored;
    int n = moves.size();

    for (int i = 0; i < n; i++)
        scored[i] = {moves[i], score_move(pos, moves[i])};

    std::stable_sort(scored.begin(), scored.begin() + n,
                     [](const ScoredMove& a, const ScoredMove& b) { return a.score > b.score; });

    for (int i = 0; i < n; i++)
        moves.moves[i] = scored[i].move;
}

int quiescence(Position& pos, int alpha, int beta)
{
    if (time_up()) return 0;

    int standPat = evaluate(pos);
    if (standPat >= beta)
    {
        return beta;
    }
    if (standPat > alpha)
    {
        alpha = standPat;
    }

    MoveList legal;
    generate_legal_moves(pos, legal);
    order_moves(pos, legal);

    for (auto move : legal)
    {
        if (is_capture(pos, move))
        {
            pos.make_move(move);
            int score = -quiescence(pos, -beta, -alpha);
            pos.undo_move();
            if (score >= beta)
            {
                return beta;
            }
            if (score > alpha)
            {
                alpha = score;
            }
        }
    }
    return alpha;
}

int negamax(Position& pos, int depth, int alpha, int beta, int ply)
{
    if (time_up()) return 0;

    if (pos.is_draw()) return 0;

    if (depth == 0)
    {
        return quiescence(pos, alpha, beta);
    }
    MoveList legal;

    generate_legal_moves(pos, legal);

    if (legal.size() == 0)
    {
        if (in_check(pos))
        {
            return -MATE_SCORE + ply;
        }
        else
        {
            return 0;
        }
    }

    order_moves(pos, legal);

    for (auto move : legal)
    {
        pos.make_move(move);
        int score = -negamax(pos, depth - 1, -beta, -alpha, ply + 1);
        pos.undo_move();
        if (score >= beta)
        {
            return beta;
        }
        if (score > alpha)
        {
            alpha = score;
        }
    }
    return alpha;
}

// Searches one fixed depth. bestMove comes in holding the previous
// iteration's best move, which is searched first, and is updated only
// if this iteration finishes before time runs out.
int search_root(Position& pos, int depth, Move& bestMove)
{
    MoveList moves;
    generate_legal_moves(pos, moves);
    order_moves(pos, moves);

    for (int i = 0; i < moves.size(); i++)
    {
        if (moves.moves[i] == bestMove)
        {
            // Slide everything before it down one slot, then put it first.
            Move previousBest = moves.moves[i];
            for (int j = i; j > 0; j--)
                moves.moves[j] = moves.moves[j - 1];
            moves.moves[0] = previousBest;
            break;
        }
    }

    int alpha = -INFINITY_SCORE;
    int beta = INFINITY_SCORE;
    Move best = moves[0];

    for (Move m : moves)
    {
        pos.make_move(m);
        int score = -negamax(pos, depth - 1, -beta, -alpha, 1);
        pos.undo_move();

        if (gStop)
            break;          // this iteration is incomplete, so don't trust it

        if (score > alpha)
        {
            alpha = score;
            best = m;
        }
    }

    if (!gStop)
        bestMove = best;
    return alpha;
}
}

Move find_best_move(Position& pos, int maxDepth, int timeLimitMs)
{
    Move bookMove = book_move(pos);
    if (bookMove.from != bookMove.to)
        return bookMove;

    MoveList moves;
    generate_legal_moves(pos, moves);
    if (moves.size() == 0)
        return Move(0, 0);

    gStop = false;
    gNodes = 0;
    gDeadline = Clock::now() + std::chrono::milliseconds(timeLimitMs);

    Move bestMove = moves[0];   // a legal fallback even if depth 1 can't finish
    for (int depth = 1; depth <= maxDepth; depth++)
    {
        search_root(pos, depth, bestMove);
        if (gStop)
            break;
    }
    return bestMove;
}

