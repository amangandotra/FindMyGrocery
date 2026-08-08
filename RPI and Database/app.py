import random
from socket import socket
from flask import Flask, Response, render_template, request, jsonify
from database import get_db, init_db
from otp_send import send_otp_email, generate_otp, send_issue_success_email, send_return_email
import requests
import json
from datetime import datetime, timedelta

import speech_recognition as sr
import tempfile
import os

app = Flask(__name__)
init_db()

@app.route("/search", methods=["GET"])
def search_book():
    name = request.args.get("name", "").strip().lower()

    if len(name) < 2:
        return jsonify({"error": "Query too short"}), 400

    db = get_db()
    cur = db.cursor()

    cur.execute("""
    SELECT * FROM book_types
    WHERE LOWER(name) LIKE ?
    """, ('%' + name + '%',))

    result = cur.fetchone()
    db.close()

    if result:
        print(result)
        side = "Left" if result[3] == "L" else "Right"
        return jsonify({
            "book_type_id": result[0],
            "name": result[1],
            "rack": result[2],
            "side": side,
            "row": result[4],
            "column": result[5],
        })

    return jsonify({"error": "Book not found"}), 404

# for issueeing
@app.route("/api/issue/preview", methods=["POST"])
def issue_preview():
    data = request.get_json(silent=True)

    if not data or "copy_uid" not in data or "enrollment_no" not in data:
        return jsonify({"error": "Invalid request data"}), 400

    copy_uid = data["copy_uid"]
    enrollment_no = data["enrollment_no"]

    db = get_db()
    cur = db.cursor()

    # ---- Book details ----
    cur.execute("""
        SELECT bt.name, bc.copy_uid, bt.book_type_id
        FROM book_copies bc
        JOIN book_types bt ON bc.book_type_id = bt.book_type_id
        WHERE bc.copy_uid = ?
    """, (copy_uid,))
    book = cur.fetchone()

    if not book:
        db.close()
        return jsonify({"error": "Invalid book"}), 400

    # ---- User details (with email) ----
    cur.execute("""
        SELECT name, email
        FROM users
        WHERE enrollment_no = ?
    """, (enrollment_no,))
    user = cur.fetchone()

    if not user:
        db.close()
        return jsonify({"error": "Invalid user"}), 400

    cur.execute("SELECT status FROM book_copies WHERE copy_uid = ?", (copy_uid,))
    status_row = cur.fetchone()

    if not status_row or status_row[0] != "available":
        db.close()
        return jsonify({"error": "Already issued"}), 400

    due = (datetime.now() + timedelta(days=14)).strftime("%Y-%m-%d")

    response = {
        "book_name": book[0],
        "book_code": book[2],
        "user_name": user[0],
        "email": user[1], 
        "due_date": due
    }

    print("Issue preview:", response)

    db.close()
    return jsonify(response), 200

RACK_IPS = {
    "a": "10.47.34.73",
    "b": "10.47.34.56"
}

@app.route("/blink", methods=["POST"])
def blink_book():
    data = request.json

    rack = str(data.get("rack", "")).lower()
    row = int(data.get("row", 0))
    column = int(data.get("column", 0))
    side = str(data.get("side", "")).lower()   

    if side not in ("left", "right"):
        return jsonify({"error": "invalid side"}), 400

    # ---- Random color ----
    r = random.randint(0, 255)
    g = random.randint(0, 255)
    b = random.randint(0, 255)

    rack_ip = RACK_IPS.get(rack)
    if not rack_ip:
        return jsonify({"error": "invalid rack"}), 400

    url = f"http://{rack_ip}/highlight"
    params = {
        "row": row,
        "side": side,
        "col": column,
        "r": r,
        "g": g,
        "b": b
    }

    print("Calling:", url, params)

    try:
        resp = requests.get(url, params=params, timeout=3)
        print("Rack response:", resp.text)

        if resp.status_code != 200:
            return jsonify({"error": "rack rejected request"}), 500

    except Exception as e:
        print("Rack error:", e)
        return jsonify({"error": "rack not reachable"}), 500

    response_data = {
        "status": "ok",
        "rack": rack.upper(),
        "row": row,
        "column": column,
        "side": side,
        "color": {
            "r": r,
            "g": g,
            "b": b
        }
    }
    print("Response data:", response_data)
    resp_json = json.dumps(response_data)

    return Response(
        resp_json,
        status=200,
        mimetype="application/json",
        headers={
            "Content-Length": str(len(resp_json))
        }
    )
@app.route("/blink_rack", methods=["POST"])
def blink_rack():
    data = request.json

    rack = str(data.get("rack", "")).lower()

    rack_ip = RACK_IPS.get(rack)
    if not rack_ip:
        return jsonify({"error": "invalid rack"}), 400

    url = f"http://{rack_ip}/glowrack"
    params = {
        "r": 255,
        "g": 100,
        "b": 0
    }

    print("Calling:", url, params)

    try:
        resp = requests.get(url, params=params, timeout=3)
        print("Rack response:", resp.text)

        if resp.status_code != 200:
            return jsonify({"error": "rack rejected request"}), 500

    except Exception as e:
        print("Rack error:", e)
        return jsonify({"error": "rack not reachable"}), 500

    return jsonify({"status": "ok"})

ESPBOX_IP =  "10.47.34.14"
@app.route("/rfid", methods=["POST"])
def rfid_receive():
    uid = request.json.get("uid")

    try:
        requests.post(f"http://{ESPBOX_IP}/rfid", json={"uid":uid}, timeout=2)
    except Exception as e:
        return {"error":"espbox offline"},500

    return {"ok":True}

# OTP GENERATION
@app.route("/api/issue/request_otp", methods=["POST"])
def issue_request_otp():
    data = request.json
    enrollment_no = data["enrollment_no"]
    copy_uid = data["copy_uid"]

    db = get_db()
    cur = db.cursor()

    cur.execute("SELECT name, email FROM users WHERE enrollment_no=?", (enrollment_no,))
    user = cur.fetchone()
    if not user:
        return jsonify({"error": "Invalid user"}), 400

    cur.execute("""
        SELECT bt.name, bt.book_type_id
        FROM book_copies bc
        JOIN book_types bt ON bc.book_type_id = bt.book_type_id
        WHERE bc.copy_uid=?
    """, (copy_uid,))
    book = cur.fetchone()
    if not book:
        return jsonify({"error": "Invalid book"}), 400

    from datetime import datetime, timedelta
    due = (datetime.now() + timedelta(days=14)).strftime("%Y-%m-%d")

    otp = generate_otp()

    send_otp_email(
        user[1], user[0],
        book[0], book[1],
        due, otp
    )

    return jsonify({
        "otp": otp,
        "email": user[1]
    })

@app.route("/api/issue/commit", methods=["POST"])
def issue_commit():
    data = request.json
    copy_uid = data["copy_uid"]
    enrollment_no = data["enrollment_no"]

    db = get_db()
    cur = db.cursor()

    cur.execute("""
        SELECT bt.name, bt.book_type_id, u.name, u.email
        FROM book_copies bc
        JOIN book_types bt ON bc.book_type_id = bt.book_type_id
        JOIN users u ON u.enrollment_no = ?
        WHERE bc.copy_uid = ?
    """, (enrollment_no, copy_uid))

    row = cur.fetchone()
    if not row:
        return jsonify({"error": "Invalid data"}), 400

    book_name, book_code, user_name, email = row

    cur.execute("SELECT status FROM book_copies WHERE copy_uid=?", (copy_uid,))
    status = cur.fetchone()[0]

    if status != "available":
        return jsonify({"error": "Book already issued"}), 400

    from datetime import datetime, timedelta
    due_date = (datetime.now() + timedelta(days=14)).strftime("%Y-%m-%d")

    cur.execute("""
        UPDATE book_copies SET status='issued' WHERE copy_uid=?
    """, (copy_uid,))

    cur.execute("""
        INSERT INTO issue_logs(copy_uid, enrollment_no, action)
        VALUES (?, ?, 'issue')
    """, (copy_uid, enrollment_no))

    db.commit()
    db.close()

    try:
        send_issue_success_email(
            email,
            user_name,
            book_name,
            book_code,
            due_date
        )
    except Exception as e:
        print("Email send failed:", e)

    return jsonify({
        "status": "success",
        "message": "Book issued and email sent"
})
@app.route("/api/return", methods=["POST"])
def return_book():
    data = request.json
    copy_uid = data.get("copy_uid")

    db = get_db()
    cur = db.cursor()

    cur.execute("SELECT status FROM book_copies WHERE copy_uid=?", (copy_uid,))
    row = cur.fetchone()
    if not row:
        return jsonify({"error": "Invalid book"}), 400

    if row[0] != "issued":
        return jsonify({"error": "Book is not issued"}), 400

    cur.execute("""
        SELECT enrollment_no FROM issue_logs
        WHERE copy_uid=? AND action='issue'
        ORDER BY timestamp DESC LIMIT 1
    """, (copy_uid,))
    user_row = cur.fetchone()
    enrollment_no = user_row[0] if user_row else "unknown"

    cur.execute("UPDATE book_copies SET status='available' WHERE copy_uid=?", (copy_uid,))

    cur.execute("""
        INSERT INTO issue_logs(copy_uid, enrollment_no, action)
        VALUES (?, ?, 'return')
    """, (copy_uid, enrollment_no))

    cur.execute("SELECT name, email FROM users WHERE enrollment_no=?", (enrollment_no,))
    user = cur.fetchone()

    db.commit()
    db.close()

    if user:
        send_return_email(user[1], user[0], copy_uid)

    return jsonify({"status": "ok"})

# THeFT Detection
@app.route("/api/book/status", methods=["POST"])
def book_status():
    data = request.json
    copy_uid = data.get("copy_uid")

    db = get_db()
    cur = db.cursor()

    cur.execute("SELECT status FROM book_copies WHERE copy_uid=?", (copy_uid,))
    row = cur.fetchone()

    if not row:
        return jsonify({"issued": False})

    issued = row[0] != "available"

    return jsonify({
        "issued": issued
    })


# WEBSITE ROUTES
@app.route("/map")
def map_view():
    rack = request.args.get("r", "").upper()
    row = request.args.get("row")
    col = request.args.get("col")
    side = request.args.get("side", "").lower()

    if not rack or not row or not col or side not in ("left", "right"):
        return "Invalid parameters", 400

    return render_template("openonphone.html")
@app.route("/api/map-data")
def api_map_data():
    rack = request.args.get("r", "").upper()
    row = request.args.get("row", type=int)
    col = request.args.get("col", type=int)
    side = request.args.get("side", "").lower()

    print("API MAP DATA:", rack, row, col, side)

    if not rack or row is None or col is None or side not in ("left", "right"):
        return jsonify({"error": "invalid parameters"}), 400

    side_db = side[0].upper()   

    db = get_db()
    cur = db.cursor()

    cur.execute("""
        SELECT bt.name, bt.book_type_id, bc.copy_uid, bc.status
        FROM book_copies bc
        JOIN book_types bt ON bc.book_type_id = bt.book_type_id
        WHERE bt.rack = ?
          AND bt.row = ?
          AND bt.column = ?
          AND bt.side = ?
        LIMIT 1
    """, (rack, row, col, side_db))

    book = cur.fetchone()

    if not book:
        return jsonify({"error": "no book"}), 404

    book_name, book_code, copy_uid, status = book

    resp = {
        "book_name": book_name,
        "book_code": book_code,
        "status": status
    }

    print("API RESPONSE:", resp)
    return jsonify(resp)

# ADMIN Page

def utc_to_ist(ts):
    dt = datetime.strptime(ts, "%Y-%m-%d %H:%M:%S")
    dt_ist = dt + timedelta(hours=5, minutes=30)
    return dt_ist.strftime("%d %b %Y, %I:%M %p")

@app.route("/admin")
def admin_page():
    db = get_db()
    cur = db.cursor()

    cur.execute("SELECT * FROM book_types")
    book_types = cur.fetchall()

    cur.execute("SELECT * FROM book_copies")
    book_copies = cur.fetchall()

    cur.execute("SELECT * FROM users")
    users = cur.fetchall()

    cur.execute("""
        SELECT id, copy_uid, enrollment_no, action, timestamp
        FROM issue_logs
        ORDER BY timestamp DESC
        LIMIT 200
    """)
    issue_logs = cur.fetchall()
    issue_logs_ist = []
    for l in issue_logs:
        l = list(l)
        l[4] = utc_to_ist(l[4])   
        issue_logs_ist.append(l)

    return render_template(
        "admin.html",
        book_types=book_types,
        book_copies=book_copies,
        users=users,
        issue_logs=issue_logs_ist
    )


ESPBOX_IP = ""
# IP ADRRESS ISSUE SOLVE
@app.route('/register_ip', methods=['POST'])
def register_ip():
    data = request.get_json()
    device = data.get('device')
    ip = data.get('ip')
    if not device or not ip:
        return jsonify({'error': 'Invalid data'}), 400
        
    app.logger.info(f"Registered {device} ? {ip}")

    response = {}
    if device.lower().startswith("esp32"):
        ESPBOX_IP = ip 
        hostname = socket.gethostname()
        server_ip = socket.gethostbyname(hostname)
        response['server_ip'] = server_ip
    else:
        RACK_IPS[device[-1].lower()] = ip
    return jsonify(response), 200


@app.route("/voiceweb")
def index():
    return render_template("voice.html")

# ===============================
# TRIGGER LISTENING OVERLAY
# ===============================

@app.route("/voice_start")
def voice_start():

    try:
        requests.get(f"http://{ESPBOX_IP}/voice_start",timeout=2)
    except:
        pass

    return "OK"

# ===============================
# MAIN VOICE PROCESSING
# ===============================

@app.route("/voice", methods=["POST"])
def voice():

    if "audio" not in request.files:
        return {"error":"no audio"},400

    audio_file = request.files["audio"]

    tmp = tempfile.NamedTemporaryFile(delete=False,suffix=".wav")
    audio_file.save(tmp.name)

    recognizer = sr.Recognizer()

    try:

        with sr.AudioFile(tmp.name) as source:
            audio = recognizer.record(source)

        text = recognizer.recognize_google(audio).lower()

        print("VOICE:",text)

    except:

        try:
            requests.get(f"http://{ESPBOX_IP}/voice_error")
        except:
            pass

        return {"status":"error"}

    finally:
        os.unlink(tmp.name)


    # =====================
    # COMMAND PARSER
    # =====================

    try:

        if "issue book" in text:

            requests.get(f"http://{ESPBOX_IP}/open_issue")
            return {"status":"issue"}

        elif "return book" in text:

            requests.get(f"http://{ESPBOX_IP}/open_return")
            return {"status":"return"}

        elif "find" in text:

            book = text.replace("smart library","").replace("find","").strip()

            requests.get(f"http://{ESPBOX_IP}/find_book?name={book}")

            return {"status":"find","book":book}

        else:

            requests.get(f"http://{ESPBOX_IP}/voice_error")
            return {"status":"unknown"}

    except Exception as e:

        print("ESP ERROR:",e)
        return {"status":"fail"}

@app.route("/espcontrol")
def esp_control():
    return render_template("espcontrol.html")

@app.route("/register")
def register():

    db = get_db()
    cur = db.cursor()

    cur.execute("SELECT enrollment_no FROM users ORDER BY enrollment_no DESC LIMIT 1")
    row = cur.fetchone()

    if row:
        next_enroll = int(row[0]) + 1
    else:
        next_enroll = 101   # starting enrollment

    db.close()

    return render_template("register.html", enrollment_no=next_enroll)

@app.route("/register_user", methods=["POST"])
def register_user():

    name = request.form.get("name")
    email = request.form.get("email")

    db = get_db()
    cur = db.cursor()

    # get last enrollment
    cur.execute("SELECT enrollment_no FROM users ORDER BY enrollment_no DESC LIMIT 1")
    row = cur.fetchone()

    if row:
        enrollment = str(int(row[0]) + 1)
    else:
        enrollment = "1001"

    try:

        cur.execute(
            "INSERT INTO users (enrollment_no, name, email) VALUES (?, ?, ?)",
            (enrollment, name, email)
        )

        db.commit()

        return jsonify({
            "status": "success",
            "enrollment": enrollment
        })

    except Exception as e:
        return jsonify({"status": "error", "message": str(e)})

    finally:
        db.close()
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug= True)
    