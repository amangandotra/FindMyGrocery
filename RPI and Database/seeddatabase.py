import sqlite3
import random
import string

DB_NAME = "library.db"

# ---------- Helpers ----------

def random_rfid():
    return ''.join(random.choices("0123456789ABCDEF", k=8))

# ---------- Book layout with your IDs ----------

BOOK_TYPES = [
    ("ENG-UL",  "English Unleashed",        "A", "L", 1, 1),
    ("M-ADV",   "Maths Adventure",          "A", "L", 1, 2),
    ("HIN-JD",  "Hindi Jadu",               "A", "L", 2, 1),
    ("SCI",     "Explore Science",          "A", "L", 2, 2),

    ("PHY-UL",  "Physics Unleashed",        "A", "R", 1, 1),
    ("BIO",     "Biology Explorers",        "A", "R", 1, 2),
    ("CHEM",    "Chemistry Explosion",      "A", "R", 2, 1),
    ("GEO",     "Geography Quest",          "A", "R", 2, 2),

    ("KOT12",   "Knowledge Of Time",        "B", "L", 1, 1),
    ("KOS22",   "Knowledge Of Space",       "B", "L", 1, 2),
    ("KOE21",   "Knowledge Of Energy",      "B", "L", 2, 1),
    ("PYPRO",   "Python Programming",       "B", "L", 2, 2),

    ("THEC",    "The Art of C",             "B", "R", 1, 1),
    ("CWRC",    "Cities Without Roads",     "B", "R", 1, 2),
    ("AIPRO",   "AI Programming",           "B", "R", 2, 1),
    ("ELEC",    "Electronics Basics",       "B", "R", 2, 2),
]

COPIES_PER_BOOK = 4

# ---------- Main ----------

def main():
    conn = sqlite3.connect(DB_NAME)
    cur = conn.cursor()

    print("Inserting book types...")

    for bt in BOOK_TYPES:
        cur.execute("""
            INSERT OR IGNORE INTO book_types
            (book_type_id, name, rack, side, row, column)
            VALUES (?, ?, ?, ?, ?, ?)
        """, bt)

    print("Inserting book copies with dummy RFIDs...")

    for book_type_id, *_ in BOOK_TYPES:
        for _ in range(COPIES_PER_BOOK):
            uid = random_rfid()

            cur.execute("""
                INSERT INTO book_copies
                (copy_uid, book_type_id, status)
                VALUES (?, ?, 'available')
            """, (uid, book_type_id))

    conn.commit()
    conn.close()

    print("✅ Database seeded successfully!")
    print(f"   → {len(BOOK_TYPES)} book types")
    print(f"   → {len(BOOK_TYPES) * COPIES_PER_BOOK} total copies")


if __name__ == "__main__":
    main()
