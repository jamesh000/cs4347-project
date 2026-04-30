#pragma once
/**
 * queries.h — Business logic for the two CLI commands.
 *
 *   searchTrip(db, src, dst)   — direct + 1-stop itineraries
 *   searchFlight(db, number)   — flight details
 */

#include "db.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <iomanip>

// ─── ANSI (re-declared here so the header is self-contained) ─────────────────
namespace C {
    const std::string R  = "\033[0m";
    const std::string B  = "\033[1m";
    const std::string D  = "\033[2m";
    const std::string CY = "\033[36m";
    const std::string GR = "\033[32m";
    const std::string YL = "\033[33m";
    const std::string RD = "\033[31m";
    const std::string MG = "\033[35m";
    const std::string BL = "\033[34m";
}

// ─── Formatting helpers ───────────────────────────────────────────────────────

static void printDivider(char ch = '-', int width = 70) {
    std::cout << C::D << std::string(width, ch) << C::R << "\n";
}

static void printHeader(const std::string& title) {
    std::cout << "\n" << C::B << C::CY << "  " << title << C::R << "\n";
    printDivider();
}

static std::string pad(const std::string& s, int width) {
    if ((int)s.size() >= width) return s.substr(0, width);
    return s + std::string(width - s.size(), ' ');
}

// ─── Airport resolution ───────────────────────────────────────────────────────
// Given a user string (3-letter code OR city name), return all matching
// airport_code values.  Comparison is case-insensitive.

static std::vector<std::string> resolveAirports(DB& db, const std::string& input) {
    std::string safe = db.escape(input);

    // Try exact 3-letter code first (case-insensitive)
    std::string sql =
        "SELECT airport_code FROM airports "
        "WHERE UPPER(airport_code) = UPPER('" + safe + "')";

    auto rows = db.query(sql);
    if (!rows.empty()) {
        std::vector<std::string> codes;
        for (auto& r : rows) codes.push_back(r.at("airport_code"));
        return codes;
    }

    // Try city name (partial match)
    sql = "SELECT airport_code FROM airports "
          "WHERE UPPER(city) LIKE UPPER('%" + safe + "%')";

    rows = db.query(sql);
    std::vector<std::string> codes;
    for (auto& r : rows) codes.push_back(r.at("airport_code"));
    return codes;
}

// ─── flight() ─────────────────────────────────────────────────────────────────

void searchFlight(DB& db, const std::string& number) {
    std::string safe = db.escape(number);

    // ── Basic flight info ────────────────────────────────────────────────────
    auto rows = db.query(
        "SELECT f.number, f.airline, f.weekdays "
        "FROM flights f "
        "WHERE UPPER(f.number) = UPPER('" + safe + "')");

    if (rows.empty()) {
        std::cout << C::RD << "  No flight found with number '" << number << "'\n" << C::R;
        return;
    }

    printHeader("FLIGHT " + rows[0]["number"]);
    std::cout << C::B << "  Airline  : " << C::R << rows[0]["airline"]  << "\n";
    std::cout << C::B << "  Number   : " << C::R << rows[0]["number"]   << "\n";
    std::cout << C::B << "  Weekdays : " << C::R << rows[0]["weekdays"] << "\n";

    // ── Legs ─────────────────────────────────────────────────────────────────
    auto legs = db.query(
        "SELECT fl.leg_no, "
        "       a1.airport_code AS dep_code, a1.city AS dep_city, "
        "       fl.scheduled_dep_time, "
        "       a2.airport_code AS arr_code, a2.city AS arr_city, "
        "       fl.scheduled_arr_time "
        "FROM flight_legs fl "
        "JOIN airports a1 ON a1.airport_code = fl.dep_airport_code "
        "JOIN airports a2 ON a2.airport_code = fl.arr_airport_code "
        "WHERE UPPER(fl.number) = UPPER('" + safe + "') "
        "ORDER BY fl.leg_no");

    if (!legs.empty()) {
        std::cout << "\n" << C::B << "  LEGS\n" << C::R;
        std::cout << "  " << C::D
                  << pad("Leg", 5) << pad("Departure", 26)
                  << pad("Arrival", 26) << "Dep Time   Arr Time\n" << C::R;
        printDivider('-', 70);
        for (auto& leg : legs) {
            std::cout << "  "
                      << pad(leg["leg_no"], 5)
                      << pad(leg["dep_code"] + " " + leg["dep_city"], 26)
                      << pad(leg["arr_code"] + " " + leg["arr_city"], 26)
                      << pad(leg["scheduled_dep_time"], 11)
                      << leg["scheduled_arr_time"]
                      << "\n";
        }
    }

    // ── Fares ─────────────────────────────────────────────────────────────────
    auto fares = db.query(
        "SELECT code, amount, restriction "
        "FROM fares "
        "WHERE UPPER(number) = UPPER('" + safe + "') "
        "ORDER BY amount");

    if (!fares.empty()) {
        std::cout << "\n" << C::B << "  FARES\n" << C::R;
        std::cout << "  " << C::D
                  << pad("Code", 8) << pad("Amount ($)", 14) << "Restriction\n" << C::R;
        printDivider('-', 50);
        for (auto& fare : fares) {
            std::cout << "  "
                      << pad(fare["code"], 8)
                      << pad(fare["amount"], 14)
                      << fare["restriction"]
                      << "\n";
        }
    }

    std::cout << "\n";
}

// ─── trip() ───────────────────────────────────────────────────────────────────

// Print a single direct-flight row
static void printDirectRow(int idx, const Row& r) {
    std::cout << "  " << C::YL << "[" << idx << "] " << C::R
              << C::B << r.at("number") << C::R
              << " (" << r.at("airline") << ")"
              << "  " << C::GR << r.at("dep_code") << C::R
              << " → " << C::GR << r.at("arr_code") << C::R
              << "  Dep " << r.at("dep_time")
              << "  Arr " << r.at("arr_time")
              << "  Leg " << r.at("leg_no")
              << "\n";
}

void searchTrip(DB& db, const std::string& srcInput, const std::string& dstInput) {

    // ── Resolve airport codes ─────────────────────────────────────────────────
    auto srcCodes = resolveAirports(db, srcInput);
    auto dstCodes = resolveAirports(db, dstInput);

    if (srcCodes.empty()) {
        std::cout << C::RD << "  No airport found matching '" << srcInput << "'\n" << C::R;
        return;
    }
    if (dstCodes.empty()) {
        std::cout << C::RD << "  No airport found matching '" << dstInput << "'\n" << C::R;
        return;
    }

    // Build SQL IN lists
    auto buildInList = [&](const std::vector<std::string>& codes) -> std::string {
        std::string s = "(";
        for (size_t i = 0; i < codes.size(); ++i) {
            if (i) s += ", ";
            s += "'" + db.escape(codes[i]) + "'";
        }
        s += ")";
        return s;
    };
    std::string srcIn = buildInList(srcCodes);
    std::string dstIn = buildInList(dstCodes);

    // Print resolved airports
    auto showAirports = [&](const std::vector<std::string>& codes, const std::string& label) {
        auto rows = db.query(
            "SELECT airport_code, city, state, name FROM airports "
            "WHERE airport_code IN " + buildInList(codes));
        std::cout << C::D << "  " << label << ": ";
        for (size_t i = 0; i < rows.size(); ++i) {
            if (i) std::cout << " | ";
            std::cout << rows[i]["airport_code"] << " — "
                      << rows[i]["city"] << ", " << rows[i]["state"];
        }
        std::cout << C::R << "\n";
    };

    printHeader("ITINERARIES: " + srcInput + "  →  " + dstInput);
    showAirports(srcCodes, "Origin");
    showAirports(dstCodes, "Destination");

    // ─────────────────────────────────────────────────────────────────────────
    // DIRECT FLIGHTS
    // A direct flight is a single flight_leg that departs from srcCodes
    // and arrives at dstCodes.
    // ─────────────────────────────────────────────────────────────────────────
    std::string directSQL =
        "SELECT fl.leg_no, fl.number, "
        "       f.airline, f.weekdays, "
        "       fl.dep_airport_code  AS dep_code, "
        "       a1.city              AS dep_city, "
        "       fl.scheduled_dep_time AS dep_time, "
        "       fl.arr_airport_code  AS arr_code, "
        "       a2.city              AS arr_city, "
        "       fl.scheduled_arr_time AS arr_time "
        "FROM flight_legs fl "
        "JOIN flights  f  ON f.number       = fl.number "
        "JOIN airports a1 ON a1.airport_code = fl.dep_airport_code "
        "JOIN airports a2 ON a2.airport_code = fl.arr_airport_code "
        "WHERE fl.dep_airport_code IN " + srcIn + " "
        "  AND fl.arr_airport_code IN " + dstIn + " "
        "ORDER BY fl.scheduled_dep_time";

    auto direct = db.query(directSQL);

    std::cout << "\n" << C::B << "  ✈  DIRECT FLIGHTS (" << direct.size() << " found)\n" << C::R;
    printDivider('-', 70);

    if (direct.empty()) {
        std::cout << C::D << "  (none)\n" << C::R;
    } else {
        // Column headers
        std::cout << "  " << C::D
                  << pad("", 5)
                  << pad("Flight", 10)
                  << pad("Airline", 20)
                  << pad("From", 6)
                  << " → "
                  << pad("To", 6)
                  << pad("Dep", 10)
                  << "Arr\n" << C::R;
        for (size_t i = 0; i < direct.size(); ++i) {
            printDirectRow((int)i + 1, direct[i]);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 1-STOP (CONNECTING) FLIGHTS
    // Leg1: src  → connect
    // Leg2: connect → dst
    // Both legs must belong to the same flight number (multi-leg) OR
    // be two separate flights (interline).
    //
    // We handle BOTH cases:
    //   (a) Same flight, different legs (most common for scheduled airlines)
    //   (b) Two separate flights via a connecting airport
    //
    // Note: We do NOT filter on connection time here (add a HAVING clause
    //       or post-filter if desired).
    // ─────────────────────────────────────────────────────────────────────────

    // (a) Same-flight multi-leg connection
    std::string sameFlightSQL =
        "SELECT "
        "  leg1.number        AS flight_no, "
        "  f.airline, "
        "  f.weekdays, "
        "  leg1.dep_airport_code  AS dep_code, "
        "  a_dep.city             AS dep_city, "
        "  leg1.scheduled_dep_time AS dep_time, "
        "  leg1.arr_airport_code   AS via_code, "
        "  a_via.city              AS via_city, "
        "  leg1.scheduled_arr_time AS via_arr_time, "
        "  leg2.scheduled_dep_time AS via_dep_time, "
        "  leg2.arr_airport_code   AS arr_code, "
        "  a_arr.city              AS arr_city, "
        "  leg2.scheduled_arr_time AS arr_time "
        "FROM flight_legs leg1 "
        "JOIN flight_legs leg2 "
        "  ON  leg2.number          = leg1.number "
        "  AND leg2.dep_airport_code = leg1.arr_airport_code "
        "  AND leg2.leg_no           > leg1.leg_no "
        "JOIN flights  f     ON f.number       = leg1.number "
        "JOIN airports a_dep ON a_dep.airport_code = leg1.dep_airport_code "
        "JOIN airports a_via ON a_via.airport_code = leg1.arr_airport_code "
        "JOIN airports a_arr ON a_arr.airport_code = leg2.arr_airport_code "
        "WHERE leg1.dep_airport_code IN " + srcIn + " "
        "  AND leg2.arr_airport_code IN " + dstIn + " "
        "ORDER BY leg1.scheduled_dep_time";

    auto sameFlight = db.query(sameFlightSQL);

    // (b) Interline connection (two different flights)
    std::string interlineSQL =
        "SELECT "
        "  leg1.number           AS flight1_no, "
        "  f1.airline            AS airline1, "
        "  leg1.dep_airport_code AS dep_code, "
        "  a_dep.city            AS dep_city, "
        "  leg1.scheduled_dep_time AS dep_time, "
        "  leg1.arr_airport_code   AS via_code, "
        "  a_via.city              AS via_city, "
        "  leg1.scheduled_arr_time AS via_arr_time, "
        "  leg2.number           AS flight2_no, "
        "  f2.airline            AS airline2, "
        "  leg2.scheduled_dep_time AS via_dep_time, "
        "  leg2.arr_airport_code   AS arr_code, "
        "  a_arr.city              AS arr_city, "
        "  leg2.scheduled_arr_time AS arr_time "
        "FROM flight_legs leg1 "
        "JOIN flight_legs leg2 "
        "  ON  leg2.dep_airport_code = leg1.arr_airport_code "
        "  AND leg2.number           <> leg1.number "
        "JOIN flights  f1    ON f1.number       = leg1.number "
        "JOIN flights  f2    ON f2.number       = leg2.number "
        "JOIN airports a_dep ON a_dep.airport_code = leg1.dep_airport_code "
        "JOIN airports a_via ON a_via.airport_code = leg1.arr_airport_code "
        "JOIN airports a_arr ON a_arr.airport_code = leg2.arr_airport_code "
        "WHERE leg1.dep_airport_code IN " + srcIn + " "
        "  AND leg2.arr_airport_code IN " + dstIn + " "
        "ORDER BY leg1.scheduled_dep_time";

    auto interline = db.query(interlineSQL);

    int totalConnecting = (int)(sameFlight.size() + interline.size());
    std::cout << "\n" << C::B << "  ✈✈ CONNECTING FLIGHTS — 1 STOP ("
              << totalConnecting << " found)\n" << C::R;
    printDivider('-', 70);

    int idx = 1;

    // Print same-flight connections
    if (!sameFlight.empty()) {
        std::cout << C::D << "  ── Same-flight multi-leg ──\n" << C::R;
        for (auto& r : sameFlight) {
            std::cout << "  " << C::YL << "[" << idx++ << "] " << C::R
                      << C::B << r["flight_no"] << C::R
                      << " (" << r["airline"] << ")  "
                      << C::GR << r["dep_code"] << C::R
                      << " [dep " << r["dep_time"] << "] "
                      << "→ " << C::MG << r["via_code"] << C::R
                      << " [arr " << r["via_arr_time"] << " / dep " << r["via_dep_time"] << "] "
                      << "→ " << C::GR << r["arr_code"] << C::R
                      << " [arr " << r["arr_time"] << "]\n";
        }
    }

    // Print interline connections
    if (!interline.empty()) {
        std::cout << C::D << "  ── Interline (two separate flights) ──\n" << C::R;
        for (auto& r : interline) {
            std::cout << "  " << C::YL << "[" << idx++ << "] " << C::R
                      << C::B << r["flight1_no"] << C::R
                      << " (" << r["airline1"] << ")  "
                      << C::GR << r["dep_code"] << C::R
                      << " [dep " << r["dep_time"] << "] "
                      << "→ " << C::MG << r["via_code"] << C::R
                      << " [arr " << r["via_arr_time"] << "]"
                      << "\n           then  "
                      << C::B << r["flight2_no"] << C::R
                      << " (" << r["airline2"] << ") "
                      << "[dep " << r["via_dep_time"] << "] "
                      << "→ " << C::GR << r["arr_code"] << C::R
                      << " [arr " << r["arr_time"] << "]\n";
        }
    }

    if (totalConnecting == 0) {
        std::cout << C::D << "  (none)\n" << C::R;
    }

    std::cout << "\n";
}
