#include "availability.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>

namespace availability_colors {
    const std::string reset  = "\033[0m";
    const std::string bold   = "\033[1m";
    const std::string dim    = "\033[2m";
    const std::string green  = "\033[32m";
    const std::string yellow = "\033[33m";
    const std::string red    = "\033[31m";
    const std::string cyan   = "\033[36m";
}

static std::string q(DB& db, const std::string& value) {
    return "'" + db.escape(value) + "'";
}

static void printLine(char ch = '-', int n = 72) {
    std::cout << "  ";
    for (int i = 0; i < n; ++i) std::cout << ch;
    std::cout << "\n";
}

static std::string seatList(const std::vector<Row>& rows) {
    std::ostringstream out;
    for (size_t i = 0; i < rows.size(); ++i) {
        if (i) out << ", ";
        out << rows[i].at("seat_no");
    }
    return out.str();
}

void checkAvailability(DB& db, const std::string& number, int leg_no, const std::string& date) {
    using namespace availability_colors;

    const std::string flightNumber = db.escape(number);
    const std::string flightDate   = db.escape(date);

    // Seats are now tied to a specific LEG_INSTANCE, whose key is:
    //   (date, leg_no, number)
    // The seats table references that same leg instance and adds seat_no:
    //   (seat_no, date, leg_no, number)
    std::string instanceSQL =
        "SELECT li.number, li.leg_no, li.date, li.no_of_avail_seats, "
        "       li.airplane_id, a.total_no_of_seats, a.typename, "
        "       fl.dep_airport_code, fl.arr_airport_code, "
        "       fl.scheduled_dep_time, fl.scheduled_arr_time "
        "FROM leg_instances li "
        "JOIN flight_legs fl ON fl.number = li.number AND fl.leg_no = li.leg_no "
        "LEFT JOIN airplanes a ON a.airplane_id = li.airplane_id "
        "WHERE UPPER(li.number) = UPPER('" + flightNumber + "') "
        "  AND li.leg_no = " + std::to_string(leg_no) + " "
        "  AND li.date = '" + flightDate + "'";

    auto instance = db.query(instanceSQL);

    if (instance.empty()) {
        std::cout << red << "  No leg instance found for flight " << number
                  << ", leg " << leg_no << ", date " << date << ".\n" << reset;
        return;
    }

    const Row& r = instance.front();

    std::string seatsSQL =
        "SELECT seat_no, customer_name, cphone "
        "FROM seats "
        "WHERE UPPER(number) = UPPER('" + flightNumber + "') "
        "  AND leg_no = " + std::to_string(leg_no) + " "
        "  AND date = '" + flightDate + "' "
        "ORDER BY seat_no";

    auto bookedSeats = db.query(seatsSQL);

    std::cout << "\n" << cyan << bold << "  AVAILABILITY\n" << reset;
    printLine('=');
    std::cout << "  Flight: " << bold << r.at("number") << reset
              << " | Leg: " << r.at("leg_no")
              << " | Date: " << r.at("date") << "\n";
    std::cout << "  Route:  " << r.at("dep_airport_code") << " -> " << r.at("arr_airport_code")
              << " | Dep: " << r.at("scheduled_dep_time")
              << " | Arr: " << r.at("scheduled_arr_time") << "\n";
    std::cout << "  Plane:  " << r.at("airplane_id")
              << " | Type: " << r.at("typename")
              << " | Total seats: " << r.at("total_no_of_seats") << "\n";

    std::cout << green << "  Available seats: " << r.at("no_of_avail_seats") << reset << "\n";
    std::cout << yellow << "  Booked seat records: " << bookedSeats.size() << reset << "\n";

    if (bookedSeats.empty()) {
        std::cout << dim << "  No booked seat rows found for this leg instance.\n" << reset;
    } else {
        std::cout << "\n" << bold << "  Booked seats\n" << reset;
        printLine('-');
        std::cout << "  " << std::left
                  << std::setw(10) << "Seat"
                  << std::setw(24) << "Customer"
                  << "Phone\n";
        printLine('-');

        for (const Row& seat : bookedSeats) {
            std::cout << "  " << std::left
                      << std::setw(10) << seat.at("seat_no")
                      << std::setw(24) << seat.at("customer_name")
                      << seat.at("cphone") << "\n";
        }
    }

    std::cout << "\n";
}

void checkSeatAvailability(DB& db, const std::string& number, int leg_no, const std::string& date, int seat_no) {
    using namespace availability_colors;

    const std::string flightNumber = db.escape(number);
    const std::string flightDate   = db.escape(date);

    std::string instanceSQL =
        "SELECT li.number, li.leg_no, li.date, li.no_of_avail_seats, "
        "       a.total_no_of_seats "
        "FROM leg_instances li "
        "LEFT JOIN airplanes a ON a.airplane_id = li.airplane_id "
        "WHERE UPPER(li.number) = UPPER('" + flightNumber + "') "
        "  AND li.leg_no = " + std::to_string(leg_no) + " "
        "  AND li.date = '" + flightDate + "'";

    auto instance = db.query(instanceSQL);
    if (instance.empty()) {
        std::cout << red << "  No leg instance found for flight " << number
                  << ", leg " << leg_no << ", date " << date << ".\n" << reset;
        return;
    }

    std::string seatSQL =
        "SELECT seat_no, customer_name, cphone "
        "FROM seats "
        "WHERE UPPER(number) = UPPER('" + flightNumber + "') "
        "  AND leg_no = " + std::to_string(leg_no) + " "
        "  AND date = '" + flightDate + "' "
        "  AND seat_no = " + std::to_string(seat_no);

    auto seat = db.query(seatSQL);

    std::cout << "\n" << cyan << bold << "  SEAT CHECK\n" << reset;
    printLine('=');
    std::cout << "  Flight: " << number
              << " | Leg: " << leg_no
              << " | Date: " << date
              << " | Seat: " << seat_no << "\n";

    if (seat.empty()) {
        std::cout << green << "  Status: AVAILABLE\n" << reset;
    } else {
        std::cout << red << "  Status: BOOKED\n" << reset;
        std::cout << "  Customer: " << seat.front().at("customer_name")
                  << " | Phone: " << seat.front().at("cphone") << "\n";
    }

    std::cout << "\n";
}

void available(DB& db, const std::string& number, int leg_no, const std::string& date) {
    checkAvailability(db, number, leg_no, date);
}

void available(DB& db, const std::string& number, int leg_no, const std::string& date, int seat_no) {
    checkSeatAvailability(db, number, leg_no, date, seat_no);
}
