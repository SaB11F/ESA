import serial
import struct
import time
import csv
import os
import psycopg2

# Serial port settings
PORT = "/dev/ttyACM0"
BAUDRATE = 115200

# CSV file path
CSV_DIR = "/home/slogiker/Desktop/CanSat/Data"
CSV_PATH = os.path.join(CSV_DIR, "data.csv")

# PostgreSQL settings (local database)
DB_NAME = "cansat_local"
DB_USER = "pi_user"
DB_PASSWORD = "cansat"
DB_HOST = "localhost"
DB_PORT = "5432"

# Struct format and expected size
FORMAT = '<BffIfffIfffffffffffI'
EXPECTED_SIZE = 77

# CSV headers
HEADERS = [
    "id", "bme_temp", "bme_press", "bme_hum", "bme_gas", "altitude", "dht_temp", "dht_hum",
    "accel_x", "accel_y", "accel_z", "gyro_x", "gyro_y", "gyro_z", "mag_x", "mag_y", "mag_z",
    "latitude", "longitude", "timestamp"
]

def connect_to_db():
    try:
        conn = psycopg2.connect(
            dbname=DB_NAME,
            user=DB_USER,
            password=DB_PASSWORD,
            host=DB_HOST,
            port=DB_PORT
        )
        print("Connected to local PostgreSQL database.")
        return conn
    except psycopg2.Error as e:
        print(f"Database connection error: {e}")
        return None

def read_serial(port=PORT, baudrate=BAUDRATE):
    os.makedirs(CSV_DIR, exist_ok=True)  # Create directory if it doesn’t exist
    conn = connect_to_db()  # Connect to database
    if conn is None:
        return

    try:
        ser = serial.Serial(port, baudrate, timeout=1)
        print(f"Connected to {port} at {baudrate} baud.")
        ser.flush()
        time.sleep(1)  # Stabilize connection

        # Write headers if CSV doesn’t exist
        if not os.path.exists(CSV_PATH):
            with open(CSV_PATH, 'w', newline='') as csvfile:
                writer = csv.writer(csvfile)
                writer.writerow(HEADERS)

        while True:
            if ser.in_waiting >= EXPECTED_SIZE:
                data = ser.read(EXPECTED_SIZE)
                print(f"Raw bytes: {data.hex()}")
                if len(data) == EXPECTED_SIZE:
                    unpacked = struct.unpack(FORMAT, data)
                    id, bme_temp, bme_press, bme_hum, bme_gas, altitude, dht_temp, dht_hum, \
                    accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, mag_x, mag_y, mag_z, \
                    latitude, longitude, timestamp = unpacked

                    # Print the data (for debugging)
                    print(f"ID: {id}")
                    print(f"BME Temp: {bme_temp:.2f} °C")
                    print(f"BME Pressure: {bme_press:.1f} hPa")
                    print(f"BME Humidity: {bme_hum} %")
                    print(f"BME Gas: {bme_gas:.1f} KOhms")
                    print(f"Altitude: {altitude:.1f} m")
                    print(f"DHT Temp: {dht_temp:.2f} °C")
                    print(f"DHT Humidity: {dht_hum} %")
                    print(f"Accel X: {accel_x:.2f} m/s²")
                    print(f"Accel Y: {accel_y:.2f} m/s²")
                    print(f"Accel Z: {accel_z:.2f} m/s²")
                    print(f"Gyro X: {gyro_x:.2f} deg/s")
                    print(f"Gyro Y: {gyro_y:.2f} deg/s")
                    print(f"Gyro Z: {gyro_z:.2f} deg/s")
                    print(f"Mag X: {mag_x:.2f} uT")
                    print(f"Mag Y: {mag_y:.2f} uT")
                    print(f"Mag Z: {mag_z:.2f} uT")
                    print(f"Latitude: {latitude:.6f} deg")
                    print(f"Longitude: {longitude:.6f} deg")
                    print(f"Timestamp: {timestamp}\n")

                    # Write to CSV with formatted floats
                    with open(CSV_PATH, 'a', newline='') as csvfile:
                        writer = csv.writer(csvfile)
                        writer.writerow([
                            id,                  # Integer
                            f"{bme_temp:.2f}",   # Float, 2 decimal places
                            f"{bme_press:.2f}",  # Float, 2 decimal places
                            bme_hum,             # Integer
                            f"{bme_gas:.2f}",    # Float, 2 decimal places
                            f"{altitude:.2f}",   # Float, 2 decimal places
                            f"{dht_temp:.2f}",   # Float, 2 decimal places
                            dht_hum,             # Integer
                            f"{accel_x:.2f}",    # Float, 2 decimal places
                            f"{accel_y:.2f}",    # Float, 2 decimal places
                            f"{accel_z:.2f}",    # Float, 2 decimal places
                            f"{gyro_x:.2f}",     # Float, 2 decimal places
                            f"{gyro_y:.2f}",     # Float, 2 decimal places
                            f"{gyro_z:.2f}",     # Float, 2 decimal places
                            f"{mag_x:.2f}",      # Float, 2 decimal places
                            f"{mag_y:.2f}",      # Float, 2 decimal places
                            f"{mag_z:.2f}",      # Float, 2 decimal places
                            f"{latitude:.2f}",   # Float, 2 decimal places
                            f"{longitude:.2f}",  # Float, 2 decimal places
                            timestamp            # Integer
                        ])

                    # Upload to database (raw values)
                    try:
                        cursor = conn.cursor()
                        insert_query = """
                            INSERT INTO sensor_data (
                                id, bme_temp, bme_press, bme_hum, bme_gas, altitude, dht_temp, dht_hum,
                                accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, mag_x, mag_y, mag_z,
                                latitude, longitude, timestamp
                            ) VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
                        """
                        cursor.execute(insert_query, (
                            id, bme_temp, bme_press, bme_hum, bme_gas, altitude, dht_temp, dht_hum,
                            accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z, mag_x, mag_y, mag_z,
                            latitude, longitude, timestamp
                        ))
                        conn.commit()
                        print("Data uploaded to database.")
                    except psycopg2.Error as e:
                        print(f"Database insert error: {e}")
                        conn.rollback()

                else:
                    print(f"Error: Read {len(data)} bytes, expected {EXPECTED_SIZE}")
            time.sleep(0.1)
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except KeyboardInterrupt:
        print("Exiting...")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("Serial connection closed.")
        if conn:
            conn.close()
            print("Database connection closed.")

if __name__ == "__main__":
    read_serial()