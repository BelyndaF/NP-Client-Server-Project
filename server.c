#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <sqlite3.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h> // read(), write(), close()
#define MAX 80 
#define PORT 8080 
#define SA struct sockaddr 


void close_db();
void init_db();
void initialize_books();
bool create_account(char username[], char password[]);
bool login(char username[], char password[]);
void display_account_info(int connfd, char name[]);
int get_user_id(char name[]);
void find_books(int connfd);
void rent_book(int connfd, int user_id);
void return_book(int connfd, int user_id);
  
// Function designed for chat between client and server. 
void func(int connfd) 
{ 
    char buff[MAX]; 
    int n; 
    char name[50];
    // infinite loop for chat 
    for (;;) { 
        bzero(buff, MAX); 
  
        // read the message from client and copy it in buffer 
        read(connfd, buff, sizeof(buff)); 
        // print buffer which contains the client contents 
        printf("From client: %s\n", buff);

        if (strncmp(buff, "1", 1) == 0) {
            // Option 1: Login
            write(connfd, "login", 5);

            char username[50], password[50];
            
            // get username
            bzero(buff, sizeof(buff));
            read(connfd, buff, sizeof(buff));
            sscanf(buff, "%49s", username);
            sscanf(buff, "%49s", name);
            printf("Username: %s\n", username);

            // get password
            bzero(buff, sizeof(buff));
            read(connfd, buff, sizeof(buff));
            sscanf(buff, "%49s", password); 
            printf("Password: %s\n", password);

            // login here
            bool res = login(username, password);
            close_db();
            if (res){
                write(connfd, "\033[1;32mYou have successfully logged in!\033[0m", sizeof(buff));
                for (;;) {
                    bzero(buff, MAX); 
  
                    // read the message from client and copy it in buffer 
                    read(connfd, buff, sizeof(buff)); 
                    if (strncmp(buff, "1", 1) == 0) {
                        display_account_info(connfd, name);
                        close_db();
                    } else if (strncmp(buff, "2", 1) == 0) {
                      bzero(buff, sizeof(buff));
                        find_books(connfd);
                        close_db();
                    } else if (strncmp(buff, "3", 1) == 0) {
                        int user_id = get_user_id(name);
                        rent_book(connfd, user_id);
                        close_db();
                    } else if (strncmp(buff, "4", 1) == 0) {
                        int user_id = get_user_id(name);
                        return_book(connfd, user_id);
                        close_db();
                    } else if (strncmp(buff, "5", 1) == 0) {
                        break;
                    }
                }
                write(connfd, "\033[1;31mexit\033[0m", 4); 
                printf("\033[1;31mServer Exit...\033[0m\n");
                break;
            }
            else{
                write(connfd, "\033[1;31mLogin failed. Check credentials\033[0m", sizeof(buff));
            }
            
        } else if (strncmp(buff, "2", 1) == 0) {
            // Option 2: Create Account
            write(connfd, "create", 6);

            char username[50], password[50];
            // int zip;
            
            // get username
            bzero(buff, sizeof(buff));
            read(connfd, buff, sizeof(buff));
            sscanf(buff, "%49s", username);
            printf("Username: %s\n", username);

            // get password
            bzero(buff, sizeof(buff));
            read(connfd, buff, sizeof(buff));
            sscanf(buff, "%49s", password); 
            printf("Password: %s\n", password);

            // get zip
            // bzero(buff, sizeof(buff));
            // read(connfd, buff, sizeof(buff));
            // sscanf(buff, "%d", &zip);
            // printf("Zip: %d\n", zip);

            // create account here
            bool res = create_account(username, password);
            close_db();
            if (res){
                write(connfd, "\033[1;32mAccount created successfully!\033[0m", sizeof(buff));
            } else {
                write(connfd, "\033[1;31mAccount creation failed. Check username\033[0m", sizeof(buff));
            }
            
        } else if (strncmp(buff, "3", 1) == 0) {
            // Option 3: Exit
            write(connfd, "exit", 4); 
            printf("Server Exit...\n");
            break;
        } else {
            // Handle invalid input
            write(connfd, "\033[1;31mInvalid option. Please try again.\033[0m", sizeof(buff));
        }
    } 
} 
  
// Driver function 
int main() 
{ 
    // initialize database
    init_db();
    close_db();
    initialize_books();
    close_db();
    
    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("\033[1;31msocket creation failed...\033[0m\n"); 
        exit(0); 
    } 
    else
        printf("\033[1;32mSocket successfully created..\033[0m\n"); 
    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("\033[1;31msocket bind failed...\033[0m\n"); 
        exit(0); 
    } 
    else
        printf("\033[1;32mSocket successfully binded..\033[0m\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 5)) != 0) { 
        printf("\033[1;31mListen failed...\033[0m\n"); 
        exit(0); 
    } 
    else
        printf("\033[1;32mServer listening..\033[0m\n"); 
    len = sizeof(cli); 
    
    // Start While Loop to allow fork process
    // Accept the data packet from client and verification 
    while(1) {
       connfd = accept(sockfd, (SA*)&cli, &len); 
        if (connfd < 0) { 
            printf("\033[1;31mserver accept failed...\033[0m\n"); 
            exit(0); 
        } 
        else
            printf("\033[1;32mserver accept the client...\033[0m\n");  
        if (fork() == 0) {
            close(sockfd);
            func(connfd);
            close(connfd);
            exit(0);
        }
        close(connfd);
    }
    close(sockfd); 
}

// Database connection
sqlite3 *db;

// Function to close the database connection
void close_db() {
  if (db != NULL) {
    sqlite3_close(db);
    db = NULL;
    printf("\033[1;31mDatabase connection closed.\033[0m\n");
  }
}

// Initialize the database
void init_db() {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "\033[1;31mCan't open database: %s\033[0m\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("\033[1;32mOpened database successfully. Init DB\033[0m\n");
  }

  // Create users table
  char *users_table = "CREATE TABLE IF NOT EXISTS users ("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                      "name TEXT, "
                      "password TEXT);";
  sqlite3_exec(db, users_table, 0, 0, 0);

  // Create books table
  char *books_table = "CREATE TABLE IF NOT EXISTS books ("
                      "id INTEGER PRIMARY KEY, "
                      "name TEXT, "
                      "author TEXT, "
                      "time_limit INTEGER, "
                      "availability INTEGER);";
  sqlite3_exec(db, books_table, 0, 0, 0);

  // Create rented_books table with rental_period
  char *rented_books_table = "CREATE TABLE IF NOT EXISTS rented_books ("
                             "user_id INTEGER, "
                             "book_id INTEGER, "
                             "return_date INTEGER, "
                             "FOREIGN KEY (user_id) REFERENCES users(id), "
                             "FOREIGN KEY (book_id) REFERENCES books(id));";
  sqlite3_exec(db, rented_books_table, 0, 0, 0);
}

// function to prepopulate books table
void initialize_books() {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "\033[1;31mCan't open database: %s\033[0m\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("\033[1;32mOpened database successfully. Initialize books\033[0m\n");
  }
  sqlite3_stmt *stmt;
  const char *check_sql = "SELECT COUNT(*) FROM books;";
  sqlite3_prepare_v2(db, check_sql, -1, &stmt, 0);
  sqlite3_step(stmt);
  int count = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);

  if (count == 0) {
    const char *insert_sql =
        "INSERT INTO books (id, name, author, time_limit, availability) "
        "VALUES "
        "(1, 'C Programming', 'Dennis Ritchie', 2, 5),"
        "(2, 'Data Structures', 'Mark Allen Weiss', 4, 3),"
        "(3, 'Algorithms', 'Robert Sedgewick', 1, 4),"
        "(4, 'Operating Systems', 'Andrew Tanenbaum', 3, 6),"
        "(5, 'Computer Networks', 'James Kurose', 4, 2),"
        "(6, 'Artificial Intelligence', 'Stuart Russell', 3, 5),"
        "(7, 'Machine Learning', 'Tom Mitchell', 2, 3),"
        "(8, 'Database Systems', 'Raghu Ramakrishnan', 3, 4),"
        "(9, 'Software Engineering', 'Ian Sommerville', 3, 5),"
        "(10, 'Computer Graphics', 'Donald Hearn', 2, 3);";

    sqlite3_exec(db, insert_sql, 0, 0, 0);
    printf("\033[1;32mBooks added to the database.\033[0m\n");
  }
}

// Function to create a new account
bool create_account(char username[], char password[]) {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "\033[1;31mCan't open database: %s\033[0m\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("\033[1;32mDatabase opened successfully\033[0m\n");
  }

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
    sqlite3_finalize(stmt);
    return false;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    int user_count = sqlite3_column_int(stmt, 0);
    printf("Debug: User count from query: %d\n", user_count);

    if (user_count > 0) {
      printf("\033[1;31mUsername already exists. Please try another one.\033[0m\n");
      sqlite3_finalize(stmt);
      return false;
    }
  } else {
    printf("\033[1;31mError: sqlite3_step failed with error code: %d\033[0m\n", rc);
  }
  sqlite3_finalize(stmt);

  // Insert new user into the database
  char sql[256];
  snprintf(sql, sizeof(sql),
           "INSERT INTO users (name, password) VALUES ('%s', '%s');",
           username, password);
  printf("Debug: Executing SQL insert: %s\n", sql);

  rc = sqlite3_exec(db, sql, 0, 0, 0);
  if (rc != SQLITE_OK) {
    printf("\033[1;31mError: sqlite3_exec failed with error: %s\033[0m\n", sqlite3_errmsg(db));
    return false;
  } else {
    printf("\033[1;32mDebug: Account created successfully for user: %s\033[0m\n", username);
    return true;
  }
}

// Function to log in
bool login(char username[], char password[]) {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "\033[1;31mCan't open database: %s\033[0m\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("\033[1;32mDatabase opened successfully, login\033[0m\n");
  }

  sqlite3_stmt *stmt;

  // Prepare SQL query to check the username and password
  char sql[256];
  snprintf(sql, sizeof(sql), "SELECT password FROM users WHERE name='%s';",
           username);
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK) {
    printf("\033[1;31mLogin failed: error retrieving user information\033[0m\n");
    return false;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    const unsigned char *db_password = sqlite3_column_text(stmt, 0);

    // Compare passwords
    if (strcmp((const char *)db_password, password) == 0) {
      sqlite3_finalize(stmt);
      return true;
    } else {
      printf("\033[1;31mLogin failed: incorrect password\033[0m\n");
    }
  } else {
    printf("\033[1;31mLogin failed: username not found\033[0m\n");
  }

  sqlite3_finalize(stmt);
  return false;
}

// Function to display user account information, including rented books
void display_account_info(int connfd, char name[]) {
  sqlite3 *db;
  char buffer[200] = {0};
  char buff[MAX];
  char result[1020]; 
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("\033[1;32mDatabase opened successfully, login\033[0m\n");
  }

  sqlite3_stmt *stmt;

  // Query for user information based on name
  snprintf(buffer, sizeof(buffer),
           "SELECT id, name FROM users WHERE name = '%s';", name);
  rc = sqlite3_prepare_v2(db, buffer, -1, &stmt, 0);

  if (rc != SQLITE_OK) {
    // Handle SQL preparation failure
    const char *err_msg = sqlite3_errmsg(db);
    printf("Error preparing user query: %s\n",
             err_msg);
    write(connfd, "An error occurred getting account information", sizeof(buff));
    return;
  }

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    int user_id = sqlite3_column_int(stmt, 0);
    const char *username = (const char *)sqlite3_column_text(stmt, 1);
    // int zip = sqlite3_column_int(stmt, 2);

    // Send user info to client
    snprintf(result, sizeof(result),
             "\033[1;32mUser Account Information:\nName: %s\033[0m\n", username);

    sqlite3_finalize(stmt);

    // Query for rented books using user_id
    printf("\033[1;32mUser ID: %d\033[0m\n", user_id);
    snprintf(buffer, sizeof(buffer),
             "SELECT b.id, b.name, b.author, rb.return_date FROM rented_books rb "
             "JOIN books b ON rb.book_id = b.id WHERE rb.user_id = %d;",
             user_id);
    rc = sqlite3_prepare_v2(db, buffer, -1, &stmt, 0);

    if (rc != SQLITE_OK) {
      // Handle SQL preparation failure for rented books query
      const char *err_msg = sqlite3_errmsg(db);
      printf("Error preparing rented books query: %s\n", err_msg);
      printf("sql: %s\n", buffer);
      sqlite3_finalize(stmt);
      write(connfd, "An error occurred getting account information", sizeof(buff));
      return;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      // If rented books are found
      strcat(result, "\033[1;32mRented Books:\033[0m\n");
      strcat(result, "\033[1;32m Book ID   Name                            Author                Renting Time \033[0m\n");
      do {
        const char *book_id = (const char *)sqlite3_column_text(stmt, 0);
        const char *book_name = (const char *)sqlite3_column_text(stmt, 1);
        const char *author = (const char *)sqlite3_column_text(stmt, 2);
        int return_date = sqlite3_column_int(stmt, 3);

        // Calculate the remaining renting time (in seconds)
        time_t now = time(NULL); // Current time
        int renting_time_left = return_date - now; // Time left in seconds

        char renting_time_str[100];

        if (renting_time_left < 0) {
            // Overdue
            snprintf(renting_time_str, sizeof(renting_time_str), "Overdue");
        } else {
            // Calculate renting time left in weeks or days
            if (renting_time_left < 7 * 24 * 60 * 60) { // Less than a week
                int days_left = renting_time_left / (60 * 60 * 24);
                snprintf(renting_time_str, sizeof(renting_time_str), "%d days left", days_left);
            } else { // More than a week
                int weeks_left = renting_time_left / (7 * 24 * 60 * 60);
                snprintf(renting_time_str, sizeof(renting_time_str), "%d weeks left", weeks_left);
            }
        }

        char book_info[100];
        // snprintf(book_info, sizeof(book_info),
        //          "Book ID: %s\t Name: %s\t Author: %s\t Renting Time: %s \n", book_id, book_name,
        //          author, renting_time_str);
        snprintf(book_info, sizeof(book_info),
         "\033[1;32m %-8s  %-30s  %-20s  %-12s \033[0m\n", book_id, book_name, author, renting_time_str);

        strcat(result, book_info);
      } while (sqlite3_step(stmt) == SQLITE_ROW);
    } else {
      // No rented books found
      strcat(result, "\033[1;31mNo rented books.\033[0m\n");
    }
    // printf("%s", result);
    write(connfd, result, sizeof(result));
  } else {
    // Error retrieving account information
    write(connfd, "\033[1;31mAn error occurred getting account information\033[0m]", sizeof(buff));
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db); // Close the database connection
}


// function to get user id based on name
int get_user_id(char name[]) {
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "\033[1;31mCan't open database: %s\033[0m\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("\033[1;32mDatabase opened successfully. Get user id.\033[0m\n");
  }
  sqlite3_stmt *stmt;
  const char *sql =
      "SELECT id FROM users WHERE name = ? LIMIT 1;"; // SQL query to fetch the
                                                      // user ID based on name
  int user_id = -1; // Default value for "not found"

  // Prepare the SQL statement
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
    printf("\033[1;31mFailed to prepare statement: %s\033[0m\n", sqlite3_errmsg(db));
    return user_id;
  }

  // Bind the name parameter to the SQL query
  if (sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC) != SQLITE_OK) {
    printf("\033[1;31mFailed to bind name: %s\033[0m\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
    return user_id;
  }

  // Execute the query and check if a result is returned
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    // Retrieve the user ID from the result set
    user_id = sqlite3_column_int(stmt, 0); // 0 is the column index for "id"
  } else {
    printf("\033[1;31User not found.\033[0m\n");
  }

  // Finalize the statement to free resources
  sqlite3_finalize(stmt);
  close_db();
  return user_id;
}

// Function to display all books in the bookstore
void find_books(int connfd) {
    char buff[MAX];
    char result[2048]; 
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "\033[1;31mCan't open database: %s\033[0m\n", sqlite3_errmsg(db));
    exit(0);
  } else {
    printf("\033[1;32mDatabase opened successfully. Find Book\033[0m\n");
  }
  char buffer[2048] = {0};
  sqlite3_stmt *stmt;

  // SQL query to select all books from the books table
  const char *sql =
      "SELECT id, name, author, time_limit, availability FROM books;";
  rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

  if (rc != SQLITE_OK) {
     write(connfd, "\033[1;31mError retrieving books from the database.\033[0m", sizeof(buff));
    return;
  }

  bzero(result, sizeof(result));
  // Check if there are any books available
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    strcat(result, "\033[1;32mAvailable Books:\033[0m\n");
    strcat(result, "\033[1;32m ID      Name                            Author                Time Limit  Availability \033[0m\n");

    // Loop through each row in the result set
    do {
      int id = sqlite3_column_int(stmt, 0);
      const char *name = (const char *)sqlite3_column_text(stmt, 1);
      const char *author = (const char *)sqlite3_column_text(stmt, 2);
      char res[50]; 
      int return_date = sqlite3_column_int(stmt, 3);
      sprintf(res, "%d weeks", return_date);
      int availability = sqlite3_column_int(stmt, 4);

      char book_info[200];
      // snprintf(
      //     book_info, sizeof(book_info),
      //     "ID: %d\t Name: %s\t Author: %s\t Time Limit: %d weeks\t, Availability: %d\n",
      //     id, name, author, return_date, availability);
          snprintf(
            book_info, sizeof(book_info),
            "\033[1;32m %-6d  %-30s  %-20s  %-10s  %-12d \033[0m\n",
            id, name, author, res, availability);

      strcat(result, book_info);
      
    } while (sqlite3_step(stmt) == SQLITE_ROW);
    // printf("%s", result);
    write(connfd, result, sizeof(result));
  } else {
    write(connfd, "\033[1;31mNo books available in the bookstore.\033[0m", sizeof(buff));
  }

  sqlite3_finalize(stmt);
}


// Function to rent a book using user_id
void rent_book(int connfd, int user_id) {
    char buff[MAX];
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "\033[1;31mCan't open database: %s\033[0m\n", sqlite3_errmsg(db));
    return;
  }

  sqlite3_stmt *stmt;
  char buffer[1024] = {0};
  int book_id, rental_period = 20, time_limit_weeks; // Rental period (e.g., 20 days)

  // Ask user to select a book to rent
  bzero(buff, sizeof(buff));
    read(connfd, buff, sizeof(buff));
    sscanf(buff, "%d", &book_id);
    printf("Book ID: %d\n", book_id);

    // Fetch the time_limit (in weeks) for the selected book from the books table
    snprintf(buffer, sizeof(buffer),
             "SELECT time_limit FROM books WHERE id = %d;", book_id);
    rc = sqlite3_prepare_v2(db, buffer, -1, &stmt, 0);

    if (rc != SQLITE_OK) {
        write(connfd, "\033[1;31mError renting book.\033[0m", sizeof(buff));
        sqlite3_finalize(stmt);
        return;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // Retrieve the time limit in weeks for the selected book
        time_limit_weeks = sqlite3_column_int(stmt, 0);
        printf("Time limit (weeks) for the book: %d\n", time_limit_weeks);
    } else {
        printf("\033[1;31mError: Book not found or invalid book ID.\033[0m\n");
        write(connfd, "\033[1;31mBook not found or invalid book ID.\033[0m", sizeof(buff));
        sqlite3_finalize(stmt);
        return;
    }
    sqlite3_finalize(stmt);

    // Convert time limit (in weeks) to days
    int rental_period_days = time_limit_weeks * 7;

    // Get current time and calculate return date as a timestamp (in seconds)
    time_t current_time;
    time(&current_time);  // Get the current time
    time_t return_timestamp = current_time + (rental_period_days * 24 * 60 * 60); 

  // Update books table with return date and decrease availability
  snprintf(buffer, sizeof(buffer),
           "UPDATE books SET availability = availability - "
           "1 WHERE id = %d;",
            book_id);
  rc = sqlite3_exec(db, buffer, 0, 0, 0);
  printf("Debug: Executing SQL update: %s\n", buffer);

  // Insert into rented_books table with rental period
  snprintf(buffer, sizeof(buffer),
           "INSERT INTO rented_books (user_id, book_id, return_date) VALUES "
           "(%d, %d, %d);",
           user_id, book_id, return_timestamp);
  rc = sqlite3_exec(db, buffer, 0, 0, 0);
  printf("Debug: Executing SQL insert: %s\n", buffer);

    bzero(buff, sizeof(buff));
  if (rc != SQLITE_OK) {
    write(connfd, "\033[1;31mError renting book.\033[0m", sizeof(buff));
    const char *err_msg = sqlite3_errmsg(db);
    printf("Debug: Error message: %s\n", err_msg);
  } else {
     printf("\033[1;32mBook rented successfully! %s\033[0m\n");
    write(connfd, "\033[1;32mBook rented successfully!\033[0m", sizeof(buff));
  }
}

// Function to return a book using user_id
void return_book(int connfd, int user_id) {
    char buff[MAX];
  int rc = sqlite3_open("bookstore.db", &db);
  if (rc) {
    fprintf(stderr, "\033[1;31mCan't open database: %s\033[0m\n", sqlite3_errmsg(db));
    return;
  }

  sqlite3_stmt *stmt;
  char buffer[1024] = {0};
  int book_id;

  // Ask the user to select the book they want to return
    bzero(buff, sizeof(buff));
    read(connfd, buff, sizeof(buff));
    sscanf(buff, "%d", &book_id);
    printf("Please select the Book ID of the book you would like to return: %d\n", book_id);

  snprintf(buffer, sizeof(buffer), "SELECT * FROM rented_books WHERE user_id = %d AND book_id = %d;", user_id, book_id);
    rc = sqlite3_prepare_v2(db, buffer, -1, &stmt, 0);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        sqlite3_finalize(stmt);
        snprintf(buffer, sizeof(buffer), "DELETE FROM rented_books WHERE user_id = %d AND book_id = %d;", user_id, book_id);
        rc = sqlite3_exec(db, buffer, 0, 0, 0);
        snprintf(buffer, sizeof(buffer), "UPDATE books SET availability = availability + 1 WHERE id = %d;", book_id);
        rc = sqlite3_exec(db, buffer, 0, 0, 0);
        write(connfd, "\033[1;32mBook returned successfully!\033[0m", sizeof(buff));
    } else {
        write(connfd, "\033[1;31mYou did not rent this book!\033[0m", sizeof(buff));
        sqlite3_finalize(stmt);
    }
}


