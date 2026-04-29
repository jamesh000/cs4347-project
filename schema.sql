CREATE DATABASE FlightDB;
USE FlightDB;

CREATE TABLE airports (
       airport_code VARCHAR(3) PRIMARY KEY,
       city VARCHAR(50) NOT NULL,
       state VARCHAR(50),
       name VARCHAR(50)
);

CREATE TABLE airplane_types (
       typename VARCHAR(50) PRIMARY KEY,
       company VARCHAR(50),
       max_seats INT CHECK (max_seats > 0)
);

CREATE TABLE airplanes (
       airplane_id INT PRIMARY KEY,
       total_no_of_seats INT CHECK (total_no_of_seats > 0),
       typename VARCHAR(50),

       FOREIGN KEY (typename) REFERENCES airplane_types(typename)
);

CREATE TABLE flights (
       number VARCHAR(6) PRIMARY KEY,
       airline VARCHAR(50),
       weekdays VARCHAR(7)
);

CREATE TABLE fares (
       code INT,
       number VARCHAR(6),
       amount INT CHECK (amount > 0),
       restriction VARCHAR(50),

       PRIMARY KEY (code, number),

       FOREIGN KEY (number) REFERENCES flights(number)
);

CREATE TABLE flight_legs (
       leg_no INT,
       number VARCHAR(6),
       dep_airport_code VARCHAR(3) NOT NULL,
       scheduled_dep_time TIME NOT NULL,
       arr_airport_code VARCHAR(3) NOT NULL,
       scheduled_arr_time TIME NOT NULL,

       PRIMARY KEY (leg_no, number),
       
       FOREIGN KEY (number) REFERENCES flights(number),
       FOREIGN KEY (dep_airport_code) REFERENCES airports(airport_code),
       FOREIGN KEY (arr_airport_code) REFERENCES airports(airport_code)
);

CREATE TABLE leg_instances (
       date DATE,
       leg_no INT,
       number VARCHAR(6),
       no_of_avail_seats INT CHECK (no_of_avail_seats >= 0),
       airplane_id INT,

       PRIMARY KEY (date, leg_no, number),

       FOREIGN KEY (leg_no, number) REFERENCES flight_legs(leg_no, number),
       FOREIGN KEY (airplane_id) REFERENCES airplanes(airplane_id)
);

CREATE TABLE seats (
       seat_no INT,
       date DATE,
       leg_no INT,
       number VARCHAR(6),
       customer_name VARCHAR(50) NOT NULL,
       cphone VARCHAR(10),

       PRIMARY KEY (seat_no, date, leg_no, number),

       FOREIGN KEY (date, leg_no, number) REFERENCES leg_instances(date, leg_no, number)
);
