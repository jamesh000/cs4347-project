/**
 * FlightDB - MySQL C++ Command Line Application
 * Milestone 2: Database Interaction Layer
 *
 * Features:
 *   trip("SRC", "DST")  - Find direct and 1-stop itineraries between airports
 *   flight("XXXXX")     - Look up flight details by flight number
 *   available("NUMBER", LEG, "YYYY-MM-DD") - Check seat availability
 *
 * Build: make
 * Run:   ./flightdb
 */

#include "db.h"
#include "queries.h"
#include "availability.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>

// ─── ANSI colours ────────────────────────────────────────────────────────────
namespace col {
    const std::string reset  = "\033[0m";
    const std::string bold   = "\033[1m";
    const std::string dim    = "\033[2m";
    const std::string cyan   = "\033[36m";
    const std::string green  = "\033[32m";
    const std::string yellow = "\033[33m";
    const std::string red    = "\033[31m";
    const std::string blue   = "\033[34m";
    const std::string magenta= "\033[35m";
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n\"'");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n\"'");
    return s.substr(a, b - a + 1);
}

static std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// Rudimentary parser: extract function name and comma-separated args
// Handles:  trip("DFW","SFO")  or  trip(DFW, SFO)  or  flight("AA3478")
struct Command {
    std::string func;
    std::vector<std::string> args;
    bool valid = false;
};

static Command parse(const std::string& line) {
    Command cmd;
    size_t paren = line.find('(');
    if (paren == std::string::npos) return cmd;

    cmd.func = trim(line.substr(0, paren));
    std::transform(cmd.func.begin(), cmd.func.end(), cmd.func.begin(), ::tolower);

    size_t close = line.rfind(')');
    if (close == std::string::npos || close <= paren) return cmd;

    std::string inner = line.substr(paren + 1, close - paren - 1);
    std::stringstream ss(inner);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        std::string arg = trim(tok);
        if (!arg.empty()) cmd.args.push_back(arg);
    }
    cmd.valid = true;
    return cmd;
}

// ─── Banner ───────────────────────────────────────────────────────────────────

static void printBanner() {
    std::cout << col::cyan << col::bold <<
R"(
  ███████╗██╗     ██╗ ██████╗ ██╗  ██╗████████╗██████╗ ██████╗
  ██╔════╝██║     ██║██╔════╝ ██║  ██║╚══██╔══╝██╔══██╗██╔══██╗
  █████╗  ██║     ██║██║  ███╗███████║   ██║   ██║  ██║██████╔╝
  ██╔══╝  ██║     ██║██║   ██║██╔══██║   ██║   ██║  ██║██╔══██╗
  ██║     ███████╗██║╚██████╔╝██║  ██║   ██║   ██████╔╝██████╔╝
  ╚═╝     ╚══════╝╚═╝ ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚═════╝ ╚═════╝
)" << col::reset;

    std::cout << col::dim << "  MySQL Flight Database — Milestone 2\n" << col::reset;
    std::cout << col::dim << "  ─────────────────────────────────────────────────────────────\n" << col::reset;
    std::cout << "  Commands:\n";
    std::cout << col::green << "    trip(\"SRC\", \"DST\")" << col::reset
              << "  — find direct & 1-stop itineraries\n";
    std::cout << col::green << "    flight(\"NUMBER\")" << col::reset
              << "    — look up flight details\n";
    std::cout << col::green << "    available(\"NUMBER\", LEG, \"YYYY-MM-DD\")" << col::reset
              << " — show seat availability\n";
    std::cout << col::green << "    available(\"NUMBER\", LEG, \"YYYY-MM-DD\", SEAT)" << col::reset
              << " — check one seat\n";
    std::cout << col::yellow << "    help" << col::reset
              << "                  — show this message\n";
    std::cout << col::yellow << "    quit" << col::reset << " / "
              << col::yellow << "exit" << col::reset
              << "          — exit\n\n";
}

static void printHelp() {
    std::cout << "\n" << col::bold << "COMMANDS\n" << col::reset;
    std::cout << col::green << "  trip(\"SRC\", \"DST\")\n" << col::reset;
    std::cout << "    Find all itineraries between two airports.\n";
    std::cout << "    SRC / DST may be:\n";
    std::cout << "      • A 3-letter IATA code  e.g.  trip(\"DFW\", \"SFO\")\n";
    std::cout << "      • A city name           e.g.  trip(\"Dallas\", \"San Francisco\")\n\n";
    std::cout << col::green << "  flight(\"NUMBER\")\n" << col::reset;
    std::cout << "    Show details for a flight by its number.\n";
    std::cout << "      e.g.  flight(\"AA3478\")\n\n";

    std::cout << col::green << "  available(\"NUMBER\", LEG, \"YYYY-MM-DD\")\n" << col::reset;
    std::cout << "    Show seat availability for one flight leg instance.\n";
    std::cout << "      e.g.  available(\"1001\", 1, \"2025-10-01\")\n\n";

    std::cout << col::green << "  available(\"NUMBER\", LEG, \"YYYY-MM-DD\", SEAT)\n" << col::reset;
    std::cout << "    Check one exact seat for that flight leg instance.\n";
    std::cout << "      e.g.  available(\"1001\", 1, \"2025-10-01\", 12)\n\n";
}

// ─── Entry point ──────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    // --- Connection setup ---
    std::string host, user, password, database;
    int port = 3306;

    // Defaults (override via env vars for security)
    host     = getenv("DB_HOST")     ? getenv("DB_HOST")     : "127.0.0.1";
    user     = getenv("DB_USER")     ? getenv("DB_USER")     : "root";
    password = getenv("DB_PASS")     ? getenv("DB_PASS")     : "";
    database = getenv("DB_NAME")     ? getenv("DB_NAME")     : "FlightDB2";
    if (getenv("DB_PORT")) port = std::stoi(getenv("DB_PORT"));

    DB db;
    if (!db.connect(host, user, password, database, port)) {
        std::cerr << col::red << "[error] Cannot connect to MySQL. "
                  << "Set DB_HOST / DB_USER / DB_PASS / DB_NAME env vars.\n"
                  << col::reset;
        return 1;
    }
    std::cout << col::green << "[ok] Connected to MySQL (" << database << ")\n" << col::reset;

    printBanner();

    // --- REPL ---
    std::string line;
    while (true) {
        std::cout << col::cyan << col::bold << "prompt> " << col::reset;
        if (!std::getline(std::cin, line)) break;   // EOF

        line = trim(line);
        if (line.empty()) continue;

        // Quit
        if (line == "quit" || line == "exit" || line == "q") break;

        // Help
        if (line == "help" || line == "?") { printHelp(); continue; }

        Command cmd = parse(line);
        if (!cmd.valid) {
            std::cout << col::red << "  Unrecognised command. Type 'help' for usage.\n" << col::reset;
            continue;
        }

        if (cmd.func == "trip") {
            if (cmd.args.size() != 2) {
                std::cout << col::red << "  Usage: trip(\"SRC\", \"DST\")\n" << col::reset;
                continue;
            }
            searchTrip(db, cmd.args[0], cmd.args[1]);

        } else if (cmd.func == "flight") {
            if (cmd.args.size() != 1) {
                std::cout << col::red << "  Usage: flight(\"NUMBER\")\n" << col::reset;
                continue;
            }
            searchFlight(db, cmd.args[0]);

        } else if (cmd.func == "available" || cmd.func == "availability") {
            if (cmd.args.size() != 3 && cmd.args.size() != 4) {
                std::cout << col::red
                          << "  Usage: available(\"NUMBER\", LEG, \"YYYY-MM-DD\")\n"
                          << "     or: available(\"NUMBER\", LEG, \"YYYY-MM-DD\", SEAT)\n"
                          << col::reset;
                continue;
            }

            std::string number = trim(cmd.args[0]);
            std::string date = trim(cmd.args[2]);
            int leg_no = 0;
            int seat_no = 0;

            try {
                leg_no = std::stoi(trim(cmd.args[1]));
                if (cmd.args.size() == 4) {
                    seat_no = std::stoi(trim(cmd.args[3]));
                }
            } catch (const std::exception&) {
                std::cout << col::red
                          << "  LEG and SEAT must be numbers.\n"
                          << "  Example: available(\"1001\", 1, \"2025-10-01\")\n"
                          << col::reset;
                continue;
            }

            try {
                if (cmd.args.size() == 3) {
                    available(db, number, leg_no, date);
                } else {
                    available(db, number, leg_no, date, seat_no);
                }
            } catch (const std::exception& e) {
                std::cout << col::red
                          << "  Availability error: " << e.what() << "\n"
                          << col::reset;
            }

        } else {
            std::cout << col::red << "  Unknown function '" << cmd.func
                      << "'. Type 'help' for usage.\n" << col::reset;
        }
    }

    std::cout << col::dim << "\nGoodbye.\n" << col::reset;
    return 0;
}
