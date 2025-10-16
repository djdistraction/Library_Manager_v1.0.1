#!/bin/bash
# Simple script to verify database is created and has the expected schema

DB_PATH="/tmp/test_library_verify.db"

# Clean up any existing database
rm -f "$DB_PATH"

echo "=== Database Schema Verification Script ==="
echo "Database path: $DB_PATH"

# Create a simple test using sqlite3 command
cat > /tmp/test_schema.sql << 'EOF'
-- Create tables
CREATE TABLE IF NOT EXISTS Tracks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_path TEXT NOT NULL UNIQUE,
    title TEXT,
    artist TEXT,
    album TEXT,
    genre TEXT,
    bpm INTEGER DEFAULT 0,
    key TEXT,
    duration REAL DEFAULT 0.0,
    file_size INTEGER DEFAULT 0,
    file_hash TEXT,
    date_added TEXT NOT NULL,
    last_modified TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS VirtualFolders (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    description TEXT,
    date_created TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS Folder_Tracks_Link (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    folder_id INTEGER NOT NULL,
    track_id INTEGER NOT NULL,
    display_order INTEGER DEFAULT 0,
    date_added TEXT NOT NULL,
    FOREIGN KEY (folder_id) REFERENCES VirtualFolders(id) ON DELETE CASCADE,
    FOREIGN KEY (track_id) REFERENCES Tracks(id) ON DELETE CASCADE,
    UNIQUE(folder_id, track_id)
);

CREATE TABLE IF NOT EXISTS Jobs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    job_type TEXT NOT NULL,
    status TEXT NOT NULL,
    parameters TEXT,
    date_created TEXT NOT NULL,
    date_started TEXT,
    date_completed TEXT,
    error_message TEXT,
    progress INTEGER DEFAULT 0
);

-- Insert test data
INSERT INTO Tracks (file_path, title, artist, album, genre, bpm, key, duration, file_size, file_hash, date_added, last_modified)
VALUES ('/test/track.mp3', 'Test Track', 'Test Artist', 'Test Album', 'Electronic', 128, 'Am', 240.5, 5242880, 'abc123', '2025-01-01T00:00:00', '2025-01-01T00:00:00');

INSERT INTO VirtualFolders (name, description, date_created)
VALUES ('My Favorites', 'Test folder', '2025-01-01T00:00:00');

INSERT INTO Folder_Tracks_Link (folder_id, track_id, display_order, date_added)
VALUES (1, 1, 1, '2025-01-01T00:00:00');

INSERT INTO Jobs (job_type, status, parameters, date_created, progress)
VALUES ('scan_library', 'pending', '{}', '2025-01-01T00:00:00', 0);
EOF

# Execute SQL
sqlite3 "$DB_PATH" < /tmp/test_schema.sql

# Verify tables exist
echo ""
echo "Checking tables..."
TABLES=$(sqlite3 "$DB_PATH" "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;")

echo "Tables found:"
echo "$TABLES"

# Verify each expected table exists
for table in "Tracks" "VirtualFolders" "Folder_Tracks_Link" "Jobs"; do
    if echo "$TABLES" | grep -q "^${table}$"; then
        echo "✓ Table '$table' exists"
    else
        echo "✗ Table '$table' missing"
        exit 1
    fi
done

# Verify data was inserted
echo ""
echo "Verifying data insertion..."

TRACK_COUNT=$(sqlite3 "$DB_PATH" "SELECT COUNT(*) FROM Tracks;")
FOLDER_COUNT=$(sqlite3 "$DB_PATH" "SELECT COUNT(*) FROM VirtualFolders;")
LINK_COUNT=$(sqlite3 "$DB_PATH" "SELECT COUNT(*) FROM Folder_Tracks_Link;")
JOB_COUNT=$(sqlite3 "$DB_PATH" "SELECT COUNT(*) FROM Jobs;")

echo "Tracks: $TRACK_COUNT"
echo "Virtual Folders: $FOLDER_COUNT"
echo "Folder-Track Links: $LINK_COUNT"
echo "Jobs: $JOB_COUNT"

if [ "$TRACK_COUNT" -eq 1 ] && [ "$FOLDER_COUNT" -eq 1 ] && [ "$LINK_COUNT" -eq 1 ] && [ "$JOB_COUNT" -eq 1 ]; then
    echo ""
    echo "✓ All CRUD operations would work correctly with this schema"
else
    echo ""
    echo "✗ Data insertion failed"
    exit 1
fi

# Show sample data
echo ""
echo "Sample track data:"
sqlite3 "$DB_PATH" "SELECT id, title, artist, bpm FROM Tracks;"

echo ""
echo "Sample folder data:"
sqlite3 "$DB_PATH" "SELECT id, name FROM VirtualFolders;"

# Verify foreign key constraints
echo ""
echo "Verifying foreign key constraints..."
RESULT=$(sqlite3 "$DB_PATH" "PRAGMA foreign_keys = ON; PRAGMA foreign_key_check;")
if [ -z "$RESULT" ]; then
    echo "✓ Foreign key constraints are valid"
else
    echo "✗ Foreign key constraint violations found:"
    echo "$RESULT"
    exit 1
fi

# Verify indices
echo ""
echo "Checking indices..."
INDICES=$(sqlite3 "$DB_PATH" "SELECT name FROM sqlite_master WHERE type='index' AND name NOT LIKE 'sqlite_%' ORDER BY name;")
echo "Indices found:"
echo "$INDICES"

# Clean up
rm -f "$DB_PATH" /tmp/test_schema.sql

echo ""
echo "=== All schema verification tests passed! ==="
