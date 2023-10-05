import serial
import csv
import time
import os, time
from influxdb_client_3 import InfluxDBClient3, Point
import datetime

INFLUXDB_TOKEN = "" # InfluxDB token
org = "WM" # InfluxDB organization
host = "https://eu-central-1-1.aws.cloud2.influxdata.com" # InfluxDB host
database = "hydrotower" # InfluxDB database

client = InfluxDBClient3(host=host, token=INFLUXDB_TOKEN, org=org)

# Define the serial port and baud rate (adjust as needed)
serial_port = "" # Serial port
baud_rate = 57600

uv_scala = ["Niedrig", "Mittel", "Hoch", "Sehr hoch", "Extrem"]
water_distance_sensor_max_height = 6  # cm
water_box_max_height = 45  # cm
water_max_volume = 25  # l

# Open the serial port
ser = serial.Serial(serial_port, baud_rate)

# Define the CSV file to save the data
csv_file_name = "arduino_data.csv"

# Create a CSV writer
with open(csv_file_name, mode="w", newline="") as file:
    writer = csv.writer(file)

    # Only Write header row if file is empty
    if os.stat(csv_file_name).st_size == 0:
        writer.writerow(["Time", "Temperature", "UVS", "UVS_readable", "Ultrasonic", "Water"])

    try:
        entry = {"Time": "", "Temperature": "", "UVS": "", "UVS_readable": "", "Ultrasonic": "", "Water": ""}

        while True:
            # Read data from the Arduino
            data = ser.readline().decode().strip()
            print(f"Received data: {data}")

            # Split the data into key-value pairs
            try:
                key, value = data.split(" = ")
                if key not in entry.keys():
                    print(f"Key {key} not in entry")
                    continue
                entry[key] = value
            except ValueError:
                print("ValueError on: ", data)
                continue

            if key == "UVS":
                entry["UVS_readable"] = uv_scala[int(value)]
            
            if key == "Ultrasonic":
                value = float(value)
                # Adjust for the distance from the sensor to the bottom of the water box
                adjusted_value = value - water_distance_sensor_max_height
                if adjusted_value > water_box_max_height:
                    entry["Water"] = 0
                elif adjusted_value < 0:
                    entry["Water"] = water_max_volume
                else:
                    # Calculate the water height in cm as the distance from the sensor to the water box minus the distance from the sensor to the bottom of the water box
                    water_height = water_box_max_height - adjusted_value 
                    entry["Water"] = round(water_height * water_max_volume / water_box_max_height, 3)

            if key == "Time":
                # convert the timestamp to a datetime object using cest time
                dt_object = datetime.datetime.fromtimestamp(int(value), tz=datetime.timezone.utc)
                entry[key] = dt_object.isoformat() 
                    
            # Check if all values are received
            if all(value != "" for value in entry.values()):
                print(entry)
                # Write the data to the CSV file
                writer.writerow(
                    [
                        entry["Time"],
                        entry["Temperature"],
                        entry["UVS"],
                        entry["UVS_readable"],
                        entry["Ultrasonic"],
                        entry["Water"],
                    ]
                )
                file.flush()

                point = (
                    Point("arduino_data")
                    .field("Temperature", float(entry["Temperature"]))
                    .field("UVS", float(entry["UVS"]))
                    .field("Ultrasonic", float(entry["Ultrasonic"]))
                    .field("UVS_readable", str(entry["UVS_readable"]))
                    .field("Water", float(entry["Water"]))
                    .time(entry["Time"], write_precision="s")
                )

                # Write the data point to InfluxDB
                client.write(database=database, record=point)

                # Reset the entry dictionary
                entry = {"Time": "", "Temperature": "", "UVS": "", "UVS_readable": "", "Ultrasonic": "", "Water": ""}
    except KeyboardInterrupt:
        # Close the serial port when interrupted
        ser.close()
        print("Data logging stopped.")
