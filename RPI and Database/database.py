import sqlite3

DB_NAME = "library.db"

def get_db():
    return sqlite3.connect(DB_NAME)

def init_db():
    db = get_db()
    cur = db.cursor()

    # Book categories (16)
    cur.execute("""
        CREATE TABLE IF NOT EXISTS book_types (
            book_type_id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            rack TEXT NOT NULL,    
            side TEXT NOT NULL,    
            row INTEGER NOT NULL,  
            column INTEGER NOT NULL
        )
    """)

    # Individual book copies (64)
    cur.execute("""
        CREATE TABLE IF NOT EXISTS book_copies (
            copy_uid TEXT PRIMARY KEY,
            book_type_id TEXT NOT NULL,
            status TEXT DEFAULT 'available',
            FOREIGN KEY(book_type_id) REFERENCES book_types(book_type_id)
        );
    """)

    # Issue / return logs
    cur.execute("""
        CREATE TABLE IF NOT EXISTS issue_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            copy_uid TEXT NOT NULL,
            enrollment_no TEXT NOT NULL,
            action TEXT NOT NULL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,

            FOREIGN KEY(copy_uid) REFERENCES book_copies(copy_uid),
            FOREIGN KEY(enrollment_no) REFERENCES users(enrollment_no)
        );
    """)

    # Users (for OTP later)
    cur.execute("""
    CREATE TABLE IF NOT EXISTS users (
        enrollment_no TEXT PRIMARY KEY,
        name TEXT,
        email TEXT
    )
    """)

    db.commit()
    db.close()
