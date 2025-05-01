CREATE TABLE IF NOT EXISTS books (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    author TEXT NOT NULL,
    isbn TEXT UNIQUE,
    borrowerName TEXT DEFAULT 'N/A',
    available INTEGER DEFAULT 1,
    borrowDate TEXT,
    dueDate TEXT,
    returnDate TEXT,
    fine REAL DEFAULT 0.0
);
