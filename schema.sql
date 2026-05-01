CREATE DATABASE FlightDB2;
USE FlightDB2;

CREATE TABLE airports (
       airport_code VARCHAR(3) PRIMARY KEY,
       city VARCHAR(50) NOT NULL,
       state VARCHAR(50),
       name VARCHAR(100)
);

CREATE TABLE airplane_types (
       typename VARCHAR(50) PRIMARY KEY,
       company VARCHAR(50),
       max_seats INT CHECK (max_seats > 0)
);

CREATE TABLE airplanes (
       airplane_id VARCHAR(15) PRIMARY KEY,
       total_no_of_seats INT CHECK (total_no_of_seats > 0),
       typename VARCHAR(50), -- updated typename from int to varchar--

       FOREIGN KEY (typename) REFERENCES airplane_types(typename)
);

CREATE TABLE flights (
       number VARCHAR(6) PRIMARY KEY,
       airline VARCHAR(50),
       weekdays VARCHAR(7)
);

CREATE TABLE fares (
       number VARCHAR(6),
       code VARCHAR(15),  -- updated code to varchar --
       amount INT CHECK (amount > 0),
       restriction VARCHAR(50),

       PRIMARY KEY (code, number), 

       FOREIGN KEY (number) REFERENCES flights(number)
);

CREATE TABLE flight_legs (
       number VARCHAR(6), -- moved number to match csv -- 
       leg_no INT,
       dep_airport_code VARCHAR(3) NOT NULL,
       arr_airport_code VARCHAR(3) NOT NULL,
       scheduled_dep_time TIME NOT NULL,
       scheduled_arr_time TIME NOT NULL,

       PRIMARY KEY (leg_no, number),
       
       FOREIGN KEY (number) REFERENCES flights(number),
       FOREIGN KEY (dep_airport_code) REFERENCES airports(airport_code),
       FOREIGN KEY (arr_airport_code) REFERENCES airports(airport_code)
);

CREATE TABLE leg_instances (
       number VARCHAR(6), -- moved number adjusted for csv --
       leg_no INT, -- moved leg_no for csv --
       date DATE,
       no_of_avail_seats INT CHECK (no_of_avail_seats >= 0),
       airplane_id VARCHAR(15),

       PRIMARY KEY (date, leg_no, number),

       FOREIGN KEY (leg_no, number) REFERENCES flight_legs(leg_no, number),
       FOREIGN KEY (airplane_id) REFERENCES airplanes(airplane_id)
);

CREATE TABLE seats (
       number VARCHAR(6),
       leg_no INT,
       date DATE,
       seat_no INT,
       customer_name VARCHAR(50) NOT NULL,
       cphone VARCHAR(10),

       PRIMARY KEY (seat_no, date, leg_no, number),

       FOREIGN KEY (date, leg_no, number) REFERENCES leg_instances(date, leg_no, number),
       FOREIGN KEY (airplane_id) REFERENCES airplanes(airplane_id)
);


-- Load CSV files
LOAD DATA LOCAL INFILE 'AIRPORT.csv' 
INTO TABLE airports 
FIELDS TERMINATED BY ',' 
ENCLOSED BY '"' 
LINES TERMINATED BY '\n'
IGNORE 1 ROWS;
-- (Airport_code, Name, City, State); This is technically correct but probably breaks the code

LOAD DATA LOCAL INFILE 'AIRPLANE_TYPE.csv'
INTO TABLE airplane_types
FIELDS TERMINATED BY ',' 
ENCLOSED BY '"' 
LINES TERMINATED BY '\n' 
IGNORE 1 ROWS;

LOAD DATA LOCAL INFILE 'AIRPLANE.csv' 
INTO TABLE airplanes 
FIELDS TERMINATED BY ',' 
ENCLOSED BY '"' 
LINES TERMINATED BY '\n' 
IGNORE 1 ROWS;

LOAD DATA LOCAL INFILE 'FLIGHT.csv'
INTO TABLE flights
FIELDS TERMINATED BY ',' 
ENCLOSED BY '"' 
LINES TERMINATED BY '\n' 
IGNORE 1 ROWS;

LOAD DATA LOCAL INFILE 'FLIGHT_LEG.csv'
INTO TABLE flight_legs
FIELDS TERMINATED BY ',' 
ENCLOSED BY '"' 
LINES TERMINATED BY '\n' 
IGNORE 1 ROWS;

LOAD DATA LOCAL INFILE 'LEG_INSTANCE.csv' 
INTO TABLE leg_instances
FIELDS TERMINATED BY ',' 
LINES TERMINATED BY '\n'
IGNORE 1 ROWS 
(number, leg_no, date, no_of_avail_seats, airplane_id, @dummy1, @dummy2);

LOAD DATA LOCAL INFILE 'SEAT.csv' 
INTO TABLE seats 
FIELDS TERMINATED BY ',' 
LINES TERMINATED BY '\n' 
IGNORE 1 ROWS 
(airplane_id, seat_no, code);

LOAD DATA LOCAL INFILE 'FARE.csv'
INTO TABLE fares
FIELDS TERMINATED BY ',' 
ENCLOSED BY '"' 
LINES TERMINATED BY '\n' 
IGNORE 1 ROWS;
