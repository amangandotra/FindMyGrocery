import __main__
import smtplib
from email.mime.text import MIMEText
import random
EMAIL = "forfindmybooktesting@gmail.com"
PASSWORD = "ztsn iynu hgaf soqh"
def generate_otp():
    return str(random.randint(100000, 999999))

def send_otp_email(to_email, user_name, book_name, book_code, due_date, otp):
    body = f"""
Hello {user_name},

You requested to issue the following book:

📚 Book Name: {book_name}
🆔 Book Code: {book_code}
📅 Due Date: {due_date}

Your One-Time Password (OTP) is:

🔐 {otp}

Please enter this OTP on the library device to confirm your book issue.

If you did not request this, please ignore this email.

Regards,
FindMyBook Smart Library
"""

    msg = MIMEText(body)
    msg["Subject"] = "FindMyBook – Book Issue Verification OTP"
    msg["From"] = EMAIL
    msg["To"] = to_email

    server = smtplib.SMTP_SSL("smtp.gmail.com", 465)
    server.login(EMAIL, PASSWORD)
    server.send_message(msg)
    server.quit()
def send_issue_success_email(to_email, user_name, book_name, book_code, due_date):
    subject = "📚 Book Issued Successfully – Smart Library"

    body = f"""
Hello {user_name},

Your book has been successfully issued from the Smart Library.

📖 Book Name : {book_name}
🏷 Book Code : {book_code}
📅 Due Date  : {due_date}

Please make sure to return the book on or before the due date to avoid any penalties.

If you have any questions, feel free to contact the library desk.

Happy Reading! 😊
Smart Library System
"""

    msg = MIMEText(body)
    msg["Subject"] = subject
    msg["From"] = EMAIL
    msg["To"] = to_email

    server = smtplib.SMTP_SSL("smtp.gmail.com", 465)
    server.login(EMAIL, PASSWORD)
    server.send_message(msg)
    server.quit()
    
def send_return_email(to_email, name, copy_uid):
    msg = MIMEText(
        f"""
Hello {name},

Your book (ID: {copy_uid}) has been successfully returned.

Thank you for using Smart Library 📚

Regards,
Smart Library System
"""
    )

    msg["Subject"] = "Book Returned Successfully"
    msg["From"] = EMAIL
    msg["To"] = to_email

    server = smtplib.SMTP_SSL("smtp.gmail.com", 465)
    server.login(EMAIL, PASSWORD)
    server.send_message(msg)
    server.quit()
