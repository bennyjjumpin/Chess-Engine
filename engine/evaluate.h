#ifndef CHESS_EVALUATE_H
#define CHESS_EVALUATE_H
#include "position.h"

// Static evaluation in centipawns (100 = one pawn), from the point of view
// of the side to move: positive means the side to move is better.
int evaluate(const Position& pos);


#endif //CHESS_EVALUATE_H
