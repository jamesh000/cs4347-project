#pragma once

#include "db.h"
#include <string>

// Availability functions for seats tied to LEG_INSTANCES.
// Composite key used by seats table:
//   (seat_no, date, leg_no, number)

// Shows availability summary and booked seats for one flight leg instance.
void available(DB& db, const std::string& number, int leg_no, const std::string& date);

// Checks one specific seat number for one flight leg instance.
void available(DB& db, const std::string& number, int leg_no, const std::string& date, int seat_no);

// Longer aliases, in case your team prefers descriptive function names.
void checkAvailability(DB& db, const std::string& number, int leg_no, const std::string& date);
void checkSeatAvailability(DB& db, const std::string& number, int leg_no, const std::string& date, int seat_no);
