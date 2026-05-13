# FlightDB — Milestone 3

MySQL/C++/IMGUI Application

---

## Requirements

| Dependency      | Install                                          |
| --------------- | ------------------------------------------------ |
| g++ ≥ 7 (C++17) | `sudo apt install g++`                           |
| MySQL C client  | `sudo apt install libmysqlclient-dev`            |
| MySQL server    | any 8.x instance with the FlightDB schema loaded |
| IMGUI           | `sudo apt install libimgui-dev`                  |

user: 'flight_db' @ '127.0.0.1'
ident: 'somepassword'

user: 'flight_db' @ 'localhost'
ident: 'somepassword'
MUST SET YOUR DB_PORT, USER, NAME AND PASS FOR THIS TO WORK.

---

## Build

Libmysqlclient, GLFW and OpenGL are required. Imgui should be downloaded from https://github.com/ocornut/imgui and placed in the source directory.

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

# Flight Search

To locate available itineraries, use the Input Query window:

    Enter Airport Codes: Input the three-letter IATA airport codes (e.g., DFW for Dallas/Fort Worth or SFO for San Francisco) into the Source and Destination fields.

    Execute Query: Click the Search button directly below the Destination field.

    View Results: A dynamic Trip window will materialize, displaying a table of all valid connections found in the database.

# Understanding the Results Table

The system automatically calculates two types of travel paths:

    Direct Flights: Single-leg entries connecting your chosen airports without stops.

    Connecting Flights: One-stop itineraries identified via a self-join on the flight_legs table. This includes:

        Same-flight transitions: One flight number with multiple stops.

        Interline connections: Switching flight numbers at a hub airport.

To ensure data clarity, airport codes are rendered in Cyan and flight numbers are Bolded. Each row provides critical details, including leg numbers, scheduled departure/arrival times, and the operating airline.
# Seat Availability

To inspect specific seating for a flight instance:

    Identify the Flight: Note the flight number and date from your active Trip results.

    System Lookup: The backend queries the leg_instances and seats tables, comparing the total capacity of the assigned aircraft against current reservations.

    Availability Status:

        Available Seats: Displayed as a list of open seat numbers.

        Booked Seats: Displayed with the associated customer name and contact info (intended for administrative oversight).

# Workspace Customization

The GUI utilizes an Immediate-Mode framework, allowing you to tailor your workspace in real-time:

    Rearranging: Click and drag the title bar of any window (Input Query, Trip, or Debug) to reposition it.

    Resizing: Use the handle in the bottom-right corner of any window to expand data tables for better visibility.

    Persistence: Your window layout is automatically saved to imgui.ini and will persist across sessions.

    Closing: Once an inquiry is finished, you can close the Trip window to clear the workspace. The Input Query window remains as your persistent anchor.

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

## File Structure (Excluding CSVs & Deprecated)

```
FlightDB/
├── main.cpp            # Entry point; manages GLFW/OpenGL 3 loop and ImGui windows
├── db.h                # Core Database Interface; RAII wrapper for MySQL C API
├── queries.h           # Service Layer; logic for itinerary and flight searching
├── availability.h      # Header for seat and leg instance availability checking
├── availability.cpp    # Implementation of availability logic and terminal formatting
├── schema.sql          # Database definition; contains table structures and CSV loading
├── imgui.ini           # Configuration file; saves window positions and GUI state
├── Makefile            # Build instructions and compiler flags
└── README.md           # Project documentation and architectural overview
```

---

## Justification & Description of Design Patterns

The system architecture for this Database Management System was designed around the idea that multiple uncomplicated, largely independent systems would be easier to troubleshoot and revise than one monolithic structure. To this end, the Database Interface, Business Logic, and UI were split. The Database Interface handles connection management and query execution using an RAII pattern to ensure the MySQL connection is safely closed. The Logic Layer handles the translation of user requests into SQL joins and directly generates the GUI-compatible output by interacting with the imgui API. The GUI layer handles the rendering loop and window state, which was built using GLFW and OpenGL 3 due to particular team members' prior experience; this was also the justification for the use of C++ as the primary language. 

The program's menus were designed primarily around the ideas of modularity and readability. The user is able to easily reorganize their workspace by dragging components around to suit their primary use case. As this modularity would likely make the program more confusing, we decided to compensate by making everything else as easily readable as possible. We implemented color coding to help important data stand out and made sure that the spacing provided enough visual breathing room so that the user would be able to determine exactly where one block of data began and another ended.

## Simplified UML Code Diagram

+-------------------------------------------------------------+
|                      USER INTERFACE (UI)                    |
|       [main.cpp] using GLFW, OpenGL 3, and Dear ImGui       |
+------------------------------+------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                    SERVICE / LOGIC LAYER                    |
|       [queries.h]              |      [availability.cpp]    |
|   - searchTrip()               |   - checkAvailability()    |
|   - searchFlight()             |   - checkSeat()            |
+------------------------------+------------------------------+
                               |
                               | (Dependencies)
                               v
+-------------------------------------------------------------+
|                   DATABASE INTERFACE (RAII)                 |
|                        [db.h] Class                         |
+-------------------------------------------------------------+
|  - conn_ : MYSQL                                            |
|  + query(sql) : vector<Row>                                 |
|  + escape(string) : string                                  |
+------------------------------+------------------------------+
                               |
                               | (TCP/Socket)
                               v
+-------------------------------------------------------------+
|                      RELATIONAL DATABASE                    |
|                       [MySQL / schema.sql]                  |
+-------------------------------------------------------------+
|  [airports] <--- [flight_legs] <--- [leg_instances]         |
|  [flights]  ----------------------> [seats]                |
+-------------------------------------------------------------+
