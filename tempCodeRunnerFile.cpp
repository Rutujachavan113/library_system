#include <iostream>
#include <vector>
#include <iomanip>
#include <string>
#include <ctime>
#include "sqlite3.h"
using namespace std;

struct Book {
    string title;
    string author;
    string isbn;
    string borrowerName = "N/A";
    bool available = true;
    tm borrowDate{};
    tm dueDate{};
    tm returnDate{};
    double fine = 0.0;
};

vector<Book> books;
sqlite3* db;

tm getCurrentDate() {
    time_t t = time(nullptr);
    tm current = *localtime(&t);
    current.tm_hour = 0;
    current.tm_min = 0;
    current.tm_sec = 0;
    return current;
}

tm addDays(tm date, int days) {
    time_t t = mktime(&date);
    t += days * 24 * 60 * 60;
    return *localtime(&t);
}

double calculateFine(const tm& dueDate, const tm& returnDate) {
    time_t due = mktime(const_cast<tm*>(&dueDate));
    time_t ret = mktime(const_cast<tm*>(&returnDate));
    double secondsLate = difftime(ret, due);
    int daysLate = secondsLate / (60 * 60 * 24);
    return daysLate > 0 ? daysLate * 5.0 : 0.0;
}

string formatDate(const tm& date) {
    if (date.tm_year <= 0) return "N/A";
    char buf[11];
    strftime(buf, sizeof(buf), "%d-%m-%Y", &date);
    return string(buf);
}

static int callback(void* NotUsed, int argc, char** argv, char** azColName) {
    for (int i = 0; i < argc; i++) {
        if (string(azColName[i]) == "Availability") {
            string avail = argv[i] ? argv[i] : "Yes";
            cout << "Availability: " << (avail == "Yes" ? "Yes" : "No") << "\n";
        } else {
            cout << azColName[i] << ": " << (argv[i] ? argv[i] : "NULL") << "\n";
        }
    }
    cout << "---------------------------------------\n";
    return 0;
}

void createTable() {
    const char* sql = "CREATE TABLE IF NOT EXISTS Books (" 
                      "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "Title TEXT NOT NULL, "
                      "Author TEXT NOT NULL, "
                      "ISBN TEXT NOT NULL, "
                      "Availability TEXT NOT NULL);";
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    } else {
        cout << "Table created successfully (or already exists).\n";
    }
}

void addBook() {
    Book book;
    cin.ignore();
    cout << "Enter Book Title: ";
    getline(cin, book.title);
    cout << "Enter Book Author: ";
    getline(cin, book.author);
    cout << "Enter Book ISBN: ";
    getline(cin, book.isbn);

    string sql = "INSERT INTO Books (Title, Author, ISBN, Availability) VALUES ('" 
                  + book.title + "', '" + book.author + "', '" + book.isbn + "', 'Yes');";
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
    } else {
        cout << "Book added successfully!\n";
    }
}

void borrowBook() {
    string queryField, queryValue, borrowerName;
    cin.ignore();
    cout << "Enter the field you want to search by (ISBN, Title, Author): ";
    getline(cin, queryField);
    cout << "Enter the value of " << queryField << ": ";
    getline(cin, queryValue);

    string query = "SELECT * FROM Books WHERE " + queryField + " LIKE '%" + queryValue + "%' AND Availability = 'Yes';";
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), callback, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "Error checking book availability.\n";
        return;
    }

    cout << "Enter borrower's name: ";
    getline(cin, borrowerName);

    // Update the availability and borrower's name in the database
    string updateSql = "UPDATE Books SET Availability = 'No', BorrowerName = '" + borrowerName + "' WHERE " + queryField + " LIKE '%" + queryValue + "%';";
    rc = sqlite3_exec(db, updateSql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "Error updating book availability.\n";
        sqlite3_free(errMsg);
    } else {
        cout << "Book borrowed successfully!\n";
    }
}

void returnBook() {
    string queryField, queryValue;
    cin.ignore();
    cout << "Enter the field you want to return by (ISBN, Title, Author): ";
    getline(cin, queryField);
    cout << "Enter the value of the " << queryField << ": ";
    getline(cin, queryValue);

    string query = "SELECT * FROM Books WHERE " + queryField + " LIKE '%" + queryValue + "%';";
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), callback, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "Error checking book return.\n";
        return;
    }

    tm returnDate = getCurrentDate();
    string updateSql = "UPDATE Books SET Availability = 'Yes', BorrowerName = 'N/A' WHERE " + queryField + " LIKE '%" + queryValue + "%';";
    rc = sqlite3_exec(db, updateSql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "Error updating book return.\n";
        sqlite3_free(errMsg);
    } else {
        cout << "Book returned successfully!\n";
    }

    tm dueDate = getCurrentDate();  // Replace with logic to fetch actual due date
    double fine = calculateFine(dueDate, returnDate);
    cout << "Fine: " << fine << "\n";
}

void searchBook() {
    string query;
    cin.ignore();
    cout << "Enter Title, Author, or ISBN to search: ";
    getline(cin, query);

    // SQL query that searches in all fields: Title, Author, or ISBN
    string sql = "SELECT * FROM Books WHERE Title LIKE '%" + query + "%' OR Author LIKE '%" + query + "%' OR ISBN LIKE '%" + query + "%';";
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), callback, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "Error during search.\n";
    }
}

void displayAllBooks() {
    string query = "SELECT * FROM Books;";
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, query.c_str(), callback, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        cerr << "Error displaying books.\n";
    }
}

int main() {
    int rc = sqlite3_open("library.db", &db);
    if (rc) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
        return 1;
    } else {
        cout << "Opened database successfully.\n";
    }

    createTable();

    int choice;
    while (true) {
        cout << "\n Library Management System\n";
        cout << "1. Add Book\n2. Borrow Book\n3. Return Book\n4. Search Book\n5. Display All Books\n6. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
            case 1: addBook(); break;
            case 2: borrowBook(); break;
            case 3: returnBook(); break;
            case 4: searchBook(); break;
            case 5: displayAllBooks(); break;
            case 6: cout << "Exiting...\n"; sqlite3_close(db); return 0;
            default: cout << "Invalid choice. Try again.\n";
        }
    }

    return 0;
}
