import sqlite3

DB_NAME = "library.db"

def update_rfid(old_uid, new_uid):
    conn = sqlite3.connect(DB_NAME)
    cur = conn.cursor()

    cur.execute("""
        UPDATE book_copies
        SET copy_uid = ?
        WHERE copy_uid = ?
    """, (new_uid, old_uid))

    if cur.rowcount == 0:
        print("❌ No copy found with UID:", old_uid)
    else:
        print("✅ RFID updated successfully")

    conn.commit()
    conn.close()


if __name__ == "__main__":
    old_uid = input("Enter OLD UID from DB: ").strip()
    new_uid = input("Enter NEW RFID UID from scanner: ").strip()

    update_rfid(old_uid, new_uid)
