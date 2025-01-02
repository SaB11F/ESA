<?php
// Database connection details
$host = 'localhost';
$dbname = 'cansat';
$user = 'slogiker';
$password = 'Plibersek.1';

// Database query function
function getLatestData() {
    global $host, $dbname, $user, $password;
    
    try {
        // Create PDO instance
        $pdo = new PDO("pgsql:host=$host;dbname=$dbname", $user, $password);
        $pdo->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
        
        // Fetch latest sensor data
        $stmt = $pdo->query("SELECT temperature, humidity, airpressure, batterypower,airquality, 
                             gyro_x, gyro_y, gyro_z, accel_x, accel_y, accel_z 
                             FROM sensor_data 
                             ORDER BY id DESC LIMIT 1");
        $data = $stmt->fetch(PDO::FETCH_ASSOC);
        return $data;
    } catch (PDOException $e) {
        return ['error' => 'Database connection failed: ' . $e->getMessage()];
    }
}

// Return data as JSON
header('Content-Type: application/json');
echo json_encode(getLatestData());

