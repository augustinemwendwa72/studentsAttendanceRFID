# Student Attendance Management System

A complete student attendance management system with RFID card integration, built with Node.js, Express, and vanilla JavaScript.

## Features

### Web Interface
- 📋 **Attendance List** - View daily attendance with filtering options
- 👥 **Registered Students** - View all registered students
- ➕ **Register Students** - Add new students with admission number, RFID card, parent phone, and name

### Functionality
- 🔍 **Search** - Search by name, admission number, or RFID card number
- 📅 **Filter by Date** - View attendance for specific dates
- ✅ **Filter by Status** - Filter by Present, Absent, or Late
- ⏰ **Time Filtering** - Filter by time in/time out ranges
- 📥 **Export to CSV** - Download attendance data
- 📤 **Import from CSV** - Upload CSV files to add attendance records
- 🤖 **RFID Integration** - Automatic attendance marking via RFID card scan

### Arduino/ESP8266 Integration
- Works with Arduino Mega + ESP8266 or standalone ESP8266
- Sends GET request to server when RFID card is scanned
- Server responds with parent phone number and success/acknowledgment message

## Project Structure

```
newlib/
├── package.json              # Node.js dependencies
├── server.js                 # Express server with API endpoints
├── README.md                 # This file
├── public/
│   ├── index.html           # Frontend web interface
│   └── data/
│       ├── students.csv     # Student database
│       └── attendance.csv   # Attendance records
└── arduino_rfid_attendance/
    ├── arduino_rfid_attendance.ino          # Arduino Mega + ESP8266 version
    └── esp8266_rfid_standalone/
        └── esp8266_rfid_standalone.ino     # Standalone ESP8266 version
```

## Installation

1. **Install Node.js dependencies:**
   ```bash
   npm install
   ```

2. **Configure WiFi in Arduino code:**
   - Open `arduino_rfid_attendance/esp8266_rfid_standalone/esp8266_rfid_standalone.ino`
   - Replace `YOUR_WIFI_SSID` with your WiFi network name
   - Replace `YOUR_WIFI_PASSWORD` with your WiFi password
   - Replace `192.168.1.100` with your server's IP address

3. **Start the server:**
   ```bash
   npm start
   ```

4. **Open the web interface:**
   - Navigate to `http://localhost:3000` in your browser

## Arduino/RFID Hardware Setup

### Option 1: Standalone ESP8266 (Wemos D1 R2)
```
RFID RC522    →    Wemos D1 R2
--------------------------------
RST (RST_PIN) →    D1 (GPIO5)
SDA (SS_PIN)  →    D2 (GPIO4)
MOSI          →    D7 (GPIO13)
MISO          →    D6 (GPIO12)
SCK           →    D5 (GPIO14)
3.3V          →    3.3V
GND           →    GND
```

### Option 2: Arduino Mega + ESP8266
```
RFID RC522    →    Arduino Mega
--------------------------------
RST           →    Pin 5
SDA           →    Pin 53
MOSI          →    Pin 51
MISO          →    Pin 50
SCK           →    Pin 52
3.3V          →    3.3V
GND           →    GND

ESP8266       →    Arduino Mega
--------------------------------
TX            →    Pin 10 (RX)
RX            →    Pin 11 (TX)
3.3V          →    3.3V
GND           →    GND
```

## API Endpoints

### Students
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/students` | Get all students |
| POST | `/api/students` | Register new student |
| GET | `/api/students/rfid/:rfid` | Get student by RFID |
| DELETE | `/api/students/:admission_number` | Delete student |

### Attendance
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/attendance` | Get all attendance (with filters) |
| POST | `/api/attendance` | Mark attendance manually |
| GET | `/api/attendance/scan/:rfid` | RFID scan endpoint |
| DELETE | `/api/attendance/:admission_number/:date` | Delete attendance record |

### Filter Parameters
- `date` - Filter by specific date
- `status` - Filter by status (present, absent, late)
- `search` - Search by name or admission number
- `rfid` - Filter by RFID card number
- `time_in_start`, `time_in_end` - Time in range
- `time_out_start`, `time_out_end` - Time out range

### Import/Export
| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/attendance/export` | Export attendance to CSV |
| POST | `/api/attendance/import` | Import attendance from CSV |

## CSV File Formats

### Students CSV
```csv
admission_number,rfid_card_number,parent_phone,name
STU001,A1B2C3D4,+254700000001,John Smith
```

### Attendance CSV
```csv
admission_number,name,rfid_card_number,date,time_in,time_out,status
STU001,John Smith,A1B2C3D4,2026-03-03,07:45:00,14:30:00,present
```

## Configuration

### Late Threshold
In `server.js`, modify the late threshold time:
```javascript
const LATE_THRESHOLD = '08:00:00'; // Students arriving after 8:00 AM are marked as late
```

## Usage

1. **Register a Student:**
   - Go to "Register Student" tab
   - Fill in admission number, name, RFID card number, and parent's phone
   - Click "Register Student"

2. **Mark Attendance via RFID:**
   - When a student scans their RFID card at the scanner
   - The system records their attendance with time in
   - The response includes the parent's phone number

3. **View Attendance:**
   - Go to "Attendance List" tab
   - Use filters to narrow down results
   - Export or import data as needed

4. **Search:**
   - Use the search box to find students by name, admission number, or RFID

## Troubleshooting

1. **Server won't start:**
   - Make sure Node.js is installed
   - Run `npm install` to install dependencies

2. **RFID not working:**
   - Check wiring connections
   - Verify correct pin numbers in code
   - Make sure RFID library is installed in Arduino IDE

3. **ESP8266 can't connect to WiFi:**
   - Verify WiFi credentials
   - Check that the server IP is reachable
   - Make sure ESP8266 is powered with sufficient current

4. **Attendance not marking:**
   - Check server is running
   - Verify RFID card is registered in the system
   - Check browser console for errors

## License

MIT License
