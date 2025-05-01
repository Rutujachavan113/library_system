#include <iostream>
#include "sqlite3.h"
#include <string>
#include <sstream>
#include <cstdlib>
#include <cctype>

using namespace std;

// Callback to display rows
static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << "\n";
    }
    cout << "\n";
    return 0;
}

// Check if string is numeric
bool isNumeric(const string& str) {
    for (char c : str) {
        if (!isdigit(c)) return false;
    }
    return !str.empty();
}

// Create table and indexes
void createTable(sqlite3* db) {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS Books (
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            Title TEXT NOT NULL,
            Author TEXT NOT NULL,
            ISBN TEXT NOT NULL UNIQUE,
            Availability INTEGER NOT NULL
        );
        
        CREATE INDEX IF NOT EXISTS idx_title ON Books (Title);
        CREATE INDEX IF NOT EXISTS idx_author ON Books (Author);
        CREATE INDEX IF NOT EXISTS idx_isbn ON Books (ISBN);
    )";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    } else {
        cout << "Database table and indexes initialized.\n";
    }
}

// Add book
void addBook(sqlite3* db) {
    string title, author, isbn;

    cout << "Enter title: ";
    getline(cin, title);

    cout << "Enter author: ";
    getline(cin, author);

    while (true) {
        cout << "Enter numeric ISBN: ";
        getline(cin, isbn);
        if (isNumeric(isbn)) break;
        else cout << "Invalid ISBN. Only numbers are allowed.\n";
    }

    string sql = "INSERT INTO Books (Title, Author, ISBN, Availability) VALUES ('"
                 + title + "', '" + author + "', '" + isbn + "', 1);";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    } else {
        cout << "Book added successfully.\n";
    }
}

// Update availability
bool updateAvailability(sqlite3* db, const string& condition, int newStatus) {
    stringstream ss;
    ss << "UPDATE Books SET Availability = " << newStatus << " WHERE " << condition
       << " AND Availability = " << (newStatus == 0 ? 1 : 0) << ";";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, ss.str().c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Prepare error: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    sqlite3_step(stmt);
    int changes = sqlite3_changes(db);
    sqlite3_finalize(stmt);

    if (changes > 0) {
        cout << (newStatus == 0 ? "Book borrowed successfully.\n" : "Book returned successfully.\n");
        return true;
    } else {
        cout << "No matching available book found or already in desired status.\n";
        return false;
    }
}

// Borrow book
void borrowBook(sqlite3* db) {
    cout << "Borrow by:\n1. Title\n2. Author\n3. ISBN\nEnter choice: ";
    int choice; cin >> choice; cin.ignore();
    string value, condition;

    if (choice == 1) { cout << "Enter Title: "; getline(cin, value); condition = "Title = '" + value + "'"; }
    else if (choice == 2) { cout << "Enter Author: "; getline(cin, value); condition = "Author = '" + value + "'"; }
    else if (choice == 3) {
        while (true) {
            cout << "Enter numeric ISBN: "; getline(cin, value);
            if (isNumeric(value)) break;
            else cout << "Invalid ISBN. Only numbers are allowed.\n";
        }
        condition = "ISBN = '" + value + "'";
    }
    else { cout << "Invalid choice.\n"; return; }

    updateAvailability(db, condition, 0);
}

// Return book
void returnBook(sqlite3* db) {
    cout << "Return by:\n1. Title\n2. Author\n3. ISBN\nEnter choice: ";
    int choice; cin >> choice; cin.ignore();
    string value, condition;

    if (choice == 1) { cout << "Enter Title: "; getline(cin, value); condition = "Title = '" + value + "'"; }
    else if (choice == 2) { cout << "Enter Author: "; getline(cin, value); condition = "Author = '" + value + "'"; }
    else if (choice == 3) {
        while (true) {
            cout << "Enter numeric ISBN: "; getline(cin, value);
            if (isNumeric(value)) break;
            else cout << "Invalid ISBN. Only numbers are allowed.\n";
        }
        condition = "ISBN = '" + value + "'";
    }
    else { cout << "Invalid choice.\n"; return; }

    updateAvailability(db, condition, 1);
}

// Search books
void searchBooks(sqlite3* db) {
    cout << "Search by:\n1. Title\n2. Author\n3. ISBN\nEnter choice: ";
    int choice; cin >> choice; cin.ignore();
    string term, query;

    if (choice == 1) {
        cout << "Enter Title keyword: "; getline(cin, term);
        query = "SELECT ID, Title, Author, ISBN, "
                "CASE Availability WHEN 1 THEN 'Yes' ELSE 'No' END AS Availability "
                "FROM Books WHERE Title LIKE '%" + term + "%';";
    } else if (choice == 2) {
        cout << "Enter Author keyword: "; getline(cin, term);
        query = "SELECT ID, Title, Author, ISBN, "
                "CASE Availability WHEN 1 THEN 'Yes' ELSE 'No' END AS Availability "
                "FROM Books WHERE Author LIKE '%" + term + "%';";
    } else if (choice == 3) {
        while (true) {
            cout << "Enter numeric ISBN keyword: "; getline(cin, term);
            if (isNumeric(term)) break;
            else cout << "Invalid ISBN. Only numbers are allowed.\n";
        }
        query = "SELECT ID, Title, Author, ISBN, "
                "CASE Availability WHEN 1 THEN 'Yes' ELSE 'No' END AS Availability "
                "FROM Books WHERE ISBN LIKE '%" + term + "%';";
    } else {
        cout << "Invalid choice.\n";
        return;
    }

    cout << "Search results:\n";
    int rc = sqlite3_exec(db, query.c_str(), callback, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Error during search.\n";
    }
}

// Display all books
void displayBooks(sqlite3* db) {
    string sql = R"(
        SELECT ID, Title, Author, ISBN,
        CASE Availability WHEN 1 THEN 'Yes' ELSE 'No' END AS Availability
        FROM Books;
    )";

    cout << "\nAll books in the library:\n";
    int rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        cerr << "Error displaying books.\n";
    }
}

// Main
int main() {
    sqlite3* db;
    int rc = sqlite3_open("library.db", &db);
    if (rc) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
        return 1;
    } else {
        cout << "Opened database successfully.\n";
    }

    createTable(db);

    int option = 0;
    while (true) {
        cout << "\n--- Advanced Library Management System ---\n";
        cout << "1. Add Book\n2. Borrow Book\n3. Return Book\n4. Search Book\n5. Display All Books\n6. Exit\n";
        cout << "Enter your choice: ";
        cin >> option;
        cin.ignore();

        switch (option) {
            case 1: addBook(db); break;
            case 2: borrowBook(db); break;
            case 3: returnBook(db); break;
            case 4: searchBooks(db); break;
            case 5: displayBooks(db); break;
            case 6:
                sqlite3_close(db);
                cout << "Goodbye!\n";
                return 0;
            default:
                cout << "Please choose a valid option.\n";
        }
    }

    return 0;
}