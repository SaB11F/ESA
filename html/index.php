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
        $stmt = $pdo->query("SELECT temperature, humidity, airpressure, batterypower, airquality, 
                             gyro_x, gyro_y, gyro_z, accel_x, accel_y, accel_z 
                             FROM sensor_data 
                             ORDER BY id DESC LIMIT 1");
        $data = $stmt->fetch(PDO::FETCH_ASSOC);
        return $data;
    } catch (PDOException $e) {
        return ['error' => 'Database connection failed: ' . $e->getMessage()];
    }
}

$data = getLatestData();
?>

<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" type="text/css" href="style.css">
    <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css">
    <title>CanSat Dashboard</title>
</head>
<body>
    <div id="overlay" class="overlay" onclick="closeSidebar()"></div>
    <div id="sidebar" class="sidebar">
        <a href="javascript:void(0)" class="close-btn" onclick="closeSidebar()">
            <i class="fa fa-caret-left"></i>
        </a>
        <a onclick="openTerminal()">
            <i class="fas fa-terminal">&nbsp;</i> Terminal
        </a>
        <a onclick="sendCommand('sos')">S.O.S</a>
        <a onclick="sendCommand('rick_roll')">RickRoll</a>
        <a onclick="openCamera()">
            <i class="fa fa-camera">&nbsp;</i> Camera
        </a>
        <a href="ai.html">Chat with AI</a>
    </div>

    <div class="header">
        <i class="fa fa-navicon icon" onclick="openSidebar()"></i>
        <h1>CanSat Dashboard</h1>
    </div>

    <div class="dashboard">
        <div class="card">
            <div class="card-title">Temperature</div>
            <div class="card-value" id="temperature">-- °C</div>
        </div>
        <div class="card">
            <div class="card-title">Humidity</div>
            <div class="card-value" id="humidity">-- %</div>
        </div>
        <div class="card">
            <div class="card-title">Air Pressure</div>
            <div class="card-value" id="airPressure">-- hPa</div>
        </div>
        <div class="card">
            <div class="card-title">Battery Power</div>
            <div class="card-value" id="batteryPower">-- %</div>
        </div>
        <div class="card">
            <div class="card-title">Air Quality</div>
            <div class="card-value" id="airQuality">-- ppm</div>
        </div>
        <div class="card">
            <div class="card-title">placeholder</div>
            <div class="card-value" id=""></div>
        </div>
        <div class="card big-card">
            <div class="card-title">Live Gyroscope</div>
            <div class="card-value" id="gyro">
                <span>X: --</span>
                <span>Y: --</span>
                <span>Z: --</span>
            </div>
        </div>
        <div class="card big-card">
            <div class="card-title">Live Acceleration</div>
            <div class="card-value" id="accel">
                <span>X: --</span>
                <span>Y: --</span>
                <span>Z: --</span>
            </div>
        </div>
    </div>

    <div class="controls">
        <button class="button" onclick="sendCommand('buzz')">Buzz</button>
        <button class="button" onclick="sendCommand('stand_up')">Stand Up</button>
    </div>

    <script>
        function sendCommand(command) {
            console.log(`Command sent: ${command}`);
        }

        function openSidebar() {
            document.getElementById("sidebar").style.width = "250px";
            document.getElementById("overlay").style.display = "block";
        }

        function closeSidebar() {
            document.getElementById("sidebar").style.width = "0";
            document.getElementById("overlay").style.display = "none";
        }

        function openTerminal() {
            console.log("terminal open");
        }

        function openCamera() {
            console.log("Camera open");
        }

        function fetchData() {
            fetch('get_data.php')
                .then(response => response.json())
                .then(data => {
                    // Update the values on the page
                    document.getElementById('temperature').textContent = data.temperature + ' °C';
                    document.getElementById('humidity').textContent = data.humidity + ' %';
                    document.getElementById('airPressure').textContent = data.airpressure + ' hPa';
                    document.getElementById('batteryPower').textContent = data.batterypower + ' %';
                    document.getElementById('airQuality').textContent = data.airquality + ' ppm';
                    document.getElementById('gyro').textContent = `X: ${data.gyro_x} Y: ${data.gyro_y} Z: ${data.gyro_z}`;
                    document.getElementById('accel').textContent = `X: ${data.accel_x} Y: ${data.accel_y} Z: ${data.accel_z}`;
                })
                .catch(error => console.error('Error fetching data:', error));
        }

        // Fetch data every second
        setInterval(fetchData, 1000);
    </script>
</body>
</html>



