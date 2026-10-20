#include "search.h"
#include "evaluate.h"
#include "movegen.h"
#include <algorithm>

namespace {

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
}

Move find_best_move(Position& pos, int depth)
{
    MoveList moves;
    generate_legal_moves(pos, moves);
    
    if (moves.size() == 0)
        return Move(0, 0);

    order_moves(pos, moves);

    Move bestMove = moves[0];
    int alpha = -INFINITY_SCORE;
    int beta = INFINITY_SCORE;

    for (Move m : moves)
    {
        pos.make_move(m);

        int score = -negamax(pos, depth - 1, -beta, -alpha, 1);
        pos.undo_move();

        if (score > alpha)
        {
            alpha = score;
            bestMove = m;
        }
    }
    return bestMove;
}

