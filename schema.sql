CREATE DATABASE FlightDB;
USE FlightDB;

CREATE TABLE airports (
       airport_code INT PRIMARY KEY,
       city VARCHAR(50),
       state VARCHAR(50),
       name VARCHAR(50)
);

CREATE TABLE airplanes (
       airplane_id INT PRIMARY KEY,
       total_no_of_seats INT CHECK (total_no_of_seats > 0)
);

CREATE TABLE airplane_types (
       typename VARCHAR(50) PRIMARY KEY,
       company VARCHAR(50),
       max_seats INT CHECK (max_seats > 0)
);

CREATE TABLE flights (
       number INT PRIMARY KEY,
       airline VARCHAR(50),
       weekdays VARCHAR(9)
);

CREATE TABLE fares (
       code INT,
       number INT,
       amount INT CHECK (amount > 0),
       restriction VARCHAR(50),

       PRIMARY KEY (code, number),

       FOREIGN KEY (number) REFERENCES flights(number)
);

CREATE TABLE flight_legs (
       leg_no INT PRIMARY KEY,
       number INT,
       scheduled_dep_time TIME,
       scheduled_arr_time TIME,

       FOREIGN KEY (number) REFERENCES flights(number)
);

CREATE TABLE leg_instances (
       date DATETIME,
       leg_no INT,
       no_of_avail_seats INT CHECK (no_of_avail_seats >= 0),
       airplane_id INT,

       PRIMARY KEY (date, leg_no),

       FOREIGN KEY (leg_no) REFERENCES flight_legs(leg_no),
       FOREIGN KEY (airplane_id) REFERENCES airplanes(airplane_id)
);

CREATE TABLE seats (
       seat_no INT,
       date DATETIME,
       leg_no INT,
       customer_name VARCHAR(50) NOT NULL,
       cphone VARCHAR(10),

       PRIMARY KEY (seat_no, date, leg_no),

       FOREIGN KEY (date, leg_no) REFERENCES leg_instances(date, leg_no)
);
