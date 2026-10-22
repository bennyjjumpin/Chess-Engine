#include "book.h"

#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "book_lines.h"
#include "movegen.h"
#include "perft.h"   // uci_to_move, move_to_uci


namespace {

struct BookEntry {
    Move move;
    int weight;
};

// Position hash -> the book moves played from that position.
std::unordered_map<uint64_t, std::vector<BookEntry>> gBook;
bool gBuilt = false;

// Random numbers for picking between book moves. Seeded once from
// std::random_device so each game can go differently.
std::mt19937 gRng{std::random_device{}()};

// Replays every line in BOOK_LINES from the starting position, recording
// each move under the hash of the position it was played from. A move that
// several lines share gets one entry with a higher weight.
void build_book()
{
    for (const char* line : BOOK_LINES)
    {
        Position pos;
        std::istringstream in(line);
        std::string uci;

        while (in >> uci)
        {
            Move m = uci_to_move(pos, uci);
            if (m.from == m.to)
                break;   // illegal move in the line: skip the rest of it

            std::vector<BookEntry>& entries = gBook[pos.hash()];

            bool found = false;
            for (BookEntry& e : entries)
            {
                if (e.move == m)
                {
                    e.weight++;
                    found = true;
                    break;
                }
            }
            if (!found)
                entries.push_back({m, 1});

            pos.make_move(m);
        }
    }
    gBuilt = true;
}

} // namespace

Move book_move(Position& pos)
{
    if (!gBuilt)
        build_book();

    // find, not [], so looking up a non-book position doesn't add an empty entry.
    auto it = gBook.find(pos.hash());
    if (it == gBook.end())
        return Move(0, 0);

    const std::vector<BookEntry>& entries = it->second;

    // Pick an index at random, in proportion to each move's weight.
    std::vector<int> weights;
    for (const BookEntry& e : entries)
        weights.push_back(e.weight);
    std::discrete_distribution<int> pick(weights.begin(), weights.end());
    Move chosen = entries[pick(gRng)].move;

    // Safety check: only play it if it's legal here. A hash collision is
    // astronomically unlikely, but an illegal move would forfeit the game.
    Move legal = uci_to_move(pos, move_to_uci(chosen));
    if (legal.from == legal.to)
        return Move(0, 0);
    return legal;
}