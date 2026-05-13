#include "availability.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include "imgui.h"

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

void checkAvailability(DB& db, const std::string& number,
                       const std::string leg_no,
                       const std::string& date)
{
    const std::string flightNumber = db.escape(number);
    const std::string flightDate   = db.escape(date);

    std::string instanceSQL =
        "SELECT li.number, li.leg_no, li.date, li.no_of_avail_seats, "
        "       li.airplane_id, a.total_no_of_seats, a.typename, "
        "       fl.dep_airport_code, fl.arr_airport_code, "
        "       fl.scheduled_dep_time, fl.scheduled_arr_time "
        "FROM leg_instances li "
        "JOIN flight_legs fl ON fl.number = li.number AND fl.leg_no = li.leg_no "
        "LEFT JOIN airplanes a ON a.airplane_id = li.airplane_id "
        "WHERE UPPER(li.number) = UPPER('" + flightNumber + "') "
        "  AND li.leg_no = " + leg_no + " "
        "  AND li.date = '" + flightDate + "'";

    auto instance = db.query(instanceSQL);

    if (instance.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f),
            "No leg instance found for flight %s, leg %s, date %s.",
            number.c_str(), leg_no.c_str(), date.c_str());
        return;
    }

    const Row& r = instance.front();

    std::string seatsSQL =
        "SELECT seat_no, customer_name, cphone "
        "FROM seats "
        "WHERE UPPER(number) = UPPER('" + flightNumber + "') "
        "  AND leg_no = " + leg_no + " "
        "  AND date = '" + flightDate + "' "
        "ORDER BY seat_no";

    auto bookedSeats = db.query(seatsSQL);

    // ============================================================
    // Header
    // ============================================================

    ImGui::SeparatorText("Availability");

    ImGui::Text("Flight: %s", r.at("number").c_str());
    ImGui::SameLine();
    ImGui::Text("| Leg: %s", r.at("leg_no").c_str());
    ImGui::SameLine();
    ImGui::Text("| Date: %s", r.at("date").c_str());

    ImGui::Text("Route: %s -> %s",
        r.at("dep_airport_code").c_str(),
        r.at("arr_airport_code").c_str());

    ImGui::Text("Departure: %s",
        r.at("scheduled_dep_time").c_str());

    ImGui::Text("Arrival: %s",
        r.at("scheduled_arr_time").c_str());

    ImGui::Text("Plane: %s",
        r.at("airplane_id").c_str());

    ImGui::Text("Type: %s",
        r.at("typename").c_str());

    ImGui::Text("Total Seats: %s",
        r.at("total_no_of_seats").c_str());

    ImGui::TextColored(
        ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
        "Available Seats: %s",
        r.at("no_of_avail_seats").c_str()
    );

    ImGui::TextColored(
        ImVec4(1.0f, 1.0f, 0.2f, 1.0f),
        "Booked Seat Records: %d",
        static_cast<int>(bookedSeats.size())
    );

    ImGui::Spacing();

    // ============================================================
    // Booked Seats Table
    // ============================================================

    if (bookedSeats.empty()) {
        ImGui::TextDisabled("No booked seat rows found for this leg instance.");
        return;
    }

    ImGui::SeparatorText("Booked Seats");

    if (ImGui::BeginTable("BookedSeatsTable", 3,
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Seat");
        ImGui::TableSetupColumn("Customer");
        ImGui::TableSetupColumn("Phone");
        ImGui::TableHeadersRow();

        for (const Row& seat : bookedSeats) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", seat.at("seat_no").c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", seat.at("customer_name").c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", seat.at("cphone").c_str());
        }

        ImGui::EndTable();
    }
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

void available(DB& db, const std::string& number, const std::string leg_no, const std::string& date) {
    checkAvailability(db, number, leg_no, date);
}

void available(DB& db, const std::string& number, int leg_no, const std::string& date, int seat_no) {
    checkSeatAvailability(db, number, leg_no, date, seat_no);
}
