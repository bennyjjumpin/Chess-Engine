#include <iostream>
#include <sstream>
#include <string>

#include "position.h"
#include "search.h"
#include "perft.h"   // move_to_uci, uci_to_move

namespace {

    Position gPos;

    void handle_position(std::istringstream& in)
    {
        // TODO: you write this (see below)
    }

    void handle_go(std::istringstream& in)
    {
        // TODO: you write this (see below)
    }

} // namespace

int main()
{
    std::string line;
    while (std::getline(std::cin, line))
    {
        std::istringstream in(line);
        std::string command;
        in >> command;

        if (command == "uci")
        {
            std::cout << "id name BenChess\n";
            std::cout << "id author Bejamin Chatman\n";
            std::cout << "uciok" << std::endl;
        }
        else if (command == "isready")
        {
            std::cout << "readyok" << std::endl;
        }
        else if (command == "ucinewgame")
        {
            gPos = Position();
        }
        else if (command == "position")
        {
            handle_position(in);
        }
        else if (command == "go")
        {
            handle_go(in);
        }
        else if (command == "quit")
        {
            break;
        }
        // Anything else (like "stop" or "setoption") is ignored for now.
    }
    return 0;
}