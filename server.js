const express = require('express');
const fs = require('fs');
const path = require('path');
const csv = require('csv-parser');
const createCsvWriter = require('csv-writer');
const cors = require('cors');
const bodyParser = require('body-parser');

const app = express();
const PORT = process.env.PORT || 3100;

// Middleware
app.use(cors());
app.use(bodyParser.json());
app.use(express.static('public'));

// File paths
const STUDENTS_FILE = path.join(__dirname, 'public/data/students.csv');
const ATTENDANCE_FILE = path.join(__dirname, 'public/data/attendance.csv');

// Configuration
const LATE_THRESHOLD = '08:00:00'; // Students arriving after 8:00 AM are marked as late

// Initialize CSV files if they don't exist
function initializeCSVFiles() {
    // Students CSV
    if (!fs.existsSync(STUDENTS_FILE)) {
        const header = 'admission_number,rfid_card_number,parent_phone,name\n';
        fs.writeFileSync(STUDENTS_FILE, header);
    }

    // Attendance CSV
    if (!fs.existsSync(ATTENDANCE_FILE)) {
        const header = 'admission_number,name,rfid_card_number,date,time_in,time_out,status\n';
        fs.writeFileSync(ATTENDANCE_FILE, header);
    }
}

initializeCSVFiles();

// Helper function to append to CSV
function appendToCSV(filePath, data, headers) {
    const csvWriter = createCsvWriter.createObjectCsvStringifier({
        header: headers
    });
    
    const existingData = fs.readFileSync(filePath, 'utf8');
    const records = existingData.split('\n').filter(line => line.trim());
    
    const newData = csvWriter.stringifyRecords(data);
    const finalData = records.join('\n') + '\n' + newData;
    
    fs.writeFileSync(filePath, finalData);
}

// Helper function to rewrite entire CSV
function rewriteCSV(filePath, data, headers) {
    const csvWriter = createCsvWriter.createObjectCsvStringifier({
        header: headers
    });
    
    const headerRow = headers.join(',') + '\n';
    const dataRows = csvWriter.stringifyRecords(data);
    
    fs.writeFileSync(filePath, headerRow + dataRows);
}

// ==================== STUDENTS API ====================

// Get all students
app.get('/api/students', (req, res) => {
    const students = [];
    fs.createReadStream(STUDENTS_FILE)
        .pipe(csv())
        .on('data', (row) => students.push(row))
        .on('end', () => {
            res.json(students);
        })
        .on('error', (err) => {
            res.status(500).json({ error: err.message });
        });
});

// Register new student
app.post('/api/students', (req, res) => {
    const { admission_number, rfid_card_number, parent_phone, name } = req.body;

    if (!admission_number || !rfid_card_number || !parent_phone || !name) {
        return res.status(400).json({ error: 'All fields are required' });
    }

    // Check if student already exists
    const students = [];
    fs.createReadStream(STUDENTS_FILE)
        .pipe(csv())
        .on('data', (row) => students.push(row))
        .on('end', () => {
            const exists = students.find(s => s.admission_number === admission_number || s.rfid_card_number === rfid_card_number);
            if (exists) {
                return res.status(400).json({ error: 'Student with this admission number or RFID card already exists' });
            }

            const newStudent = [{ admission_number, rfid_card_number, parent_phone, name }];
            const headers = ['admission_number', 'rfid_card_number', 'parent_phone', 'name'];
            
            appendToCSV(STUDENTS_FILE, newStudent, headers);
            
            res.json({ success: true, message: 'Student registered successfully', student: { admission_number, rfid_card_number, parent_phone, name } });
        });
});

// Get student by RFID card number
app.get('/api/students/rfid/:rfid', (req, res) => {
    const rfid = req.params.rfid;
    const students = [];

    fs.createReadStream(STUDENTS_FILE)
        .pipe(csv())
        .on('data', (row) => students.push(row))
        .on('end', () => {
            const student = students.find(s => s.rfid_card_number === rfid);
            if (!student) {
                return res.status(404).json({ error: 'Student not found' });
            }
            res.json(student);
        })
        .on('error', (err) => {
            res.status(500).json({ error: err.message });
        });
});

// Delete student
app.delete('/api/students/:admission_number', (req, res) => {
    const admissionNumber = req.params.admission_number;
    const students = [];

    fs.createReadStream(STUDENTS_FILE)
        .pipe(csv())
        .on('data', (row) => students.push(row))
        .on('end', () => {
            const filtered = students.filter(s => s.admission_number !== admissionNumber);
            
            if (filtered.length === students.length) {
                return res.status(404).json({ error: 'Student not found' });
            }

            const headers = ['admission_number', 'rfid_card_number', 'parent_phone', 'name'];
            rewriteCSV(STUDENTS_FILE, filtered, headers);
            
            res.json({ success: true, message: 'Student deleted successfully' });
        });
});

// ==================== ATTENDANCE API ====================

// Get all attendance records
app.get('/api/attendance', (req, res) => {
    const { date, status, search, rfid, time_in_start, time_in_end, time_out_start, time_out_end } = req.query;
    const attendance = [];

    fs.createReadStream(ATTENDANCE_FILE)
        .pipe(csv())
        .on('data', (row) => attendance.push(row))
        .on('end', () => {
            let filtered = [...attendance];

            // Filter by date
            if (date) {
                filtered = filtered.filter(r => r.date === date);
            }

            // Filter by status
            if (status) {
                filtered = filtered.filter(r => r.status === status);
            }

            // Filter by search (name or admission number)
            if (search) {
                const searchLower = search.toLowerCase();
                filtered = filtered.filter(r => 
                    r.name.toLowerCase().includes(searchLower) || 
                    r.admission_number.toLowerCase().includes(searchLower)
                );
            }

            // Filter by RFID
            if (rfid) {
                filtered = filtered.filter(r => r.rfid_card_number === rfid);
            }

            // Filter by time in range
            if (time_in_start) {
                filtered = filtered.filter(r => r.time_in >= time_in_start);
            }
            if (time_in_end) {
                filtered = filtered.filter(r => r.time_in <= time_in_end);
            }

            // Filter by time out range
            if (time_out_start) {
                filtered = filtered.filter(r => r.time_out && r.time_out >= time_out_start);
            }
            if (time_out_end) {
                filtered = filtered.filter(r => r.time_out && r.time_out <= time_out_end);
            }

            res.json(filtered);
        })
        .on('error', (err) => {
            res.status(500).json({ error: err.message });
        });
});

// Mark attendance via RFID scan (called by ESP8266)
app.get('/api/attendance/scan/:rfid', (req, res) => {
    const rfid = req.params.rfid;
    const today = new Date().toISOString().split('T')[0];
    const currentTime = new Date().toTimeString().split(' ')[0];

    console.log(`RFID Scan: ${rfid} at ${currentTime} on ${today}`);

    // First, find the student
    const students = [];
    
    fs.createReadStream(STUDENTS_FILE)
        .pipe(csv())
        .on('data', (row) => students.push(row))
        .on('end', () => {
            const student = students.find(s => s.rfid_card_number === rfid);
            
            if (!student) {
                return res.status(404).json({ 
                    success: false, 
                    message: 'Student not found',
                    parent_phone: ''
                });
            }

            console.log(`Student found: ${student.name}`);

            // Now check attendance for today
            const attendance = [];
            fs.createReadStream(ATTENDANCE_FILE)
                .pipe(csv())
                .on('data', (row) => attendance.push(row))
                .on('end', () => {
                    // Check if student already has attendance for today
                    const todayRecord = attendance.find(r => 
                        r.rfid_card_number === rfid && r.date === today
                    );

                    if (todayRecord) {
                        // If already present, mark time out
                        if (todayRecord.time_out === '' || !todayRecord.time_out) {
                            // Update all records
                            attendance.forEach(r => {
                                if (r.rfid_card_number === rfid && r.date === today) {
                                    r.time_out = currentTime;
                                }
                            });

                            const headers = ['admission_number', 'name', 'rfid_card_number', 'date', 'time_in', 'time_out', 'status'];
                            rewriteCSV(ATTENDANCE_FILE, attendance, headers);

                            res.json({ 
                                success: true, 
                                message: 'Time out recorded',
                                student_name: student.name,
                                parent_phone: student.parent_phone
                            });
                        } else {
                            res.json({ 
                                success: true, 
                                message: 'Already checked out',
                                student_name: student.name,
                                parent_phone: student.parent_phone
                            });
                        }
                    } else {
                        // New attendance record - mark time in
                        // Determine status based on time
                        let status = 'present';
                        if (currentTime > LATE_THRESHOLD) {
                            status = 'late';
                        }

                        const newRecord = {
                            admission_number: student.admission_number,
                            name: student.name,
                            rfid_card_number: student.rfid_card_number,
                            date: today,
                            time_in: currentTime,
                            time_out: '',
                            status: status
                        };

                        const headers = ['admission_number', 'name', 'rfid_card_number', 'date', 'time_in', 'time_out', 'status'];
                        appendToCSV(ATTENDANCE_FILE, [newRecord], headers);

                        res.json({ 
                            success: true, 
                            message: status === 'late' ? 'Late arrival recorded' : 'Attendance marked successfully',
                            student_name: student.name,
                            parent_phone: student.parent_phone,
                            status: status
                        });
                    }
                });
        });
});

// Manual attendance marking
app.post('/api/attendance', (req, res) => {
    const { admission_number, status, date } = req.body;

    const students = [];
    fs.createReadStream(STUDENTS_FILE)
        .pipe(csv())
        .on('data', (row) => students.push(row))
        .on('end', () => {
            const student = students.find(s => s.admission_number === admission_number);
            if (!student) {
                return res.status(404).json({ error: 'Student not found' });
            }

            const attendance = [];
            const targetDate = date || new Date().toISOString().split('T')[0];
            const currentTime = new Date().toTimeString().split(' ')[0];

            fs.createReadStream(ATTENDANCE_FILE)
                .pipe(csv())
                .on('data', (row) => attendance.push(row))
                .on('end', () => {
                    const existingRecord = attendance.find(r => 
                        r.admission_number === admission_number && r.date === targetDate
                    );

                    if (existingRecord) {
                        return res.status(400).json({ error: 'Attendance already marked for this date' });
                    }

                    const newRecord = {
                        admission_number: student.admission_number,
                        name: student.name,
                        rfid_card_number: student.rfid_card_number,
                        date: targetDate,
                        time_in: status === 'absent' ? '' : currentTime,
                        time_out: '',
                        status: status
                    };

                    const headers = ['admission_number', 'name', 'rfid_card_number', 'date', 'time_in', 'time_out', 'status'];
                    appendToCSV(ATTENDANCE_FILE, [newRecord], headers);

                    res.json({ success: true, message: 'Attendance marked successfully' });
                });
        });
});

// Delete attendance record
app.delete('/api/attendance/:admission_number/:date', (req, res) => {
    const { admission_number, date } = req.params;
    const attendance = [];

    fs.createReadStream(ATTENDANCE_FILE)
        .pipe(csv())
        .on('data', (row) => attendance.push(row))
        .on('end', () => {
            const filtered = attendance.filter(r => 
                !(r.admission_number === admission_number && r.date === date)
            );
            
            if (filtered.length === attendance.length) {
                return res.status(404).json({ error: 'Attendance record not found' });
            }

            const headers = ['admission_number', 'name', 'rfid_card_number', 'date', 'time_in', 'time_out', 'status'];
            rewriteCSV(ATTENDANCE_FILE, filtered, headers);

            res.json({ success: true, message: 'Attendance record deleted successfully' });
        });
});

// ==================== IMPORT/EXPORT API ====================

// Export attendance to CSV
app.get('/api/attendance/export', (req, res) => {
    const attendance = [];
    fs.createReadStream(ATTENDANCE_FILE)
        .pipe(csv())
        .on('data', (row) => attendance.push(row))
        .on('end', () => {
            const csvWriter = createCsvWriter.createObjectCsvStringifier({
                header: ['admission_number', 'name', 'rfid_card_number', 'date', 'time_in', 'time_out', 'status']
            });
            
            const csvContent = csvWriter.stringifyRecords(attendance);
            res.setHeader('Content-Type', 'text/csv');
            res.setHeader('Content-Disposition', 'attachment; filename=attendance_export.csv');
            res.send(csvContent);
        })
        .on('error', (err) => {
            res.status(500).json({ error: err.message });
        });
});

// Import attendance from CSV
app.post('/api/attendance/import', (req, res) => {
    const { csvData } = req.body;

    if (!csvData) {
        return res.status(400).json({ error: 'No CSV data provided' });
    }

    const lines = csvData.split('\n').filter(line => line.trim());
    
    // Read existing attendance
    const existingAttendance = [];
    fs.createReadStream(ATTENDANCE_FILE)
        .pipe(csv())
        .on('data', (row) => existingAttendance.push(row))
        .on('end', () => {
            // Parse new records
            const newRecords = [];
            for (let i = 1; i < lines.length; i++) {
                const values = lines[i].split(',');
                if (values.length >= 7) {
                    const record = {
                        admission_number: values[0].trim(),
                        name: values[1].trim(),
                        rfid_card_number: values[2].trim(),
                        date: values[3].trim(),
                        time_in: values[4].trim(),
                        time_out: values[5].trim(),
                        status: values[6].trim()
                    };
                    
                    // Check if record already exists
                    const exists = existingAttendance.find(r => 
                        r.admission_number === record.admission_number && r.date === record.date
                    );
                    
                    if (!exists) {
                        newRecords.push(record);
                    }
                }
            }

            if (newRecords.length === 0) {
                return res.json({ success: true, message: 'No new records to import' });
            }

            const headers = ['admission_number', 'name', 'rfid_card_number', 'date', 'time_in', 'time_out', 'status'];
            appendToCSV(ATTENDANCE_FILE, newRecords, headers);

            res.json({ success: true, message: `Imported ${newRecords.length} records successfully` });
        });
});

// ==================== SEARCH API ====================

// Search students
app.get('/api/search', (req, res) => {
    const { query } = req.query;
    
    if (!query) {
        return res.json([]);
    }

    const students = [];
    const attendance = [];
    const searchLower = query.toLowerCase();

    const readStudents = new Promise((resolve, reject) => {
        fs.createReadStream(STUDENTS_FILE)
            .pipe(csv())
            .on('data', (row) => students.push(row))
            .on('end', resolve)
            .on('error', reject);
    });

    const readAttendance = new Promise((resolve, reject) => {
        fs.createReadStream(ATTENDANCE_FILE)
            .pipe(csv())
            .on('data', (row) => attendance.push(row))
            .on('end', resolve)
            .on('error', reject);
    });

    Promise.all([readStudents, readAttendance])
        .then(() => {
            // Search in students
            const studentResults = students.filter(s => 
                s.name.toLowerCase().includes(searchLower) || 
                s.admission_number.toLowerCase().includes(searchLower) ||
                s.rfid_card_number.includes(query)
            );

            // Search in attendance
            const attendanceResults = attendance.filter(r => 
                r.name.toLowerCase().includes(searchLower) || 
                r.admission_number.toLowerCase().includes(searchLower) ||
                r.rfid_card_number.includes(query)
            );

            res.json({
                students: studentResults,
                attendance: attendanceResults
            });
        })
        .catch(err => {
            res.status(500).json({ error: err.message });
        });
});

// Start server
app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
    console.log(`RFID scan endpoint: http://localhost:${PORT}/api/attendance/scan/:rfid`);
});
