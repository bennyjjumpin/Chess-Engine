#ifndef CHESS_BOOK_H
#define CHESS_BOOK_H
#include "position.h"

// A random book move for this position, weighted by how many book lines
// play it, or Move(0, 0) if the position isn't in the book.
Move book_move(Position& pos);

#endif //CHESS_BOOK_H

