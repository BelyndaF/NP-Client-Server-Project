#include <arpa/inet.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define MAX_CLIENTS 10

// Database connection
sqlite3 *db;

// Function to close the database connection
void close_db() {
  if (db != NULL) {
    sqlite3_close(db);
    db = NULL;
    printf("Database connection closed.\n");
  }
}

// Initialize the database
void init_db() {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("Opened database successfully. Init DB\n");
  }

  // Create users table
  char *users_table = "CREATE TABLE IF NOT EXISTS users ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "name TEXT, "
                      "password TEXT, "
                      "zip INTEGER);";
  sqlite3_exec(db, users_table, 0, 0, 0);

  // Create books table
  char *books_table = "CREATE TABLE IF NOT EXISTS books ("
                      "id INTEGER PRIMARY KEY, "
                      "name TEXT, "
                      "author TEXT, "
                      "return_date INTEGER, "
                      "availability INTEGER);";
  sqlite3_exec(db, books_table, 0, 0, 0);

  // Create rented_books table to track rented books by user
  char *rented_books_table = "CREATE TABLE IF NOT EXISTS rented_books ("
                             "user_id INTEGER, "
                             "book_id INTEGER, "
                             "FOREIGN KEY (user_id) REFERENCES users(id), "
                             "FOREIGN KEY (book_id) REFERENCES books(id));";
  sqlite3_exec(db, rented_books_table, 0, 0, 0);
}

// function to prepopulate books table
void initialize_books() {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("Opened database successfully. Initialize books\n");
  }
  sqlite3_stmt *stmt;
  const char *check_sql = "SELECT COUNT(*) FROM books;";
  sqlite3_prepare_v2(db, check_sql, -1, &stmt, 0);
  sqlite3_step(stmt);
  int count = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);

  if (count == 0) {
    const char *insert_sql =
        "INSERT INTO books (id, name, author, return_date, availability) "
        "VALUES "
        "(1, 'C Programming', 'Dennis Ritchie', 0, 5),"
        "(2, 'Data Structures', 'Mark Allen Weiss', 0, 3),"
        "(3, 'Algorithms', 'Robert Sedgewick', 0, 4),"
        "(4, 'Operating Systems', 'Andrew Tanenbaum', 0, 6),"
        "(5, 'Computer Networks', 'James Kurose', 0, 2),"
        "(6, 'Artificial Intelligence', 'Stuart Russell', 0, 5),"
        "(7, 'Machine Learning', 'Tom Mitchell', 0, 3),"
        "(8, 'Database Systems', 'Raghu Ramakrishnan', 0, 4),"
        "(9, 'Software Engineering', 'Ian Sommerville', 0, 5),"
        "(10, 'Computer Graphics', 'Donald Hearn', 0, 3);";

    sqlite3_exec(db, insert_sql, 0, 0, 0);
    printf("Books added to the database.\n");
  }
}

// Function to create a new account
void create_account(int client_socket, char name[]) {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("Database opened successfully\n");
  }

  char buffer[1024] = {0};
  char username[50], password[50];
  int zip;

  // Debug: Start of function
  printf("Debug: Starting create_account function\n");

  // Get username
  send(client_socket, "Enter a username: ", 18, 0);
  int bytes_read = read(client_socket, buffer, 1024);
  printf("Debug: Read %d bytes for username input: %s\n", bytes_read, buffer);

  if (bytes_read <= 0) {
    printf("Error: Failed to read username from client\n");
    send(client_socket, "Error reading input\n", 20, 0);
    return;
  }

  sscanf(buffer, "%s", username);
  printf("Debug: Parsed username: %s\n", username);

  // Check if username already exists
  char check_sql[256];
  snprintf(check_sql, sizeof(check_sql),
           "SELECT COUNT(*) FROM users WHERE name='%s';", username);
  printf("Debug: Executing SQL query: %s\n", check_sql);

  sqlite3_stmt *stmt;
  rc = sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    printf("Error: sqlite3_prepare_v2 failed with error: %s\n",
           sqlite3_errmsg(db));
    send(client_socket, "Failed to query database\n", 25, 0);
    sqlite3_finalize(stmt);
    return;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    int user_count = sqlite3_column_int(stmt, 0);
    printf("Debug: User count from query: %d\n", user_count);

    if (user_count > 0) {
      send(client_socket, "Username already exists. Please try another one.\n",
           49, 0);
      sqlite3_finalize(stmt);
      return;
    }
  } else {
    printf("Error: sqlite3_step failed with error code: %d\n", rc);
  }
  sqlite3_finalize(stmt);

  // Get password
  send(client_socket, "Enter a password: ", 18, 0);
  memset(buffer, 0, sizeof(buffer));
  bytes_read = read(client_socket, buffer, 1024);
  printf("Debug: Read %d bytes for password input: %s\n", bytes_read, buffer);

  if (bytes_read <= 0) {
    printf("Error: Failed to read password from client\n");
    send(client_socket, "Error reading input\n", 20, 0);
    return;
  }

  sscanf(buffer, "%s", password);
  printf("Debug: Parsed password: %s\n", password);

  // Get zip code
  send(client_socket, "Enter your ZIP code: ", 21, 0);
  memset(buffer, 0, sizeof(buffer));
  bytes_read = read(client_socket, buffer, 1024);
  printf("Debug: Read %d bytes for ZIP code input: %s\n", bytes_read, buffer);

  if (bytes_read <= 0) {
    printf("Error: Failed to read ZIP code from client\n");
    send(client_socket, "Error reading input\n", 20, 0);
    return;
  }

  sscanf(buffer, "%d", &zip);
  printf("Debug: Parsed ZIP code: %d\n", zip);

  // Insert new user into the database
  char sql[256];
  snprintf(sql, sizeof(sql),
           "INSERT INTO users (name, password, zip) VALUES ('%s', '%s', %d);",
           username, password, zip);
  printf("Debug: Executing SQL insert: %s\n", sql);

  rc = sqlite3_exec(db, sql, 0, 0, 0);
  if (rc != SQLITE_OK) {
    printf("Error: sqlite3_exec failed with error: %s\n", sqlite3_errmsg(db));
    send(client_socket, "Account creation failed\n", 24, 0);
  } else {
    strncpy(name, username, 50);
    send(client_socket, "Account created successfully\n", 29, 0);
    printf("Debug: Account created successfully for user: %s\n", username);
  }
}

// function to get user id based on name
int get_user_id(char name[]) {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("Database opened successfully. Get user id.\n");
  }
  sqlite3_stmt *stmt;
  const char *sql =
      "SELECT id FROM users WHERE name = ? LIMIT 1;"; // SQL query to fetch the
                                                      // user ID based on name
  int user_id = -1; // Default value for "not found"

  // Prepare the SQL statement
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
    printf("Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return user_id;
  }

  // Bind the name parameter to the SQL query
  if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK) {
    printf("Failed to bind name: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return user_id;
  }

  // Execute the query and check if a result is returned
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    // Retrieve the user ID from the result set
    user_id = sqlite3_column_int(stmt, 0); // 0 is the column index for "id"
  } else {
    printf("User not found.\n");
  }

  // Finalize the statement to free resources
  sqlite3_finalize(stmt);
  close_db();
  return user_id;
}

// Function to display user account information, including rented books
void display_account_info(int client_socket, char name[]) {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("Database opened successfully. Display Info\n");
  }

  char buffer[1024] = {0};
  sqlite3_stmt *stmt;

  // Query for user information based on name
  snprintf(buffer, sizeof(buffer),
           "SELECT id, name, zip FROM users WHERE name = '%s';", name);
  rc = sqlite3_prepare_v2(db, buffer, -1, &stmt, 0);

  if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
    int user_id = sqlite3_column_int(stmt, 0);
    const char *username = (const char *)sqlite3_column_text(stmt, 1);
    int zip = sqlite3_column_int(stmt, 2);

    snprintf(buffer, sizeof(buffer),
             "User Account Information:\nName: %s\nZIP: %d", username, zip);
    send(client_socket, buffer, strlen(buffer), 0);
    sqlite3_finalize(stmt);

    // Query for rented books using user_id
    snprintf(buffer, sizeof(buffer),
             "SELECT b.name, b.author, b.return_date FROM rented_books rb "
             "JOIN books b ON rb.book_id = b.id WHERE rb.user_id = %d;",
             user_id);
    rc = sqlite3_prepare_v2(db, buffer, -1, &stmt, 0);

    if (rc == SQLITE_OK) {
      if (sqlite3_step(stmt) == SQLITE_ROW) {
        send(client_socket, "Rented Books:\n", 14, 0);
        do {
          const char *book_name = (const char *)sqlite3_column_text(stmt, 0);
          const char *author = (const char *)sqlite3_column_text(stmt, 1);
          int return_date = sqlite3_column_int(stmt, 2);

          snprintf(buffer, sizeof(buffer),
                   "Book: %s, Author: %s, Return Date: %d\n", book_name, author,
                   return_date);
          send(client_socket, buffer, strlen(buffer), 0);
        } while (sqlite3_step(stmt) == SQLITE_ROW);
      } else {
        send(client_socket, "No rented books.\n", 17, 0);
      }
    } else {
      send(client_socket, "Error retrieving rented books.\n", 31, 0);
    }
  } else {
    send(client_socket, "Error retrieving account information.\n", 38, 0);
  }

  sqlite3_finalize(stmt);
}

// Function to display all books in the bookstore
void find_books(int client_socket) {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("Database opened successfully. Find Book\n");
  }
  char buffer[1024] = {0};
  sqlite3_stmt *stmt;

  // SQL query to select all books from the books table
  const char *sql =
      "SELECT id, name, author, return_date, availability FROM books;";
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK) {
    send(client_socket, "Error retrieving books from the database.\n", 41, 0);
    return;
  }

  // Check if there are any books available
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    send(client_socket, "Available Books:\n", 17, 0);

    // Loop through each row in the result set
    do {
      int id = sqlite3_column_int(stmt, 0);
      const char *name = (const char *)sqlite3_column_text(stmt, 1);
      const char *author = (const char *)sqlite3_column_text(stmt, 2);
      int return_date = sqlite3_column_int(stmt, 3);
      int availability = sqlite3_column_int(stmt, 4);

      snprintf(
          buffer, sizeof(buffer),
          "ID: %d, Name: %s, Author: %s, Return Date: %d, Availability: %d\n",
          id, name, author, return_date, availability);
      send(client_socket, buffer, strlen(buffer), 0);
    } while (sqlite3_step(stmt) == SQLITE_ROW);
  } else {
    send(client_socket, "No books available in the bookstore.\n", 37, 0);
  }

  sqlite3_finalize(stmt);
}

// Function to rent a book
void rent_book(int client_socket, int user_id) {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("Database opened successfully. Rent Book\n");
  }
  char buffer[1024] = {0};
  int book_id;
  sqlite3_stmt *stmt;

  // Ask client for the book ID to rent
  send(client_socket, "Enter the ID of the book you want to rent: ", 42, 0);
  // Receive client choice
  memset(buffer, 0, sizeof(buffer));
  read(client_socket, buffer, sizeof(buffer));
  book_id = atoi(buffer);

  // Check if the book exists and is available
  snprintf(buffer, sizeof(buffer),
           "SELECT availability FROM books WHERE id = %d;", book_id);
  if (sqlite3_prepare_v2(db, buffer, -1, &stmt, 0) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      int availability = sqlite3_column_int(stmt, 0);

      if (availability > 0) {
        // Book is available; proceed with renting

        // Decrease the book's availability count
        snprintf(
            buffer, sizeof(buffer),
            "UPDATE books SET availability = availability - 1 WHERE id = %d;",
            book_id);
        sqlite3_exec(db, buffer, 0, 0, 0);

        // Add entry to rented_books
        snprintf(buffer, sizeof(buffer),
                 "INSERT INTO rented_books (user_id, book_id) VALUES (%d, %d);",
                 user_id, book_id);
        sqlite3_exec(db, buffer, 0, 0, 0);

        send(client_socket, "Book rented successfully.\n", 26, 0);
      } else {
        // Book is not available
        send(client_socket, "Book is not available for rent.\n", 32, 0);
      }
    } else {
      send(client_socket, "Book ID not found.\n", 19, 0);
    }
  } else {
    send(client_socket, "Error checking book availability.\n", 35, 0);
  }
  sqlite3_finalize(stmt);
}

// Function to return a book
void return_book(int client_socket, int user_id) {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("Database opened successfully. Return Book\n");
  }

  char buffer[1024] = {0};
  int book_id;
  sqlite3_stmt *stmt;

  // Ask client for the book ID to return
  send(client_socket, "Enter the ID of the book you want to return: ", 44, 0);
  read(client_socket, buffer, sizeof(buffer));
  book_id = atoi(buffer);

  // Check if the user has this book rented
  snprintf(buffer, sizeof(buffer),
           "SELECT * FROM rented_books WHERE user_id = %d AND book_id = %d;",
           user_id, book_id);
  if (sqlite3_prepare_v2(db, buffer, -1, &stmt, 0) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      // User has the book rented; proceed with return

      // Remove entry from rented_books
      snprintf(buffer, sizeof(buffer),
               "DELETE FROM rented_books WHERE user_id = %d AND book_id = %d;",
               user_id, book_id);
      sqlite3_exec(db, buffer, 0, 0, 0);

      // Increase the book's availability count
      snprintf(
          buffer, sizeof(buffer),
          "UPDATE books SET availability = availability + 1 WHERE id = %d;",
          book_id);
      sqlite3_exec(db, buffer, 0, 0, 0);

      send(client_socket, "Book returned successfully.\n", 28, 0);
    } else {
      send(client_socket, "You have not rented this book.\n", 31, 0);
    }
  } else {
    send(client_socket, "Error checking rental status.\n", 30, 0);
  }
  sqlite3_finalize(stmt);
}

// Function to log in
int login(int client_socket, char name[]) {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("Database opened successfully, login\n");
  }

  char buffer[1024] = {0};
  char username[50], password[50];
  sqlite3_stmt *stmt;

  // Get username
  send(client_socket, "Enter your username: ", 21, 0);
  read(client_socket, buffer, 1024);
  sscanf(buffer, "%s", username);

  // Get password
  send(client_socket, "Enter your password: ", 21, 0);
  memset(buffer, 0, sizeof(buffer));
  read(client_socket, buffer, 1024);
  sscanf(buffer, "%s", password);

  // Prepare SQL query to check the username and password
  char sql[256];
  snprintf(sql, sizeof(sql), "SELECT password FROM users WHERE name='%s';",
           username);
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK) {
    send(client_socket, "Login failed: error retrieving user information\n", 47,
         0);
    return 0;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    const unsigned char *db_password = sqlite3_column_text(stmt, 0);

    // Compare passwords
    if (strcmp((const char *)db_password, password) == 0) {
      strncpy(name, username, 50);
      send(client_socket, "Login successful\n", 17, 0);
      sqlite3_finalize(stmt);
      return 1;
    } else {
      send(client_socket, "Login failed: incorrect password\n", 33, 0);
    }
  } else {
    send(client_socket, "Login failed: username not found\n", 33, 0);
  }

  sqlite3_finalize(stmt);
  shutdown(client_socket, SHUT_WR);
  return 0;
}

void handle_client(int client_socket) {
  char buffer[1024] = {0};
  int option, login_success = 0;
  char name[50];

  while (!login_success) {
    // Send options menu to client
    send(client_socket,
         "Choose an option:\n1. Login\n2. Create Account\n5. Exit\n", 58, 0);

    // Receive client choice
    memset(buffer, 0, sizeof(buffer));
    read(client_socket, buffer, sizeof(buffer));
    option = atoi(buffer); // Convert input to integer for comparison

    switch (option) {
    case 1: // Login
      login_success = login(client_socket, name);
      close_db();
      break;
    case 2: // Create Account
      create_account(client_socket, name);
      close_db();
      break;
    case 5: // Exit
      send(client_socket, "Exiting...\n", 11, 0);
      close(client_socket);
      return; // Exit the function, closing connection with client
    default:
      send(client_socket, "Invalid option. Please try again.\n", 35, 0);
      break;
    }
  }

  int exit = 0;
  int user_id;
  while (!exit) {
    send(client_socket,
         "Welcome to the online library!\n1. Account\n2. Find Book\n3. Rent "
         "Book\n4. Return Book\n5. Exit\nChoose an option: ",
         115, 0);

    memset(buffer, 0, sizeof(buffer));
    read(client_socket, buffer, sizeof(buffer));
    option = atoi(buffer); // Convert input to integer for comparison

    switch (option) {
    case 1: // Account
      display_account_info(client_socket, name);
      close_db();
      break;
    case 2: // Find Book
      find_books(client_socket);
      close_db();
      break;
    case 3: // Rent Book
      user_id = get_user_id(name);
      rent_book(client_socket, user_id);
      close_db();
      break;
    case 4: // Return Book
      user_id = get_user_id(name);
      return_book(client_socket, user_id);
      close_db();
      break;
    case 5: // Exit
      send(client_socket, "Exiting...\n", 11, 0);
      close(client_socket);
      return;
    default:
      send(client_socket, "Option not implemented yet.\n", 28, 0);
      break;
    }
  }
  close(client_socket); // Close the socket when done
}

int main() {
  // initialize database
  init_db();
  close_db();
  initialize_books();
  close_db();

  int server_fd, client_socket;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);

  // Create server socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Bind the socket to the port
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("setsockopt failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  // Listen for incoming connections
  if (listen(server_fd, MAX_CLIENTS) < 0) {
    perror("listen failed");
    close(server_fd);
    exit(EXIT_FAILURE);
  }

  printf("Server is listening on port %d\n", PORT);

  while (1) {
    // Accept a new connection
    if ((client_socket = accept(server_fd, (struct sockaddr *)&address,
                                (socklen_t *)&addrlen)) < 0) {
      perror("accept failed");
      continue;
    }

    printf("New client connected\n");

    // Handle each client connection in a new process
    if (fork() == 0) {
      close(server_fd); // Close server socket in the child process
      handle_client(client_socket);
      exit(0);
    }
    close(client_socket); // Close client socket in the parent process
  }

  return 0;
}
