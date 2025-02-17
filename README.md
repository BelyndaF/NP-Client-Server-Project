# Library Management System

## Overview

This project is a simple client-server-based Library Management System implemented in C using sockets for communication and SQLite for database management. The system allows users to create accounts, log in, view available books, rent books, and return books.

## Features

- User account creation and authentication.
- View available books in the library.
- Rent books with a time limit.
- Return rented books.
- Server handles multiple client connections using forked processes.

## Technologies Used

- **C Programming Language**: For implementing the client and server.
- **SQLite**: For managing user and book records.
- **Sockets API**: For client-server communication.
- **Linux System Calls**: For handling networking and database interactions.

## File Structure

- `server.c`: Contains the implementation of the server-side logic, including database interactions and client handling.
- `client.c`: Implements the client-side operations such as user input handling and communication with the server.
- `bookstore.db`: SQLite database containing user and book information (generated dynamically).

## How to Run

### 1. Compile the Server and Client

```sh
gcc server.c -o server -lsqlite3
gcc client.c -o client
```

### 2. Run the Server

```sh
./server
```

The server will initialize the database and start listening for incoming connections.

### 3. Run the Client

```sh
./client
```

The client will attempt to connect to the server and display a menu for user interaction.

## Usage

1. **Create an Account**: Enter a username and password to register.
2. **Login**: Provide credentials to access the library.
3. **View Books**: Displays a list of available books.
4. **Rent a Book**: Enter a book ID to rent it (if available).
5. **Return a Book**: Enter a book ID to return a previously rented book.
6. **Exit**: Close the connection to the server.

## Database Schema

- `users` table:
  - `id` (INTEGER PRIMARY KEY)
  - `name` (TEXT UNIQUE)
  - `password` (TEXT)
- `books` table:
  - `id` (INTEGER PRIMARY KEY)
  - `name` (TEXT)
  - `author` (TEXT)
  - `time_limit` (INTEGER, weeks)
  - `availability` (INTEGER)
- `rented_books` table:
  - `user_id` (INTEGER, FOREIGN KEY to users)
  - `book_id` (INTEGER, FOREIGN KEY to books)
  - `return_date` (INTEGER, timestamp)

## Notes

- The server listens on `PORT 8080` by default.
- Users must rent books within the given time limit.
- Books are tracked based on availability.

## Future Enhancements

- Implement encryption for password storage.
- Improve UI by using a GUI-based client.
- Add book search and filtering features.

## Author

This project was developed as part of a network programming assignment.
Belinda Foley & Magda Wawrowska
