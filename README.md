# FlightDB — Milestone 2

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

## Usage

```
prompt> trip("DFW", "SFO")          # by IATA code
prompt> trip("Dallas", "San Francisco")  # by city name
prompt> trip("Dallas", "SFO")       # mixed

prompt> flight("AA3478")            # by flight number

prompt> help                        # show commands
prompt> quit                        # exit
```

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

## Extending for Milestone 3+

- **Add `book()`** — insert into `seats`, decrement `leg_instances.no_of_avail_seats`
- **Add `cancel()`** — delete from `seats`, increment available seats
- **Add date filtering** — join `leg_instances` on a user-supplied date
- **GUI frontend** — `main.cpp` is isolated from the query layer; swap in a web server or Qt UI without touching `queries.h`
