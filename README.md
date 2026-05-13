# FlightDB — Milestone 3

MySQL + C++ Command-Line Application

---

## Requirements

| Dependency      | Install                                          |
| --------------- | ------------------------------------------------ |
| g++ ≥ 7 (C++17) | `sudo apt install g++`                           |
| MySQL C client  | `sudo apt install libmysqlclient-dev`            |
| MySQL server    | any 8.x instance with the FlightDB schema loaded |

user: 'flight_db' @ '127.0.0.1'
ident: 'somepassword'

user: 'flight_db' @ 'localhost'
ident: 'somepassword'
MUST SET YOUR DB_PORT, USER, NAME AND PASS FOR THIS TO WORK.

---

## Build

```bash
make
```

This runs `mysql_config --cflags --libs` automatically to find your MySQL headers and library.

**macOS (Homebrew):**

```bash
brew install mysql-client
export PKG_CONFIG_PATH="$(brew --prefix mysql-client)/lib/pkgconfig"
make
```

---

## Configuration

The app reads credentials from **environment variables** (never hard-code passwords):

| Variable  | Default          | Description    |
| --------- | ---------------- | -------------- |
| `DB_HOST` | `127.0.0.1`      | MySQL host     |
| `DB_PORT` | `3306`           | MySQL port     |
| `DB_USER` | `flight_db`      | MySQL user     |
| `DB_PASS` | _(somepassword)_ | MySQL password |
| `DB_NAME` | `FlightDB`       | Database name  |

```bash
export DB_USER=flightapp
export DB_PASS=secret
./flightdb
```

---

## Usage (Via TUI)

```
prompt> trip("DFW", "SFO")          # by IATA code
prompt> trip("Dallas", "San Francisco")  # by city name
prompt> trip("Dallas", "SFO")       # mixed

prompt> flight("AA3478")            # by flight number

prompt> help                        # show commands
prompt> quit                        # exit
```

---

## Usage (Via GUI)

-- Flight Search --
Input the three letter IATA airport codes of your intended Source & Destination into their respective fields, and click on the "Search for trips" button directly below the Destination field.

---

## What `trip()` Returns

1. **Direct flights** — a single `flight_leg` from source to destination.
2. **1-stop connecting flights** — two legs sharing a connecting airport:
   - _Same-flight multi-leg_ — the airline operates both legs under one flight number (e.g. AA100 Leg 1 + Leg 2).
   - _Interline_ — two different flight numbers that connect at the same airport.

Airport lookup is case-insensitive and supports both 3-letter IATA codes and city name substrings.

## What `flight()` Returns

- Airline name, weekdays of operation
- All legs (departure/arrival airports, scheduled times)
- All fares (code, amount, restriction)

---

## File Structure

```
FlightDB/
├── main.cpp      # REPL, argument parsing, banner
├── db.h          # MySQL C API wrapper (RAII)
├── queries.h     # searchTrip() and searchFlight() business logic
├── Makefile      # Build rules
└── README.md     # This file
```

---

## Justification & Description of Design Patterns

The system architecture for this Database Management System was designed around the idea that multiple uncomplicated, largely independent systems would be easier to troubleshoot and revise than one monolithic structure. To this end, the Database Interface, Business Logic, and UI were split. The Database Interface handles connection management and query execution using an RAII pattern to ensure the MySQL connection is safely closed. The Logic Layer handles the translation of user requests into SQL joins and directly generates the GUI-compatible output by interacting with the imgui API. The GUI layer handles the rendering loop and window state, which was built using GLFW and OpenGL 3 due to particular team members' prior experience; this was also the justification for the use of C++ as the primary language. 

The program's menus were designed primarily around the ideas of modularity and readability. The user is able to easily reorganize their workspace by dragging components around to suit their primary use case. As this modularity would likely make the program more confusing, we decided to compensate by making everything else as easily readable as possible. We implemented color coding to help important data stand out and made sure that the spacing provided enough visual breathing room so that the user would be able to determine exactly where one block of data began and another ended.
