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
#include "imgui.h"

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
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                           "No flight found with number '%s'",
                           number.c_str());
        return;
    }

    auto& flight = rows[0];

    // ── Flight Header ────────────────────────────────────────────────────────
    std::string title = "FLIGHT " + flight["number"];
    ImGui::SeparatorText(title.c_str());

    ImGui::Text("Airline:");
    ImGui::SameLine(150);
    ImGui::TextUnformatted(flight["airline"].c_str());

    ImGui::Text("Number:");
    ImGui::SameLine(150);
    ImGui::TextUnformatted(flight["number"].c_str());

    ImGui::Text("Weekdays:");
    ImGui::SameLine(150);
    ImGui::TextUnformatted(flight["weekdays"].c_str());

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
        ImGui::Spacing();
        ImGui::SeparatorText("LEGS");

        if (ImGui::BeginTable("LegsTable", 5,
                              ImGuiTableFlags_Borders |
                              ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_SizingStretchProp)) {

            ImGui::TableSetupColumn("Leg");
            ImGui::TableSetupColumn("Departure");
            ImGui::TableSetupColumn("Arrival");
            ImGui::TableSetupColumn("Dep Time");
            ImGui::TableSetupColumn("Arr Time");
            ImGui::TableHeadersRow();

            for (auto& leg : legs) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(leg["leg_no"].c_str());

                ImGui::TableSetColumnIndex(1);
                std::string dep =
                    leg["dep_code"] + " " + leg["dep_city"];
                ImGui::TextUnformatted(dep.c_str());

                ImGui::TableSetColumnIndex(2);
                std::string arr =
                    leg["arr_code"] + " " + leg["arr_city"];
                ImGui::TextUnformatted(arr.c_str());

                ImGui::TableSetColumnIndex(3);
                ImGui::TextUnformatted(
                    leg["scheduled_dep_time"].c_str());

                ImGui::TableSetColumnIndex(4);
                ImGui::TextUnformatted(
                    leg["scheduled_arr_time"].c_str());
            }

            ImGui::EndTable();
        }
    }

    // ── Fares ────────────────────────────────────────────────────────────────
    auto fares = db.query(
        "SELECT code, amount, restriction "
        "FROM fares "
        "WHERE UPPER(number) = UPPER('" + safe + "') "
        "ORDER BY amount");

    if (!fares.empty()) {
        ImGui::Spacing();
        ImGui::SeparatorText("FARES");

        if (ImGui::BeginTable("FaresTable", 3,
                              ImGuiTableFlags_Borders |
                              ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_Resizable |
                              ImGuiTableFlags_SizingStretchProp)) {

            ImGui::TableSetupColumn("Code");
            ImGui::TableSetupColumn("Amount ($)");
            ImGui::TableSetupColumn("Restriction");
            ImGui::TableHeadersRow();

            for (auto& fare : fares) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(fare["code"].c_str());

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("$%s", fare["amount"].c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(
                    fare["restriction"].c_str());
            }

            ImGui::EndTable();
        }
    }

    ImGui::Spacing();
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

    if (srcInput.length() != 3 || dstInput.length() != 3) {
        return;
    }

    if (srcCodes.empty()) {
        return;
    }
    if (dstCodes.empty()) {
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
    auto showAirports = [&](const std::vector<std::string>& codes, const std::string& label)
        {
            auto rows = db.query(
                "SELECT airport_code, city, state, name FROM airports "
                "WHERE airport_code IN " + buildInList(codes));

            ImGui::TextDisabled("%s:", label.c_str());
            ImGui::SameLine();

            if (rows.empty())
            {
                ImGui::TextUnformatted("(none)");
                return;
            }

            for (size_t i = 0; i < rows.size(); ++i)
            {
                const auto& r = rows[i];

                if (i > 0)
                    ImGui::SameLine();

                ImGui::Text("%s — %s, %s",
                            r.at("airport_code").c_str(),
                            r.at("city").c_str(),
                            r.at("state").c_str()
                    );

                if (i + 1 < rows.size())
                    ImGui::TextUnformatted("|");
            }
        };

    ImGui::SeparatorText(("ITINERARIES: " + srcInput + " → " + dstInput).c_str());
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
    
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 1.0f, 1.0f),
                       "✈ DIRECT FLIGHTS (%zu found)", direct.size());

    ImGui::Separator();

    if (direct.empty()) {
        ImGui::TextDisabled("(none)");
    } else {
        if (ImGui::BeginTable("direct_flights", 7,
                              ImGuiTableFlags_Borders |
                              ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Flight");
            ImGui::TableSetupColumn("Airline");
            ImGui::TableSetupColumn("From");
            ImGui::TableSetupColumn("To");
            ImGui::TableSetupColumn("Dep");
            ImGui::TableSetupColumn("Arr");
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < direct.size(); ++i)
            {
                const Row& r = direct[i];

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(1, 0.85f, 0.2f, 1), "[%d]", (int)i + 1);

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", r.at("number").c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("(%s)", r.at("airline").c_str());

                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%s", r.at("dep_code").c_str());

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%s", r.at("arr_code").c_str());

                ImGui::TableSetColumnIndex(5);
                ImGui::Text("%s", r.at("dep_time").c_str());

                ImGui::TableSetColumnIndex(6);
                ImGui::Text("%s", r.at("arr_time").c_str());
            }

            ImGui::EndTable();
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

    ImGui::Spacing();
    ImGui::SeparatorText("✈✈ CONNECTING FLIGHTS — 1 STOP");

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.40f, 0.70f, 1.00f, 1.0f),
                       "(%d found)", totalConnecting);

    int idx = 1;

// ============================================================
// SAME-FLIGHT MULTI-LEG
// ============================================================

    if (!sameFlight.empty())
    {
        ImGui::Spacing();

        ImGui::TextDisabled("── Same-flight multi-leg ──");

        if (ImGui::BeginTable(
                "same_flight_connections",
                8,
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersInnerH |
                ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Flight");
            ImGui::TableSetupColumn("Airline");
            ImGui::TableSetupColumn("From");
            ImGui::TableSetupColumn("Departure");
            ImGui::TableSetupColumn("Via");
            ImGui::TableSetupColumn("Transfer");
            ImGui::TableSetupColumn("Arrival");

            ImGui::TableHeadersRow();

            for (auto& r : sameFlight)
            {
                ImGui::TableNextRow();

                // #
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(
                    ImVec4(1.00f, 0.85f, 0.20f, 1.0f),
                    "[%d]",
                    idx++);

                // Flight
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(
                    ImVec4(0.40f, 0.70f, 1.00f, 1.0f),
                    "%s",
                    r["flight_no"].c_str());

                // Airline
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("(%s)",
                            r["airline"].c_str());

                // From
                ImGui::TableSetColumnIndex(3);
                ImGui::TextColored(
                    ImVec4(0.40f, 1.00f, 0.40f, 1.0f),
                    "%s",
                    r["dep_code"].c_str());

                // Departure
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("dep %s",
                            r["dep_time"].c_str());

                // Via
                ImGui::TableSetColumnIndex(5);
                ImGui::TextColored(
                    ImVec4(1.00f, 0.40f, 1.00f, 1.0f),
                    "%s",
                    r["via_code"].c_str());

                // Transfer
                ImGui::TableSetColumnIndex(6);
                ImGui::Text(
                    "arr %s / dep %s",
                    r["via_arr_time"].c_str(),
                    r["via_dep_time"].c_str());

                // Arrival
                ImGui::TableSetColumnIndex(7);
                ImGui::Text(
                    "%s  [arr %s]",
                    r["arr_code"].c_str(),
                    r["arr_time"].c_str());
            }

            ImGui::EndTable();
        }
    }

// ============================================================
// INTERLINE CONNECTIONS
// ============================================================

    if (!interline.empty())
    {
        ImGui::Spacing();

        ImGui::TextDisabled(
            "── Interline (two separate flights) ──");

        if (ImGui::BeginTable(
                "interline_connections",
                9,
                ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersInnerH |
                ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("#");
            ImGui::TableSetupColumn("Flight 1");
            ImGui::TableSetupColumn("Airline");
            ImGui::TableSetupColumn("From");
            ImGui::TableSetupColumn("Dep");
            ImGui::TableSetupColumn("Via");
            ImGui::TableSetupColumn("Arr @ Via");
            ImGui::TableSetupColumn("Flight 2");
            ImGui::TableSetupColumn("Final");

            ImGui::TableHeadersRow();

            for (auto& r : interline)
            {
                ImGui::TableNextRow();

                // #
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(
                    ImVec4(1.00f, 0.85f, 0.20f, 1.0f),
                    "[%d]",
                    idx++);

                // Flight 1
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(
                    ImVec4(0.40f, 0.70f, 1.00f, 1.0f),
                    "%s",
                    r["flight1_no"].c_str());

                // Airline 1
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("(%s)",
                            r["airline1"].c_str());

                // From
                ImGui::TableSetColumnIndex(3);
                ImGui::TextColored(
                    ImVec4(0.40f, 1.00f, 0.40f, 1.0f),
                    "%s",
                    r["dep_code"].c_str());

                // Departure
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("dep %s",
                            r["dep_time"].c_str());

                // Via
                ImGui::TableSetColumnIndex(5);
                ImGui::TextColored(
                    ImVec4(1.00f, 0.40f, 1.00f, 1.0f),
                    "%s",
                    r["via_code"].c_str());

                // Arrive via
                ImGui::TableSetColumnIndex(6);
                ImGui::Text("arr %s",
                            r["via_arr_time"].c_str());

                // Flight 2
                ImGui::TableSetColumnIndex(7);

                ImGui::TextColored(
                    ImVec4(0.40f, 0.70f, 1.00f, 1.0f),
                    "%s",
                    r["flight2_no"].c_str());

                ImGui::SameLine();

                ImGui::Text(
                    "(%s) dep %s",
                    r["airline2"].c_str(),
                    r["via_dep_time"].c_str());

                // Final arrival
                ImGui::TableSetColumnIndex(8);

                ImGui::Text(
                    "%s [arr %s]",
                    r["arr_code"].c_str(),
                    r["arr_time"].c_str());
            }

            ImGui::EndTable();
        }
    }

    if (totalConnecting == 0)
    {
        ImGui::TextDisabled("(none)");
    }

    ImGui::Spacing();
}

// Itinerary check
void itinerary(DB &db, const std::string& name) {
    std::string userName = db.escape(name);

    auto rows =
        db.query("SELECT s.customer_name, s.number, s.date, s.leg_no, "
                 "fl.dep_airport_code, fl.arr_airport_code, "
                 "fl.scheduled_dep_time, fl.scheduled_arr_time, s.seat_no "
                 "FROM seats s "
                 "JOIN leg_instances li "
                 "ON li.number = s.number "
                 "AND li.leg_no = s.leg_no "
                 "AND li.date = s.date "
                 "JOIN flight_legs fl "
                 "ON fl.number = li.number "
                 "AND fl.leg_no = li.leg_no "
                 "WHERE UPPER(s.customer_name) = UPPER('" +
                 userName +
                 "') "
                 "ORDER BY s.number");

    
    if (!rows.empty()) {
        std::cout << "\n" << C::B << "  RESERVATIONS\n" << C::R;

        std::cout << "  " << C::D
                  << pad("Name", 20)
                  << pad("Flight", 10)
                  << pad("Date", 12)
                  << pad("Leg", 5)
                  << pad("From", 8)
                  << pad("To", 8)
                  << pad("Dep Time", 11)
                  << pad("Arr Time", 11)
                  << "Seat\n"
                  << C::R;

        printDivider('-', 100);

        for (auto& row : rows) {
            std::cout << "  "
                      << pad(row["customer_name"], 20)
                      << pad(row["number"], 10)
                      << pad(row["date"], 12)
                      << pad(row["leg_no"], 5)
                      << pad(row["dep_airport_code"], 8)
                      << pad(row["arr_airport_code"], 8)
                      << pad(row["scheduled_dep_time"], 11)
                      << pad(row["scheduled_arr_time"], 11)
                      << row["seat_no"]
                      << "\n";
        }
    }
}

void util(DB &db, const std::string& start, const std::string& end) {
    std::string startDate = db.escape(start);
    std::string endDate = db.escape(end);

    auto rows = db.query(
        "SELECT a.airplane_id, a.typename, COUNT(li.number) AS total_flights "
        "FROM airplanes a "
        "LEFT JOIN leg_instances li "
        "ON a.airplane_id = li.airplane_id "
        "AND li.date BETWEEN '" +
        startDate + "' AND '" + endDate +
        "' "
        "GROUP BY "
        "a.airplane_id, "
        "a.typename "
        "ORDER BY a.airplane_id;");

    if (!rows.empty()) {
        std::cout << "\n" << C::B << "  AIRCRAFT UTILIZATION REPORT\n" << C::R;
        
        std::cout << "  " << C::D
                  << pad("Aircraft", 18)
                  << pad("Type", 20)
                  << "Flights\n" << C::R;

        printDivider('-', 55);

        for (auto& p : rows) {
            std::cout << "  "
                      << pad(p["airplane_id"], 18)
                      << pad(p["typename"], 20)
                      << p["total_flights"]
                      << "\n";
        }
    }
}
