#include <arpa/inet.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
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
                      "return_date INTEGER, " // return date as Unix timestamp
                      "availability INTEGER);";
  sqlite3_exec(db, books_table, 0, 0, 0);

  // Create rented_books table with rental_period
  char *rented_books_table = "CREATE TABLE IF NOT EXISTS rented_books ("
                             "user_id INTEGER, "
                             "book_id INTEGER, "
                             "rental_period INTEGER, " // number of days rented
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
  char buffer[1024] = {0};
  sqlite3 *db;
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    // Error opening the database
    const char *err_msg = sqlite3_errmsg(db);
    snprintf(buffer, sizeof(buffer), "Can't open database: %s\n", err_msg);
    send(client_socket, buffer, strlen(buffer), 0);
    return;
  }

  sqlite3_stmt *stmt;

  // Query for user information based on name
  snprintf(buffer, sizeof(buffer),
           "SELECT id, name, zip FROM users WHERE name = '%s';", name);
  rc = sqlite3_prepare_v2(db, buffer, -1, &stmt, 0);

  if (rc != SQLITE_OK) {
    // Handle SQL preparation failure
    const char *err_msg = sqlite3_errmsg(db);
    snprintf(buffer, sizeof(buffer), "Error preparing user query: %s\n",
             err_msg);
    send(client_socket, buffer, strlen(buffer), 0);
    return;
  }

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    int user_id = sqlite3_column_int(stmt, 0);
    const char *username = (const char *)sqlite3_column_text(stmt, 1);
    int zip = sqlite3_column_int(stmt, 2);

    // Send user info to client
    snprintf(buffer, sizeof(buffer),
             "User Account Information:\nName: %s\nZIP: %d\n", username, zip);
    send(client_socket, buffer, strlen(buffer), 0);

    sqlite3_finalize(stmt);

    // Query for rented books using user_id
    snprintf(buffer, sizeof(buffer),
             "SELECT b.name, b.author, b.return_date FROM rented_books rb "
             "JOIN books b ON rb.book_id = b.id WHERE rb.user_id = %d;",
             user_id);
    rc = sqlite3_prepare_v2(db, buffer, -1, &stmt, 0);

    if (rc != SQLITE_OK) {
      // Handle SQL preparation failure for rented books query
      const char *err_msg = sqlite3_errmsg(db);
      snprintf(buffer, sizeof(buffer),
               "Error preparing rented books query: %s\n", err_msg);
      send(client_socket, buffer, strlen(buffer), 0);
      sqlite3_finalize(stmt);
      return;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      // If rented books are found
      send(client_socket, "Rented Books:\n", 14, 0);
      do {
        const char *book_name = (const char *)sqlite3_column_text(stmt, 0);
        const char *author = (const char *)sqlite3_column_text(stmt, 1);
        const char *return_date = (const char *)sqlite3_column_text(stmt, 2);

        // Calculate renting time in days
        time_t now = time(NULL);
        struct tm return_tm = {0};
        sscanf(return_date, "%d-%d-%d", &return_tm.tm_year, &return_tm.tm_mon,
               &return_tm.tm_mday);
        return_tm.tm_year -= 1900;
        return_tm.tm_mon -= 1;
        time_t return_time = mktime(&return_tm);

        int renting_time = difftime(return_time, now) / (60 * 60 * 24);

        // Assuming return_date is the number of days from the rental date
        snprintf(buffer, sizeof(buffer),
                 "Book: %s, Author: %s, Renting Time: %d days\n", book_name,
                 author, renting_time);
        send(client_socket, buffer, strlen(buffer), 0);
      } while (sqlite3_step(stmt) == SQLITE_ROW);
    } else {
      // No rented books found
      send(client_socket, "No rented books.\n", 17, 0);
    }
  } else {
    // Error retrieving account information
    send(client_socket, "Error retrieving account information.\n", 38, 0);
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db); // Close the database connection
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

// Function to rent a book using user_id
void rent_book(int client_socket, int user_id) {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    return;
  }

  sqlite3_stmt *stmt;
  char buffer[1024] = {0};
  int book_id, rental_period = 20; // Rental period (e.g., 20 days)

  // Send available books to the user
  snprintf(buffer, sizeof(buffer),
           "Available Books:\nID | Name | Author | Availability\n");
  send(client_socket, buffer, strlen(buffer), 0);

  const char *sql = "SELECT id, name, author, availability FROM books WHERE "
                    "availability > 0;";
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      int id = sqlite3_column_int(stmt, 0);
      const char *name = (const char *)sqlite3_column_text(stmt, 1);
      const char *author = (const char *)sqlite3_column_text(stmt, 2);
      int availability = sqlite3_column_int(stmt, 3);

      snprintf(buffer, sizeof(buffer), "%d | %s | %s | %d\n", id, name, author,
               availability);
      send(client_socket, buffer, strlen(buffer), 0);
    }
  }
  sqlite3_finalize(stmt);

  // Ask user to select a book to rent
  send(client_socket, "Enter the book ID you want to rent: ", 35, 0);
  int bytes_read = read(client_socket, buffer, sizeof(buffer));
  buffer[bytes_read] = '\0'; // Null-terminate the string
  book_id = atoi(buffer);

  // Get current time and calculate return date
  time_t current_time;
  struct tm *tm_info;
  time(&current_time);
  tm_info = localtime(&current_time);
  tm_info->tm_mday += rental_period; // Add rental period (20 days)
  mktime(tm_info);                   // Normalize the structure

  char return_date_str[20];
  strftime(return_date_str, sizeof(return_date_str), "%Y-%m-%d", tm_info);

  // Update books table with return date and decrease availability
  snprintf(buffer, sizeof(buffer),
           "UPDATE books SET return_date = '%s', availability = availability - "
           "1 WHERE id = %d;",
           return_date_str, book_id);
  rc = sqlite3_exec(db, buffer, 0, 0, 0);
  printf("Debug: Executing SQL update: %s\n", buffer);

  // Insert into rented_books table with rental period
  snprintf(buffer, sizeof(buffer),
           "INSERT INTO rented_books (user_id, book_id, rental_period) VALUES "
           "(%d, %d, %d);",
           user_id, book_id, rental_period);
  rc = sqlite3_exec(db, buffer, 0, 0, 0);
  printf("Debug: Executing SQL insert: %s\n", buffer);

  if (rc != SQLITE_OK) {
    send(client_socket, "Error renting book.\n", 20, 0);
    const char *err_msg = sqlite3_errmsg(db);
    printf("Debug: Error message: %s\n", err_msg);
  } else {
    send(client_socket, "Book rented successfully!\n", 25, 0);
  }
}

// Function to return a book using user_id
void return_book(int client_socket, int user_id) {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    return;
  }

  sqlite3_stmt *stmt;
  char buffer[1024] = {0};
  int book_id;

  // Display rented books for the user
  snprintf(buffer, sizeof(buffer),
           "Your Rented Books:\nID | Name | Author | Rental Period (Days) | "
           "Return Date\n");
  send(client_socket, buffer, strlen(buffer), 0);

  const char *sql =
      "SELECT b.id, b.name, b.author, rb.rental_period, b.return_date "
      "FROM rented_books rb "
      "JOIN books b ON rb.book_id = b.id "
      "WHERE rb.user_id = ?;";

  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
  if (rc == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, user_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      int id = sqlite3_column_int(stmt, 0);
      const char *name = (const char *)sqlite3_column_text(stmt, 1);
      const char *author = (const char *)sqlite3_column_text(stmt, 2);
      int rental_period = sqlite3_column_int(stmt, 3);
      const char *return_date = (const char *)sqlite3_column_text(stmt, 4);

      snprintf(buffer, sizeof(buffer), "%d | %s | %s | %d | %s\n", id, name,
               author, rental_period, return_date);
      send(client_socket, buffer, strlen(buffer), 0);
    }
  }
  sqlite3_finalize(stmt);

  // Ask the user to select the book they want to return
  send(client_socket, "Enter the book ID you want to return: ", 35, 0);
  int bytes_read = read(client_socket, buffer, sizeof(buffer));
  sscanf(buffer, "%d", &book_id);

  // Update availability in books table
  snprintf(buffer, sizeof(buffer),
           "UPDATE books SET availability = availability + 1 WHERE id = %d;",
           book_id);
  rc = sqlite3_exec(db, buffer, 0, 0, 0);

  // Remove the book from rented_books table
  snprintf(buffer, sizeof(buffer),
           "DELETE FROM rented_books WHERE user_id = %d AND book_id = %d;",
           user_id, book_id);
  rc = sqlite3_exec(db, buffer, 0, 0, 0);

  if (rc != SQLITE_OK) {
    send(client_socket, "Error returning book.\n", 21, 0);
  } else {
    send(client_socket, "Book returned successfully!\n", 27, 0);
  }
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
  return 0;
}

void handle_client(int client_socket) {
  char buffer[1024] = {0};
  int option, login_success = 0;
  char name[50];

  while (!login_success) {
    // Send options menu to client
    send(client_socket,
         "Choose an option:\n1. Login\n2. Create Account\n99. Exit\n", 57, 0);

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
    case 99: // Exit
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
         "Book\n4. Return Book\n99. Exit\nChoose an option: ",
         116, 0);

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
    case 99: // Exit
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
